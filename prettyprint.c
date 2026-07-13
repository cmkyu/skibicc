#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "array.h"
#include "errors.h"
#include "ir.h"
#include "lexer.h"
#include "parser.h"

//! Prints `depth` number of tab characters.
static void print_tabs(size_t depth) {
  for (size_t i = 0; i < depth; ++i) {
    printf("\t");
  }
}

//! Prints a variable `ast` node. `depth` is the depth of the node within the
//! whole AST.
static void print_ast_variable(ast_variable* ast, size_t depth) {
  token* tok = ast->tok;
  char* str = strndup(tok->loc, tok->size);
  print_tabs(depth);
  printf("(Variable: %s)\n", str);
  free(str);
}

//! Prints a constant `ast` node. `depth` is the depth of the node within the
//! whole AST.
static void print_ast_constant(ast_constant* ast, size_t depth) {
  token* tok = ast->tok;
  char* str = strndup(tok->loc, tok->size);
  print_tabs(depth);
  printf("(Constant: %s)\n", str);
  free(str);
}

// Forward declaration.
static void prettyprint_ast_expression(ast_expression*, size_t);

//! Prints an expression `ast` node. `depth` is the depth of the node within the
//! whole AST.
static void print_ast_expression_node(ast_expression_node* ast, size_t depth) {
  print_tabs(depth);
  printf("(Expression, op: ");
  // Print the operator
  token* op_tok = ast->op->tok;
  char* str = strndup(op_tok->loc, op_tok->size);
  printf("%s,\n", str);
  free(str);

  print_tabs(depth);
  printf(" lhs: \n");
  prettyprint_ast_expression(ast->lhs, depth + 1);
  if (ast->rhs) {
    print_tabs(depth);
    printf(" rhs: \n");
    prettyprint_ast_expression(ast->rhs, depth + 1);
  }

  print_tabs(depth);
  printf(")\n");
}

// TODO: Temporary.
static void prettyprint_ast_return_statement(ast_statement* statement,
                                             size_t depth) {
  print_tabs(depth);
  printf("(Return: \n");

  ast_statement_item* item = array_at(&statement->items, 0);
  prettyprint_ast_expression(item->expression, depth + 1);

  print_tabs(depth);
  printf(")\n");
}

//! Prints the `ast` expression. `depth` is the node's depth within the entire
//! AST.
static void prettyprint_ast_expression(ast_expression* ast, size_t depth) {
  switch (ast->type) {
    case EXPR:
      print_ast_expression_node(ast->node.expression, depth);
      break;
    case EXPR_VAR:
      print_ast_variable(ast->node.variable, depth);
      break;
    case EXPR_CONST:
      print_ast_constant(ast->node.consant, depth);
      break;
  }
}

//! Helper method that implements the actual meat of printing the `ast` node.
static void prettyprint_ast_internal(ast_node* ast) {
  switch (ast->node_type) {
    case AST_STMNT:
      prettyprint_ast_return_statement(ast->node.statement, /*depth=*/0);
      break;
    case AST_EXPR:
      prettyprint_ast_expression(ast->node.expression, /*depth=*/0);
      break;
  }
}

void prettyprint_ast(ast_node* ast) { prettyprint_ast_internal(ast); }

static void prettyprint_ir_val(ir_val* val) {
  if (val->is_constant) {
    printf("constant(%zu)", val->val.constant);
    return;
  }
  printf("var(%s)", val->val.var_name.data);
}

static void prettyprint_ir_arithmetic_instruction(ir_instruction* inst) {
  token* op_tok = inst->op->tok;
  char* str = strndup(op_tok->loc, op_tok->size);
  printf("op(%s)", str);
  free(str);

  if (inst->lhs) {
    printf(", ");
    prettyprint_ir_val(inst->lhs);
  }
  if (inst->rhs) {
    printf(", ");
    prettyprint_ir_val(inst->rhs);
  }
  if (inst->dst) {
    printf(", ");
    prettyprint_ir_val(inst->dst);
  }
}

static void prettyprint_ir_return_instruction(ir_instruction* inst) {
  printf("return ");
  if (inst->lhs) {
    prettyprint_ir_val(inst->lhs);
  }
}

static void prettyprint_ir_copy_instruction(ir_instruction* inst) {
  printf("copy ");
  prettyprint_ir_val(inst->lhs);
  printf(", ");
  prettyprint_ir_val(inst->dst);
}

static void prettyprint_ir_jmp_instruction(ir_instruction* inst) {
  printf("jmp %s", inst->label.data);
}

static void prettyprint_ir_jz_instruction(ir_instruction* inst) {
  printf("jz ");
  prettyprint_ir_val(inst->lhs);
  printf(", @%s", inst->label.data);
}

static void prettyprint_ir_jnz_instruction(ir_instruction* inst) {
  printf("jnz ");
  prettyprint_ir_val(inst->lhs);
  printf(", @%s", inst->label.data);
}

static void prettyprint_ir_label(ir_instruction* inst) {
  printf("%s: ", inst->label.data);
}

static void prettyprint_ir_instructions(array* instructions) {
  size_t sz = instructions->size;
  for (size_t i = 0; i < sz; ++i) {
    printf("\t");
    ir_instruction* inst = array_at(instructions, i);
    ir_instruction_type inst_type = inst->instruction_type;
    switch (inst_type) {
      case IR_RETURN:
        prettyprint_ir_return_instruction(inst);
        break;
      case IR_UNARY:
      case IR_BINARY:
        prettyprint_ir_arithmetic_instruction(inst);
        break;
      case IR_COPY:
        prettyprint_ir_copy_instruction(inst);
        break;
      case IR_JMP:
        prettyprint_ir_jmp_instruction(inst);
        break;
      case IR_JZ:
        prettyprint_ir_jz_instruction(inst);
        break;
      case IR_JNZ:
        prettyprint_ir_jnz_instruction(inst);
        break;
      case IR_LABEL:
        prettyprint_ir_label(inst);
        break;
      default:
        error("Unknown instruction type: %d", inst->instruction_type);
    }
    printf("\n");
  }
}

static void prettyprint_ir_func_def(ir_func_def* func_def) {
  printf("func_def(%s): \n", func_def->name);
  prettyprint_ir_instructions(func_def->instructions);
}

void prettyprint_ir(ir_node* ir) {
  prettyprint_ir_func_def(ir->function_definition);
}
