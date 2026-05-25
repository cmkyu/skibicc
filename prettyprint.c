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
static void print_ast_variable(ast_node* ast, size_t depth) {
  token* tok = ast->node.variable->tok;
  char* str = strndup(tok->loc, tok->size);
  print_tabs(depth);
  printf("(Variable: %s)\n", str);
  free(str);
}

//! Prints a constant `ast` node. `depth` is the depth of the node within the
//! whole AST.
static void print_ast_constant(ast_node* ast, size_t depth) {
  token* tok = ast->node.consant->tok;
  char* str = strndup(tok->loc, tok->size);
  print_tabs(depth);
  printf("(Identifier: %s)\n", str);
  free(str);
}

// Forward declaration.
static void prettyprint_ast_internal(ast_node*, size_t);

//! Prints an expression `ast` node. `depth` is the depth of the node within the
//! whole AST.
static void print_ast_expression(ast_node* ast, size_t depth) {
  print_tabs(depth);
  printf("(Expression, op: ");
  ast_expression* expression = ast->node.expression;
  // Print the operator
  token* op_tok = expression->op->tok;
  char* str = strndup(op_tok->loc, op_tok->size);
  printf("%s,\n", str);
  free(str);

  print_tabs(depth);
  printf(" lhs: \n");
  prettyprint_ast_internal(expression->lhs, depth + 1);
  if (expression->rhs) {
    print_tabs(depth);
    printf(" rhs: \n");
    prettyprint_ast_internal(expression->rhs, depth + 1);
  }

  print_tabs(depth);
  printf(")\n");
}

// TODO: Temporary. AST_RETSTMNT is probably going away soon.
static void prettyprint_ast_return_statement(ast_node* node, size_t depth) {
  print_tabs(depth);
  printf("(Return: \n");

  prettyprint_ast_internal(node->node.statement->expression, depth + 1);

  print_tabs(depth);
  printf(")\n");
}

//! Helper method that implements the actual meat of the prettyprint function.
//! Prints the `ast` node. `depth` is the node's depth within the entire AST.
static void prettyprint_ast_internal(ast_node* ast, size_t depth) {
  switch (ast->node_type) {
    case AST_RETSTMNT:
      prettyprint_ast_return_statement(ast, depth);
      break;
    case AST_EXPR:
      print_ast_expression(ast, depth);
      break;
    case AST_VAR:
      print_ast_variable(ast, depth);
      break;
    case AST_CONST:
      print_ast_constant(ast, depth);
      break;
  }
}

void prettyprint_ast(ast_node* ast) {
  prettyprint_ast_internal(ast, /*depth=*/0);
}

static void prettyprint_ir_val(ir_val* val) {
  if (val->is_constant) {
    token* tok = val->val.constant->node.consant->tok;
    char* str = strndup(tok->loc, tok->size);
    printf("constant(%s)", str);
    free(str);
    return;
  }
  printf("var(%s)", val->val.var_name);
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

static void prettyprint_ir_instructions(array* instructions) {
  size_t sz = instructions->size;
  for (size_t i = 0; i < sz; ++i) {
    printf("\t");
    ir_instruction* inst = array_at(instructions, i);
    if (inst->instruction_type == IR_ARITH) {
      prettyprint_ir_arithmetic_instruction(inst);
    } else if (inst->instruction_type == IR_RETURN) {
      prettyprint_ir_return_instruction(inst);
    } else {
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
