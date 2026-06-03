#include "ir.h"

#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "array.h"
#include "errors.h"
#include "parser.h"

static uint64_t COUNT = 0;

static char* generate_name(void) {
  // TODO: use string_view here.
  char* name;
  size_t size;
  FILE* f = open_memstream(&name, &size);
  if (!f) {
    error("FATAL: generate_name(): open_memstream() failed.");
  }
  ++COUNT;
  // Names are formatted like 1_, 2_, 3_,...
  fprintf(f, "%" PRIu64 "_", COUNT);
  fclose(f);
  return name;
}

static ir_val* create_ir_val_var(void) {
  ir_val* val = calloc_safe(/*nelem=*/1, sizeof(ir_val));
  val->is_constant = false;
  val->val.var_name = generate_name();
  return val;
}

static ir_val* create_ir_val_constant(ast_node* node) {
  ir_val* val = calloc_safe(/*nelem=*/1, sizeof(ir_val));
  val->is_constant = true;
  val->val.constant = node;
  return val;
}

static ir_val* dup_ir_val(ir_val* val) {
  val->ref_count++;
  return val;
}

ir_val* emit_ir_instruction(ast_node* node, array* instructions) {
  if (node->node_type == AST_VAR) {
    return create_ir_val_var();
  }
  if (node->node_type == AST_CONST) {
    return create_ir_val_constant(node);
  }

  if (node->node_type == AST_EXPR) {
    // Only unary ops for now. Only recurse on lhs.
    ir_val* lhs = emit_ir_instruction(node->node.expression->lhs, instructions);
    ir_instruction* inst = array_push_back(instructions);
    inst->instruction_type = IR_ARITH;
    inst->op = node->node.expression->op;
    inst->lhs = lhs;
    inst->dst = create_ir_val_var();
    return dup_ir_val(inst->dst);
  }

  if (node->node_type == AST_RETSTMNT) {
    ir_val* lhs =
        emit_ir_instruction(node->node.statement->expression, instructions);
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
  emit_ir_instruction(ast, ir->function_definition->instructions);
  return ir;
}

static void destroy_ir_val(ir_val* ir_val) {
  if (!ir_val) {
    return;
  }
  if (ir_val->ref_count > 0) {
    --ir_val->ref_count;
    return;
  }

  if (!ir_val->is_constant) {
    free(ir_val->val.var_name);
  }
  free(ir_val);
}

static void destroy_ir_instruction(ir_instruction* ir_instruction) {
  // Don't free the `ir_instruction` pointer here. It will be released
  // when the array is destroyed.
  destroy_ir_val(ir_instruction->lhs);
  destroy_ir_val(ir_instruction->rhs);
  destroy_ir_val(ir_instruction->dst);
}

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
