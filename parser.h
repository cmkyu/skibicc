#ifndef SKIBICC_PARSER_H
#define SKIBICC_PARSER_H

#include "array.h"
#include "lexer.h"

typedef enum ast_node_type {
  AST_EXPR,
  AST_RETSTMNT,
  AST_VAR,
  AST_CONST,
} ast_node_type;

typedef struct ast_node ast_node;

typedef struct ast_constant {
  // Must be TK_FCONST, TK_ICONST or TK_STRLIT
  token* tok;
} ast_constant;

typedef struct ast_variable {
  // Must be TK_IDENT
  token* tok;
} ast_variable;

typedef enum ast_operator_type {
  // Postfix "++"
  OP_POSTINC,
  // Postfix "--"
  OP_POSTDEC,
  // Pointer dereference "*"
  OP_DEREF,
  // Address of "&"
  OP_ADDROF,
  // Unary plus "+"
  OP_POS,
  // Unary minus "-"
  OP_NEG,
  // Logical NOT "!"
  OP_NOT,
  // Bitwise NOT "~"
  OP_BITNOT,
  // Prefix "++"
  OP_PREINC,
  // Prefix "--"
  OP_PREDEC,
} ast_operator_type;

typedef struct ast_operator {
  ast_operator_type op_type;
  // Must be TK_PUNCT
  token* tok;
} ast_operator;

typedef struct ast_expression {
  ast_operator* op;
  // Must be `AST_EXPR`, `AST_CONST` or `AST_VAR`. Always populated.
  struct ast_node* lhs;
  // Must be `AST_EXPR`, `AST_CONST` or `AST_VAR`. Populated if `op` is not NULL
  // and is a binary operator.
  struct ast_node* rhs;
} ast_expression;

typedef struct ast_statement {
  ast_node* expression;
} ast_statement;

struct ast_node {
  ast_node_type node_type;
  union {
    ast_expression* expression;
    ast_statement* statement;
    ast_constant* consant;
    ast_variable* variable;
  } node;
};

typedef struct parser {
  array* tokens;
  size_t cur;
  ast_node* ast;
} parser;

ast_node* parse(array* tokens);

void ast_destroy(ast_node* node);

#endif  // SKIBICC_PARSER_H
