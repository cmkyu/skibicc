#include "parser.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "array.h"
#include "errors.h"
#include "lexer.h"

// Forward declarations.
static ast_expression* parse_unary_expression(parser*);
ast_expression* parse_expression(parser*);

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

//! Consumes the current token if it is an identifier token and returns the
//! token. Otherwise emits an error message and exits.
static token* consume_identifier(parser* parser) {
  token* tok = peek_token(parser);
  if (tok->token_type != TK_IDENT) {
    error_tok_fmt(tok, "Expected an identifier.");
  }
  consume_token(parser);
  return tok;
}

static ast_node* parse_type_name(parser* parser) {
  // TODO: Implement this. Probably need to look ahead tokens without consuming
  // them.
  return NULL;
}

// Incomplete. Right now just delegates to parse_unary_expression.
static ast_expression* parse_cast_expression(parser* parser) {
  token* tok = peek_token(parser);
  if (is_punctuator_token(tok, "(") && parse_type_name(parser)) {
    return parse_cast_expression(parser);
  }
  return parse_unary_expression(parser);
}

//! Retruns a nested expression AST expression node.
static ast_expression* create_ast_expression(void) {
  ast_expression* expr = calloc_safe(/*nelem=*/1, sizeof(ast_expression));
  expr->type = EXPR;
  ast_expression_node* expression = calloc_safe(1, sizeof(ast_expression_node));
  expr->node.expression = expression;
  return expr;
}

//! Retruns a constant AST expression node.
static ast_expression* create_ast_constant(token* tok) {
  ast_expression* expr = calloc_safe(/*nelem=*/1, sizeof(ast_expression));
  expr->type = EXPR_CONST;
  ast_constant* constant = calloc_safe(/*nelem=*/1, sizeof(ast_constant));
  constant->tok = tok;
  expr->node.consant = constant;
  return expr;
}

//! Retruns a variable AST expression node.
static ast_expression* create_ast_variable(token* tok) {
  ast_expression* expr = calloc_safe(/*nelem=*/1, sizeof(ast_expression));
  expr->type = EXPR_VAR;
  ast_variable* variable = calloc_safe(/*nelem=*/1, sizeof(ast_variable));
  variable->tok = tok;
  expr->node.variable = variable;
  return expr;
}

//! Parses a primary expression and returns an AST node representing it. A
//! primary expression is one of the following:
//!
//! - constant or string literal. Returns a constant AST node.
//! - identifier. Returns a variable AST node.
//! - ( expression ). Returns an expression AST node.
//!
//! Returns NULL no primary expression can be parsed.
static ast_expression* parse_primary_expression(parser* parser) {
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
    ast_expression* node = parse_expression(parser);
    consume_punctuator(parser, ")");
    return node;
  }
  // TODO: implement generic selection parsing.
  error_tok_fmt(tok, "parse_primary_expression(): unexpected token.");
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
static ast_expression* parse_postfix_operator(parser* parser, token* tok) {
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
    ast_expression* expr = create_ast_expression();
    expr->node.expression->op = op;
    return expr;
  }
  return NULL;
}

static ast_expression* parse_postfix_expression(parser* parser) {
  // TODO: handle compound struct literal ( type-name ) { initializer-list }

  ast_expression* node = parse_primary_expression(parser);
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
      consume_identifier(parser);
      continue;
    }

    // postfix-expression ++
    // postfix-expression --
    ast_expression* new_node = parse_postfix_operator(parser, tok);
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
static ast_expression* parse_prefix_operator(parser* parser, token* tok) {
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
    ast_expression* res = create_ast_expression();
    res->node.expression->op = op;
    return res;
  }
  return NULL;
}

//! Parses an unary operator from `tok`. If successful, consumes `tok` and
//! returns an expression AST node containing the operator. Returns NULL if no
//! unary operator can be parsed from `tok`.
static ast_expression* parse_unary_operator(parser* parser, token* tok) {
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
    ast_expression* res = create_ast_expression();
    res->node.expression->op = op;
    return res;
  }
  return NULL;
}

