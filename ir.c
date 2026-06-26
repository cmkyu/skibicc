#include "ir.h"

#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "array.h"
#include "errors.h"
#include "parser.h"

//! Variable name and label name generator. Generated variable name will be
//! formatted like: "1_", "2_", ... Generated label name will be formatted
//! like "_label1", "_label2", ...
typedef struct name_generator {
  //! Counter for tracking the number for the next name.
  uint64_t name_count;
  uint64_t label_count;
} name_generator;

//! Initializes the name generator.
static void name_generator_init(name_generator* gen) {
  gen->name_count = 0;
  gen->label_count = 0;
}

//! Returns a unique variable name together with its string length.
static string_view name_generator_get_name(name_generator* gen) {
  char* data;
  size_t size;
  FILE* f = open_memstream(&data, &size);
  if (!f) {
    error("FATAL: generate_name(): open_memstream() failed.");
  }
  ++gen->name_count;
  // Names are formatted like 1_, 2_, 3_,...
  fprintf(f, "%" PRIu64 "_", gen->name_count);
  fclose(f);

  string_view name;
  name.data = data;
  name.length = size;
  return name;
}

//! Returns a unique label name together with its string length.
static string_view name_generator_get_label(name_generator* gen) {
  char* data;
  size_t size;
  FILE* f = open_memstream(&data, &size);
  if (!f) {
    error("FATAL: generate_name(): open_memstream() failed.");
  }
  ++gen->label_count;
  // Names are formatted like _label1, _label2, ...
  fprintf(f, "_label%" PRIu64, gen->name_count);
  fclose(f);

  string_view name;
  name.data = data;
  name.length = size;
  return name;
}

static ir_val* create_ir_val_var(name_generator* gen) {
  ir_val* val = calloc_safe(/*nelem=*/1, sizeof(ir_val));
  val->is_constant = false;
  val->val.var_name = name_generator_get_name(gen);
  return val;
}

static ir_val* create_ir_val_constant(uint64_t constant) {
  ir_val* val = calloc_safe(/*nelem=*/1, sizeof(ir_val));
  val->is_constant = true;
  val->val.constant = constant;
  return val;
}

static ir_val* dup_ir_val(ir_val* val) {
  val->ref_count++;
  return val;
}

static void emit_jump(string_view label, array* instructions) {
  ir_instruction* inst = array_push_back(instructions);
  inst->instruction_type = IR_JMP;
  inst->label = label;
}

static void emit_cond_jump(ir_instruction_type jump_type, ir_val* condition,
                           string_view label, array* instructions) {
  ir_instruction* inst = array_push_back(instructions);
  switch (jump_type) {
    case IR_JZ:
    case IR_JNZ:
      inst->instruction_type = jump_type;
      inst->lhs = condition;
      inst->label = label;
      break;
    default:
      error("FATAL: unsupported jump IR.");
  }
}

static void emit_label(string_view label, array* instructions) {
  ir_instruction* inst = array_push_back(instructions);
  inst->instruction_type = IR_LABEL;
  inst->label = label;
}

static void emit_copy(ir_val* src, ir_val* dst, array* instructions) {
  ir_instruction* inst = array_push_back(instructions);
  inst->instruction_type = IR_COPY;
  inst->lhs = src;
  inst->dst = dst;
}

// Forward declaration.
static ir_val* emit_ir_instruction(ast_node*, array*, name_generator*);

static void emit_logical_and_expression(ast_node* node, array* instructions,
                                        name_generator* gen) {
  string_view false_label = name_generator_get_label(gen);
  string_view end_label = name_generator_get_label(gen);
  ir_val* result = create_ir_val_var(gen);

  // <instructions for e1>
  // JumpIfZero(e1, false_label)
  // <instructions for e2>
  // JumpIfZero(e2, false_label)
  // result = 1
  // Jump(end)
  // Label(false_label)
  // result = 0
  // Label(end_label)
  ir_val* e1 =
      emit_ir_instruction(node->node.expression->lhs, instructions, gen);
  emit_cond_jump(IR_JZ, e1, false_label, instructions);
  ir_val* e2 =
      emit_ir_instruction(node->node.expression->rhs, instructions, gen);
  emit_cond_jump(IR_JZ, e2, false_label, instructions);
  emit_copy(create_ir_val_constant(1), result, instructions);
  emit_jump(end_label, instructions);
  emit_label(false_label, instructions);
  emit_copy(create_ir_val_constant(0), result, instructions);
  emit_label(end_label, instructions);
}

