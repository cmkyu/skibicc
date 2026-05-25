#include "parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "array.h"
#include "errors.h"
#include "lexer.h"

// Forward declarations.
static ast_node* parse_unary_expression(parser*);
ast_node* parse_expression(parser*);

//! Returns true if `tok`'s string representation matches `expected`. Otherwise
//! returns false.
static bool is_token_string_match(token* tok, const char* expected) {
  size_t len = strlen(expected);
  if (len != tok->size) {
    return false;
  }
  if (strncmp(tok->loc, expected, len) != 0) {
    return false;
  }
  return true;
}

//! Returns true if `tok` is a punctuator token with string representation
//! `expected`.
static bool is_punctuator_token(token* tok, const char* expected) {
  if (tok->token_type != TK_PUNCT) {
    return false;
  }
  return is_token_string_match(tok, expected);
}

//! Returns true if `tok` is a keyword token with string representation
//! `expected`.
static bool is_keyword_token(token* tok, const char* expected) {
  if (tok->token_type != TK_KEYWRD) {
    return false;
  }
  return is_token_string_match(tok, expected);
}

//! Returns the current token the cursor is pointing to.
static token* peek_token(parser* parser) {
  return array_at(parser->tokens, parser->cur);
}

//! Consumes the current token. Moves the cursor to the next token.
static inline void consume_token(parser* parser) { parser->cur++; }

//! Returns true if there are still unconumed tokens. Otherwise returns false.
static inline bool has_token(parser* parser) {
  return parser->cur < parser->tokens->size;
}

//! Emits an error message and exits if `actual` token does not have the
//! `expected_type` or does not have a string representation that is
//! `expecte_tok`.
static void check_token(token* actual, token_type expected_type,
                        const char* expecte_tok) {
  if (actual->token_type != expected_type ||
      !is_token_string_match(actual, expecte_tok)) {
    error_tok_fmt(actual, "Expected a \"%s\".", expecte_tok);
  }
}

//! Consumes the current token if it is a keyword token. Otherwise emits an
//! error message and exits.
static void consume_keyword(parser* parser, const char* expecte_tok) {
  token* tok = peek_token(parser);
  check_token(tok, TK_KEYWRD, expecte_tok);
  consume_token(parser);
}

//! Consumes the current token if it is a punctuator token. Otherwise emits an
//! error message and exits.
static void consume_punctuator(parser* parser, const char* expecte_tok) {
  token* tok = peek_token(parser);
  check_token(tok, TK_PUNCT, expecte_tok);
  consume_token(parser);
}

static void consume_any_literal(parser* parser) {
  token* tok = peek_token(parser);
  if (tok->token_type != TK_ICONST && tok->token_type != TK_FCONST &&
      tok->token_type != TK_STRLIT) {
    error_tok_fmt(tok, "Expected a constant or a literal.");
  }
  consume_token(parser);
}

static void consume_any_identifier(parser* parser) {
  token* tok = peek_token(parser);
  if (tok->token_type != TK_IDENT) {
    error_tok_fmt(tok, "Expected an identifier.");
  }
  consume_token(parser);
}

static ast_node* parse_type_name(parser* parser) {
  // TODO: Implement this. Probably need to look ahead tokens without consuming
  // them.
  return NULL;
}

// Incomplete. Right now just delegates to parse_unary_expression.
static ast_node* parse_cast_expression(parser* parser) {
  token* tok = peek_token(parser);
  if (is_punctuator_token(tok, "(") && parse_type_name(parser)) {
    return parse_cast_expression(parser);
  }
  return parse_unary_expression(parser);
}

//! Retruns an expression AST node.
static ast_node* create_ast_expression(void) {
  ast_node* node = calloc_safe(/*nelem=*/1, sizeof(ast_node));
  node->node_type = AST_EXPR;
  ast_expression* expression = calloc_safe(1, sizeof(ast_expression));
  node->node.expression = expression;
  return node;
}

//! Retruns a constant AST node.
static ast_node* create_ast_constant(token* tok) {
  ast_node* node = calloc_safe(/*nelem=*/1, sizeof(ast_node));
  node->node_type = AST_CONST;
  ast_constant* constant = calloc_safe(/*nelem=*/1, sizeof(ast_constant));
  constant->tok = tok;
  node->node.consant = constant;
  return node;
}