static ast_expression* parse_unary_expression(parser* parser) {
  token* tok = peek_token(parser);
  ast_expression* node = parse_prefix_operator(parser, tok);
  if (node) {
    ast_expression* lhs = parse_unary_expression(parser);
    node->node.expression->lhs = lhs;
    return node;
  }

  node = parse_unary_operator(parser, tok);
  if (node) {
    ast_expression* lhs = parse_cast_expression(parser);
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

//! If `tok` is a binary operator, populates `op_type` with the binary operator
//! type represented by `tok` and returns true. Otherwise, returns false.
static bool get_binary_op_type(token* tok, ast_operator_type* op_type) {
  if (tok->token_type != TK_PUNCT) {
    return false;
  }

  if (is_token_string_match(tok, "*")) {
    *op_type = OP_MUL;
    return true;
  } else if (is_token_string_match(tok, "/")) {
    *op_type = OP_DIV;
    return true;
  } else if (is_token_string_match(tok, "+")) {
    *op_type = OP_ADD;
    return true;
  } else if (is_token_string_match(tok, "-")) {
    *op_type = OP_SUB;
    return true;
  } else if (is_token_string_match(tok, "%")) {
    *op_type = OP_MOD;
    return true;
  } else if (is_token_string_match(tok, "<<")) {
    *op_type = OP_SHL;
    return true;
  } else if (is_token_string_match(tok, ">>")) {
    *op_type = OP_SHR;
    return true;
  } else if (is_token_string_match(tok, "<")) {
    *op_type = OP_LT;
    return true;
  } else if (is_token_string_match(tok, "<=")) {
    *op_type = OP_LE;
    return true;
  } else if (is_token_string_match(tok, ">")) {
    *op_type = OP_GT;
    return true;
  } else if (is_token_string_match(tok, ">=")) {
    *op_type = OP_GE;
    return true;
  } else if (is_token_string_match(tok, "==")) {
    *op_type = OP_EQ;
    return true;
  } else if (is_token_string_match(tok, "!=")) {
    *op_type = OP_NE;
    return true;
  } else if (is_token_string_match(tok, "&")) {
    *op_type = OP_BITAND;
    return true;
  } else if (is_token_string_match(tok, "^")) {
    *op_type = OP_BITXOR;
    return true;
  } else if (is_token_string_match(tok, "|")) {
    *op_type = OP_BITOR;
    return true;
  } else if (is_token_string_match(tok, "&&")) {
    *op_type = OP_AND;
    return true;
  } else if (is_token_string_match(tok, "||")) {
    *op_type = OP_OR;
    return true;
  } else if (is_token_string_match(tok, "=")) {
    *op_type = OP_ASSIGN;
    return true;
  }
  return false;
}

//! Returns a binary expression whose operator is of `op_type` from `tok`. The
//! caller is responsible for populating in the left handside and the right
//! handside of the expression.
static ast_expression* create_binary_expression(ast_operator_type op_type,
                                                token* tok) {
  ast_operator* op = create_ast_operator(tok, op_type);
  ast_expression* res = create_ast_expression();
  res->node.expression->op = op;
  return res;
}

//! Maps `ast_operator_type` to their corresponding precedence.
static uint64_t* PRECEDENCE = NULL;

//! Initializes `PRECEDENCE`. Bigger number means higher precedence.
static void init_precedence(void) {
  PRECEDENCE = malloc_safe(50 * sizeof(uint64_t));

  PRECEDENCE[OP_POSTINC] = 15;
  PRECEDENCE[OP_POSTDEC] = 15;

  PRECEDENCE[OP_PREINC] = 14;
  PRECEDENCE[OP_PREDEC] = 14;
  PRECEDENCE[OP_POS] = 14;
  PRECEDENCE[OP_NEG] = 14;
  PRECEDENCE[OP_NOT] = 14;
  PRECEDENCE[OP_BITNOT] = 14;
  PRECEDENCE[OP_DEREF] = 14;
  PRECEDENCE[OP_ADDROF] = 14;

  PRECEDENCE[OP_MUL] = 13;
  PRECEDENCE[OP_DIV] = 13;
  PRECEDENCE[OP_MOD] = 13;

  PRECEDENCE[OP_ADD] = 12;
  PRECEDENCE[OP_SUB] = 12;

  PRECEDENCE[OP_SHL] = 11;
  PRECEDENCE[OP_SHR] = 11;

  PRECEDENCE[OP_LT] = 10;
  PRECEDENCE[OP_LE] = 10;
  PRECEDENCE[OP_GT] = 10;
  PRECEDENCE[OP_GE] = 10;

  PRECEDENCE[OP_EQ] = 9;
  PRECEDENCE[OP_NE] = 9;

  PRECEDENCE[OP_BITAND] = 8;
  PRECEDENCE[OP_BITXOR] = 7;
  PRECEDENCE[OP_BITOR] = 6;
  PRECEDENCE[OP_AND] = 5;
  PRECEDENCE[OP_OR] = 4;

  PRECEDENCE[OP_ASSIGN] = 2;
}

//! Returns the operator precedence of `op_type`.
static uint64_t get_precedence(ast_operator_type op_type) {
  if (!PRECEDENCE) {
    init_precedence();
  }
  return PRECEDENCE[op_type];
}

//! Operator associativity
typedef enum op_assoc {
  //! Left associative
  LEFT,
  //! Right associative
  RIGHT,
} op_assoc;

//! Maps `ast_operator_type` to their corresponding associativity.
static op_assoc* ASSOC = NULL;

//! Initializes `ASSOC`.
static void init_associativity(void) {
  ASSOC = malloc_safe(50 * sizeof(op_assoc));

  ASSOC[OP_POSTINC] = LEFT;
  ASSOC[OP_POSTDEC] = LEFT;

  ASSOC[OP_PREINC] = RIGHT;
  ASSOC[OP_PREDEC] = RIGHT;
  ASSOC[OP_POS] = RIGHT;
  ASSOC[OP_NEG] = RIGHT;
  ASSOC[OP_NOT] = RIGHT;
  ASSOC[OP_BITNOT] = RIGHT;
  ASSOC[OP_DEREF] = RIGHT;
  ASSOC[OP_ADDROF] = RIGHT;

  ASSOC[OP_MUL] = LEFT;
  ASSOC[OP_DIV] = LEFT;
  ASSOC[OP_MOD] = LEFT;
  ASSOC[OP_ADD] = LEFT;
  ASSOC[OP_SUB] = LEFT;
  ASSOC[OP_SHL] = LEFT;
  ASSOC[OP_SHR] = LEFT;

  ASSOC[OP_LT] = LEFT;
  ASSOC[OP_LE] = LEFT;
  ASSOC[OP_GT] = LEFT;
  ASSOC[OP_GE] = LEFT;

  ASSOC[OP_EQ] = LEFT;
  ASSOC[OP_NE] = LEFT;

  ASSOC[OP_BITAND] = LEFT;
  ASSOC[OP_BITXOR] = LEFT;
  ASSOC[OP_BITOR] = LEFT;
  ASSOC[OP_AND] = LEFT;
  ASSOC[OP_OR] = LEFT;

  ASSOC[OP_ASSIGN] = RIGHT;
}

//! Returns the operator associativity of `op_type`.
static op_assoc get_associativity(ast_operator_type op_type) {
  if (!ASSOC) {
    init_associativity();
  }
  return ASSOC[op_type];
}

//! Recursively purses expressions using precedence climbing.
//! Based on:
//! https://eli.thegreenplace.net/2012/08/02/parsing-expressions-by-precedence-climbing
static ast_expression* parse_expression_internal(parser* parser,
                                                 uint64_t min_pred) {
  ast_expression* lhs = parse_unary_expression(parser);
  while (has_token(parser)) {
    token* tok = peek_token(parser);
    ast_operator_type op_type;
    if (!get_binary_op_type(tok, &op_type)) {
      break;
    }
    uint64_t pred = get_precedence(op_type);
    if (pred < min_pred) {
      break;
    }
    op_assoc assoc = get_associativity(op_type);

    uint64_t next_min_pred = pred;
    if (assoc == LEFT) {
      next_min_pred = pred + 1;
    }

    ast_expression* binary_expr = create_binary_expression(op_type, tok);
    consume_token(parser);
    ast_expression* rhs = parse_expression_internal(parser, next_min_pred);
    binary_expr->node.expression->lhs = lhs;
    binary_expr->node.expression->rhs = rhs;
    lhs = binary_expr;
  }
  return lhs;
}

//! Parses expression.
ast_expression* parse_expression(parser* parser) {
  return parse_expression_internal(parser, /*min_pred=*/1);
}

//! Returns an AST node with an empty statement.
static ast_node* create_statement(void) {
  ast_node* node = calloc_safe(/*nelem=*/1, sizeof(ast_node));
  node->node_type = AST_STMNT;
  ast_statement* statement = calloc_safe(/*nelem=*/1, sizeof(ast_statement));
  array_init(&statement->items, sizeof(ast_statement_item));
  node->node.statement = statement;
  return node;
}

static ast_declaration* parse_ast_declaration(parser* parser) {
  // Very basic declaration grammar: int <identifier> = <expression>;
  token* tok = peek_token(parser);
  if (!is_keyword_token(tok, "int")) {
    return NULL;
  }
  consume_token(parser);

  ast_declaration* declaration =
      calloc_safe(/*nelem=*/1, sizeof(ast_declaration));
  tok = consume_identifier(parser);
  declaration->identifier = tok;

  tok = peek_token(parser);
  if (is_punctuator_token(tok, "=")) {
    consume_token(parser);
    ast_expression* expr = parse_expression(parser);
    declaration->expression = expr;
  }
  consume_punctuator(parser, ";");
  return declaration;
}

//! Parses a return statement.
static ast_expression* parse_return_statement(parser* parser) {
  consume_keyword(parser, "return");
  ast_expression* expression = parse_expression(parser);
  consume_punctuator(parser, ";");
  return expression;
}

//! Parses a compound statement. A compound statement is a statement enclosed
//! with curly brackets "{}". It may contain multiple declarations and/or other
//! statements. It may also be empty.
static ast_node* parse_compound_statement(parser* parser) {
  consume_punctuator(parser, "{");
  ast_node* statements = create_statement();

  for (token* tok = peek_token(parser); !is_punctuator_token(tok, "}");
       tok = peek_token(parser)) {
    ast_declaration* declaration = parse_ast_declaration(parser);
    if (declaration) {
      ast_statement_item* item =
          array_push_back(&statements->node.statement->items);
      item->type = STMT_DECL;
      item->declaration = declaration;
      continue;
    }
    ast_expression* expr = parse_return_statement(parser);
    ast_statement_item* item =
        array_push_back(&statements->node.statement->items);
    item->type = STMT_RET;
    item->expression = expr;
  }

  consume_punctuator(parser, "}");
  return statements;
}

static ast_node* parse_function_definition(parser* parser) {
  consume_keyword(parser, "int");
  consume_identifier(parser);

  consume_punctuator(parser, "(");
  consume_keyword(parser, "void");
  consume_punctuator(parser, ")");

  return parse_compound_statement(parser);
}

ast_node* parse(array* tokens) {
  parser parser;
  parser.cur = 0;
  parser.tokens = tokens;

  return parse_function_definition(&parser);
}

// Forward declarations.
void ast_destroy(ast_node*);
static void destroy_ast_expression(ast_expression*);

//! Destroys `var`.
static void destroy_ast_variable(ast_variable* var) { free(var); }

//! Destroys `constant`.
static void destroy_ast_constant(ast_constant* constant) { free(constant); }

//! Destroys an AST expression `node`. Recursively destroys `node`'s left and
//! right hand sides.
static void destroy_ast_expression_node(ast_expression_node* node) {
  destroy_ast_expression(node->lhs);
  destroy_ast_expression(node->rhs);
  free(node->op);
  free(node);
}

//! Recursively destroys the entire expression tree `expr`.
static void destroy_ast_expression(ast_expression* expr) {
  if (!expr) {
    return;
  }

  switch (expr->type) {
    case EXPR:
      destroy_ast_expression_node(expr->node.expression);
      break;
    case EXPR_VAR:
      destroy_ast_variable(expr->node.variable);
      break;
    case EXPR_CONST:
      destroy_ast_constant(expr->node.consant);
      break;
  }
  free(expr);
}

//! Destroys `declaration`.
static void destroy_ast_declaration(ast_declaration* declaration) {
  destroy_ast_expression(declaration->expression);
  free(declaration);
}

//! Destroys a statement `item`.
static void destroy_ast_statement_item(ast_statement_item* item) {
  switch (item->type) {
    case STMT_DECL:
      destroy_ast_declaration(item->declaration);
      break;
    case STMT_EXPR:
      destroy_ast_expression(item->expression);
      break;
    case STMT_RET:
      destroy_ast_expression(item->expression);
      break;
  }
}

//! Destroys all items within the statement `stmt`.
static void destroy_ast_statement(ast_statement* stmt) {
  for (size_t i = 0; i < stmt->items.size; ++i) {
    ast_statement_item* item = array_at(&stmt->items, i);
    destroy_ast_statement_item(item);
  }
  array_destroy(&stmt->items);
  free(stmt);
}

void ast_destroy(ast_node* node) {
  if (!node) {
    return;
  }

  switch (node->node_type) {
    case AST_EXPR:
      destroy_ast_expression(node->node.expression);
      break;
    case AST_STMNT:
      destroy_ast_statement(node->node.statement);
      break;
  }
  free(node);
}