static void emit_logical_or_expression(ast_node* node, array* instructions,
                                       name_generator* gen) {
  string_view true_label = name_generator_get_label(gen);
  string_view end_label = name_generator_get_label(gen);
  ir_val* result = create_ir_val_var(gen);

  // <instructions for e1>
  // JumpIfNotZero(e1, true_label)
  // <instructions for e2>
  // JumpIfNotZero(e2, true_label)
  // result = 0
  // Jump(end)
  // Label(true_label)
  // result = 1
  // Label(end_label)
  ir_val* e1 =
      emit_ir_instruction(node->node.expression->lhs, instructions, gen);
  emit_cond_jump(IR_JNZ, e1, true_label, instructions);
  ir_val* e2 =
      emit_ir_instruction(node->node.expression->rhs, instructions, gen);
  emit_cond_jump(IR_JNZ, e2, true_label, instructions);
  emit_copy(create_ir_val_constant(0), result, instructions);
  emit_jump(end_label, instructions);
  emit_label(true_label, instructions);
  emit_copy(create_ir_val_constant(1), result, instructions);
  emit_label(end_label, instructions);
}

//! Assuming `node` is an expression node, emits the IR for the expression, and
//! returns the IR node representing the destination (i.e., final result) of the
//! expression.
static ir_val* emit_expression(ast_node* node, array* instructions,
                               name_generator* gen) {
  ir_val* lhs =
      emit_ir_instruction(node->node.expression->lhs, instructions, gen);
  ir_val* rhs =
      emit_ir_instruction(node->node.expression->rhs, instructions, gen);
  ir_instruction* inst = array_push_back(instructions);
  inst->instruction_type = (rhs == NULL ? IR_UNARY : IR_BINARY);
  inst->op = node->node.expression->op;
  inst->lhs = lhs;
  inst->rhs = rhs;
  inst->dst = create_ir_val_var(gen);
  return dup_ir_val(inst->dst);
}

static ir_val* emit_ir_instruction(ast_node* node, array* instructions,
                                   name_generator* gen) {
  if (!node) {
    return NULL;
  }

  if (node->node_type == AST_VAR) {
    return create_ir_val_var(gen);
  }
  if (node->node_type == AST_CONST) {
    return create_ir_val_constant(node->node.consant->tok->constant.int_val);
  }
  if (node->node_type == AST_EXPR) {
    return emit_expression(node, instructions, gen);
  }

  if (node->node_type == AST_RETSTMNT) {
    ir_val* lhs = emit_ir_instruction(node->node.statement->expression,
                                      instructions, gen);
    ir_instruction* inst = array_push_back(instructions);
    inst->instruction_type = IR_RETURN;
    inst->lhs = lhs;
    return NULL;
  }

  error("Unimplemented");
  return NULL;
}

static ir_func_def* create_ir_func_def(void) {
  ir_func_def* func_def = calloc_safe(/*nelem=*/1, sizeof(ir_func_def));
  func_def->instructions = malloc_safe(sizeof(array));
  array_init(func_def->instructions, sizeof(ir_instruction));
  return func_def;
}

static ir_node* create_ir_node(void) {
  ir_node* node = calloc_safe(/*nelem=*/1, sizeof(ir_node));
  node->function_definition = create_ir_func_def();
  return node;
}

ir_node* emit_ir(ast_node* ast) {
  ir_node* ir = create_ir_node();
  // TODO: hard coded for now. Implement later.
  ir->function_definition->name = "main";
  name_generator gen;
  name_generator_init(&gen);
  emit_ir_instruction(ast, ir->function_definition->instructions, &gen);
  return ir;
}

//! Frees all memory allocated by `ir_val` and its members.
static void destroy_ir_val(ir_val* ir_val) {
  if (!ir_val) {
    return;
  }
  if (ir_val->ref_count > 0) {
    --ir_val->ref_count;
    return;
  }

  if (!ir_val->is_constant) {
    free(ir_val->val.var_name.data);
  }
  free(ir_val);
}

//! Frees memories allocated by `ir_instruction`'s members. Does not free the
//! `ir_instruction` itself.
static void destroy_ir_instruction(ir_instruction* ir_instruction) {
  // Don't free the `ir_instruction` pointer here. It will be released
  // when the array is destroyed.
  destroy_ir_val(ir_instruction->lhs);
  destroy_ir_val(ir_instruction->rhs);
  destroy_ir_val(ir_instruction->dst);
}

//! Frees all memories allocated by `ir_func_def` and its underlying members.
static void destroy_ir_func_def(ir_func_def* ir_func_def) {
  // TODO: free name here. Not doing this for now since name is hard coded.
  array* instructions = ir_func_def->instructions;
  for (size_t i = 0; i < instructions->size; ++i) {
    ir_instruction* inst = array_at(instructions, i);
    destroy_ir_instruction(inst);
  }
  array_destroy(instructions);
  free(instructions);
  free(ir_func_def);
}

void ir_destroy(ir_node* ir_node) {
  destroy_ir_func_def(ir_node->function_definition);
  free(ir_node);
}