//! Retruns a variable AST node.
static ast_node* create_ast_variable(token* tok) {
  ast_node* node = calloc_safe(/*nelem=*/1, sizeof(ast_node));
  node->node_type = AST_VAR;
  ast_variable* variable = calloc_safe(/*nelem=*/1, sizeof(ast_variable));
  variable->tok = tok;
  node->node.variable = variable;
  return node;
}

//! Parses a primary expression and returns an AST node representing it. A
//! primary expression is one of the following:
//!
//! - constant or string literal. Returns a constant AST node.
//! - identifier. Returns a variable AST node.
//! - ( expression ). Returns an expression AST node.
//!
//! Returns NULL no primary expression can be parsed.
static ast_node* parse_primary_expression(parser* parser) {
  token* tok = peek_token(parser);
  token_type tok_type = tok->token_type;
  // Constant or string literal.
  if (tok_type == TK_ICONST || tok_type == TK_FCONST || tok_type == TK_STRLIT) {
    consume_token(parser);
    return create_ast_constant(tok);
  }
  // Identifier.
  if (tok_type == TK_IDENT) {
    consume_token(parser);
    return create_ast_variable(tok);
  }

  // ( expression )
  if (is_punctuator_token(tok, "(")) {
    consume_token(parser);
    ast_node* node = parse_expression(parser);
    consume_punctuator(parser, ")");
    return node;
  }
  // TODO: implement generic selection parsing.
  error_tok(tok);
  return NULL;
}

//! Returns an operator AST node of `op_type` from `tok`.
static ast_operator* create_ast_operator(token* tok,
                                         ast_operator_type op_type) {
  ast_operator* op = calloc_safe(/*nelem=*/1, sizeof(ast_operator));
  op->tok = tok;
  op->op_type = op_type;
  return op;
}

//! Parses a postfix operator from `tok`. If successful, consumes `tok` and
//! returns an expression AST node containing the operator. Returns NULL if no
//! postfix operator can be parsed from `tok`.
static ast_node* parse_postfix_operator(parser* parser, token* tok) {
  if (tok->token_type != TK_PUNCT) {
    return NULL;
  }

  ast_operator* op = NULL;
  if (is_token_string_match(tok, "++")) {
    op = create_ast_operator(tok, OP_POSTINC);
  } else if (is_token_string_match(tok, "--")) {
    op = create_ast_operator(tok, OP_POSTDEC);
  }

  if (op) {
    consume_token(parser);
    ast_node* node = create_ast_expression();
    node->node.expression->op = op;
    return node;
  }
  return NULL;
}

static ast_node* parse_postfix_expression(parser* parser) {
  // TODO: handle compound struct literal ( type-name ) { initializer-list }

  ast_node* node = parse_primary_expression(parser);
  while (has_token(parser)) {
    token* tok = peek_token(parser);
    // postfix-expression [ expression ]
    if (is_punctuator_token(tok, "[")) {
      consume_token(parser);
      parse_expression(parser);
      consume_punctuator(parser, "]");
      continue;
    }

    // postfix-expression ( argument-expression-list_opt )
    if (is_punctuator_token(tok, "(")) {
      consume_token(parser);
      tok = peek_token(parser);
      if (is_punctuator_token(tok, ")")) {
        consume_token(parser);
        continue;
      }
      parse_expression(parser);
      consume_punctuator(parser, ")");
      continue;
    }

    // postfix-expression . identifier
    // postfix-expression -> identifier
    if (is_punctuator_token(tok, ".") || is_punctuator_token(tok, "->")) {
      consume_token(parser);
      consume_any_identifier(parser);
      continue;
    }

    // postfix-expression ++
    // postfix-expression --
    ast_node* new_node = parse_postfix_operator(parser, tok);
    if (new_node) {
      new_node->node.expression->lhs = node;
      node = new_node;
      continue;
    }
    // No matching token - no more postfix expression to parse.
    break;
  }
  return node;
}

//! Parses a prefix operator from `tok`. If successful, consumes `tok` and
//! returns an expression AST node containing the operator. Returns NULL if no
//! prefix operator can be parsed from `tok`.
static ast_node* parse_prefix_operator(parser* parser, token* tok) {
  if (tok->token_type != TK_PUNCT) {
    return NULL;
  }

  ast_operator* op = NULL;
  if (is_token_string_match(tok, "++")) {
    op = create_ast_operator(tok, OP_PREINC);
  } else if (is_token_string_match(tok, "--")) {
    op = create_ast_operator(tok, OP_PREDEC);
  }

  if (op) {
    consume_token(parser);
    ast_node* res = create_ast_expression();
    res->node.expression->op = op;
    return res;
  }
  return NULL;
}

