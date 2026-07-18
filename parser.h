#ifndef SKIBICC_PARSER_H
#define SKIBICC_PARSER_H

#include "array.h"
#include "hashmap.h"
#include "lexer.h"
#include "list.h"

//! Represents types of AST nodes.
typedef enum ast_node_type {
  //! Expression.
  AST_EXPR,
  //! Statement.
  AST_STMNT,
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

//! Types of operators. Note that the declaration order is important here:
//! operators are listed top to bottom, in descending precedence.
typedef enum ast_operator_type {
  //! Postfix "++"
  OP_POSTINC,
  //! Postfix "--"
  OP_POSTDEC,
  //! Prefix "++"
  OP_PREINC,
  //! Prefix "--"
  OP_PREDEC,
  //! Unary plus "+"
  OP_POS,
  //! Unary minus "-"
  OP_NEG,
  //! Logical NOT "!"
  OP_NOT,
  //! Bitwise NOT "~"
  OP_BITNOT,
  //! Pointer dereference "*"
  OP_DEREF,
  //! Address of "&"
  OP_ADDROF,
  //! Multiplication "*"
  OP_MUL,
  //! Division "/"
  OP_DIV,
  //! Modulo "%"
  OP_MOD,
  //! Addition "+"
  OP_ADD,
  //! Subtraction "-"
  OP_SUB,
  //! Bit left shift "<<"
  OP_SHL,
  //! Bit right shift ">>"
  OP_SHR,
  //! Relational less than "<"
  OP_LT,
  //! Relational less than or equal to "<="
  OP_LE,
  //! Relational greater than ">"
  OP_GT,
  //! Relational greater than or equal to ">="
  OP_GE,
  //! Relational equal "=="
  OP_EQ,
  //! Relational not equal "!="
  OP_NE,
  //! Bitwise and "&"
  OP_BITAND,
  //! Bitwise xor "^"
  OP_BITXOR,
  //! Bitwise or "|"
  OP_BITOR,
  //! Logical and "&&"
  OP_AND,
  //! Logial or "||"
  OP_OR,
  //! Simple assignment "="
  OP_ASSIGN,
} ast_operator_type;

//! Represents an operator.
typedef struct ast_operator {
  //! Type of the operator.
  ast_operator_type op_type;
  //! Token representing the operator. Must be `TK_PUNCT`.
  token* tok;
} ast_operator;

//! Types of an expression.
typedef enum ast_expression_type {
  //! Nested expression.
  EXPR,
  //! Variable.
  EXPR_VAR,
  //! Constant.
  EXPR_CONST,
} ast_expression_type;

//! Represents a single expression AST node.
typedef struct ast_expression_node {
  //! Operator of the expression. Can be NULL, which means this is a terminal
  //! expression (constant or variable).
  ast_operator* op;
  //! Left hand side of the expression. Always populated.
  struct ast_expression* lhs;
  //! Right hand side of the expression. Populated if `op` is a binary operator.
  struct ast_expression* rhs;
} ast_expression_node;

//! Represents an AST for a full expression.
typedef struct ast_expression {
  //! Type of the expression.
  ast_expression_type type;

  union {
    //! Expression (nested). Corresponds to `EXPR`.
    struct ast_expression_node* expression;
    //! Constant. Corresponds to `EXPR_CONST`.
    ast_constant* consant;
    //! Variable. Corresponds to `EXPR_VAR`.
    ast_variable* variable;
  } node;
} ast_expression;

// Most basic declaration:
// int i;
// int i = 123 + 456;
typedef struct ast_declaration {
  // Identifier token.
  token* identifier;
  // Optional assignment expression.
  ast_expression* expression;
} ast_declaration;

//! Types of block items within a statement.
typedef enum ast_statement_item_type {
  //! Declaration.
  STMT_DECL,
  //! Expression statement item.
  STMT_EXPR,
  //! Return statement item.
  STMT_RET,
  //! Nested statement.
  STMT_NESTED,
} ast_statement_item_type;

//! Block item within a statement.
typedef struct ast_statement_item {
  //! Type of the block item.
  ast_statement_item_type type;

  //! Only used if `type` is `STMT_DECL`.
  ast_declaration* declaration;
  //! Only used if `type` is `STMT_EXPR` or `STMT_RET`.
  ast_expression* expression;
  //! Only used if `type` is `STMT_NESTED`.
  struct ast_statement* statement;
} ast_statement_item;

//! Represents a statement.
typedef struct ast_statement {
  //! Block items within a statement. Must be of type `ast_statement_item`.
  array items;
} ast_statement;

struct ast_node {
  ast_node_type node_type;
  union {
    ast_expression* expression;
    ast_statement* statement;
  } node;
};

typedef struct scope {
  hashmap var_map;
} scope;

typedef struct parser {
  array* tokens;
  size_t cur;
  //! A LIFO stack of scopes. The most recent scope is at the top.
  list* scopes;
} parser;

ast_node* parse(array* tokens);

void ast_destroy(ast_node* node);

#endif  // SKIBICC_PARSER_H