//! Parses an unary operator from `tok`. If successful, consumes `tok` and
//! returns an expression AST node containing the operator. Returns NULL if no
//! unary operator can be parsed from `tok`.
static ast_node* parse_unary_operator(parser* parser, token* tok) {
  if (tok->token_type != TK_PUNCT) {
    return NULL;
  }

  ast_operator* op = NULL;
  if (is_token_string_match(tok, "*")) {
    op = create_ast_operator(tok, OP_DEREF);
  } else if (is_token_string_match(tok, "&")) {
    op = create_ast_operator(tok, OP_ADDROF);
  } else if (is_token_string_match(tok, "+")) {
    op = create_ast_operator(tok, OP_POS);
  } else if (is_token_string_match(tok, "-")) {
    op = create_ast_operator(tok, OP_NEG);
  } else if (is_token_string_match(tok, "!")) {
    op = create_ast_operator(tok, OP_NOT);
  } else if (is_token_string_match(tok, "~")) {
    op = create_ast_operator(tok, OP_BITNOT);
  }

  if (op) {
    consume_token(parser);
    ast_node* res = create_ast_expression();
    res->node.expression->op = op;
    return res;
  }
  return NULL;
}

static ast_node* parse_unary_expression(parser* parser) {
  token* tok = peek_token(parser);
  ast_node* node = parse_prefix_operator(parser, tok);
  if (node) {
    ast_node* lhs = parse_unary_expression(parser);
    node->node.expression->lhs = lhs;
    return node;
  }

  node = parse_unary_operator(parser, tok);
  if (node) {
    ast_node* lhs = parse_cast_expression(parser);
    node->node.expression->lhs = lhs;
    return node;
  }

  if (is_keyword_token(tok, "sizeof")) {
    consume_token(parser);
    tok = peek_token(parser);
    if (is_punctuator_token(tok, "(")) {
      consume_token(parser);
      parse_type_name(parser);
      consume_punctuator(parser, ")");
      return NULL;
    }
    return parse_unary_expression(parser);
  }
  // TODO: Handle _Alignof

  return parse_postfix_expression(parser);
}

ast_node* parse_expression(parser* parser) {
  return parse_unary_expression(parser);
}

static ast_node* create_statement(ast_node_type node_type) {
  ast_node* node = calloc_safe(/*nelem=*/1, sizeof(ast_node));
  node->node_type = node_type;
  ast_statement* statement = calloc_safe(/*nelem=*/1, sizeof(ast_statement));
  node->node.statement = statement;
  return node;
}

static ast_node* parse_statement(parser* parser) {
  consume_keyword(parser, "return");

  ast_node* expression = parse_expression(parser);

  consume_punctuator(parser, ";");

  ast_node* node = create_statement(AST_RETSTMNT);
  node->node.statement->expression = expression;
  return node;
}

static ast_node* parse_function_definition(parser* parser) {
  consume_keyword(parser, "int");
  consume_any_identifier(parser);

  consume_punctuator(parser, "(");
  consume_keyword(parser, "void");
  consume_punctuator(parser, ")");

  consume_punctuator(parser, "{");
  ast_node* node = parse_statement(parser);
  consume_punctuator(parser, "}");
  return node;
}

ast_node* parse(array* tokens) {
  parser parser;
  parser.cur = 0;
  parser.tokens = tokens;

  return parse_function_definition(&parser);
}

// Forward declarations.
void ast_destroy(ast_node*);

static void destroy_ast_expression(ast_expression* expr) {
  ast_destroy(expr->lhs);
  ast_destroy(expr->rhs);
  free(expr->op);
  free(expr);
}

static void destroy_ast_statement(ast_statement* stmt) {
  ast_destroy(stmt->expression);
  free(stmt);
}

static void destroy_ast_variable(ast_variable* var) { free(var); }

static void destroy_ast_constant(ast_constant* constant) { free(constant); }

void ast_destroy(ast_node* node) {
  if (!node) {
    return;
  }

  switch (node->node_type) {
    case AST_EXPR:
      destroy_ast_expression(node->node.expression);
      break;
    case AST_RETSTMNT:
      destroy_ast_statement(node->node.statement);
      break;
    case AST_VAR:
      destroy_ast_variable(node->node.variable);
      break;
    case AST_CONST:
      destroy_ast_constant(node->node.consant);
      break;
  }
  free(node);
}
