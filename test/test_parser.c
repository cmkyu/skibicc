#include "../array.h"
#include "../lexer.h"
#include "../parser.h"
#include "../prettyprint.h"
#include "../unity/unity.h"

void setUp(void) {}

void tearDown(void) {}

void test_parser_basic(void) {
  char text[] = "int main(void) { return 2; }";
  array tokens = lex(text, "test.c");
  ast_node* ast = parse(&tokens);
  prettyprint_ast(ast);
  ast_destroy(ast);
  destroy_tokens(&tokens);
}

void test_parser_unary_expression(void) {
  char text[] = "int main(void){ return ++ + - (&foo)++--;}";
  array tokens = lex(text, "test.c");
  ast_node* tree = parse(&tokens);
  ast_node* root = tree;
  prettyprint_ast(tree);

  TEST_ASSERT_EQUAL(AST_STMNT, tree->node_type);
  ast_statement_item* item = array_at(&tree->node.statement->items, 0);
  TEST_ASSERT_EQUAL(STMT_RET, item->type);

  ast_expression* ast = item->expression;
  TEST_ASSERT_EQUAL(EXPR, ast->type);
  TEST_ASSERT_EQUAL(OP_PREINC, ast->node.expression->op->op_type);

  ast = ast->node.expression->lhs;
  TEST_ASSERT_EQUAL(EXPR, ast->type);
  TEST_ASSERT_EQUAL(OP_POS, ast->node.expression->op->op_type);

  ast = ast->node.expression->lhs;
  TEST_ASSERT_EQUAL(EXPR, ast->type);
  TEST_ASSERT_EQUAL(OP_NEG, ast->node.expression->op->op_type);

  ast = ast->node.expression->lhs;
  TEST_ASSERT_EQUAL(EXPR, ast->type);
  TEST_ASSERT_EQUAL(OP_POSTDEC, ast->node.expression->op->op_type);

  ast = ast->node.expression->lhs;
  TEST_ASSERT_EQUAL(EXPR, ast->type);
  TEST_ASSERT_EQUAL(OP_POSTINC, ast->node.expression->op->op_type);

  ast = ast->node.expression->lhs;
  TEST_ASSERT_EQUAL(EXPR, ast->type);
  TEST_ASSERT_EQUAL(OP_ADDROF, ast->node.expression->op->op_type);

  ast = ast->node.expression->lhs;
  TEST_ASSERT_EQUAL(EXPR_VAR, ast->type);

  ast_destroy(root);
  destroy_tokens(&tokens);
}

void test_parser_binary_expression(void) {
  char text[] = "int main(void){ return -1 + 2 * (3 % 6 - -5) / 4 - 1;}";
  array tokens = lex(text, "test.c");
  ast_node* tree = parse(&tokens);
  ast_node* root = tree;
  prettyprint_ast(tree);

  TEST_ASSERT_EQUAL(AST_STMNT, tree->node_type);
  ast_statement_item* item = array_at(&tree->node.statement->items, 0);
  TEST_ASSERT_EQUAL(STMT_RET, item->type);

  ast_expression* ast = item->expression;
  TEST_ASSERT_EQUAL(EXPR, ast->type);
  TEST_ASSERT_EQUAL(OP_SUB, ast->node.expression->op->op_type);

  ast_expression* lhs = ast->node.expression->lhs;
  TEST_ASSERT_EQUAL(EXPR, lhs->type);
  TEST_ASSERT_EQUAL(OP_ADD, lhs->node.expression->op->op_type);
  ast_expression* rhs = ast->node.expression->rhs;
  TEST_ASSERT_EQUAL(EXPR_CONST, rhs->type);

  ast = lhs;
  lhs = ast->node.expression->lhs;
  TEST_ASSERT_EQUAL(EXPR, lhs->type);
  TEST_ASSERT_EQUAL(OP_NEG, lhs->node.expression->op->op_type);
  lhs = lhs->node.expression->lhs;
  TEST_ASSERT_EQUAL(EXPR_CONST, lhs->type);
  rhs = ast->node.expression->rhs;
  TEST_ASSERT_EQUAL(AST_EXPR, rhs->type);
  TEST_ASSERT_EQUAL(OP_DIV, rhs->node.expression->op->op_type);

  ast = rhs;
  lhs = ast->node.expression->lhs;
  TEST_ASSERT_EQUAL(EXPR, lhs->type);
  TEST_ASSERT_EQUAL(OP_MUL, lhs->node.expression->op->op_type);
  rhs = ast->node.expression->rhs;
  TEST_ASSERT_EQUAL(EXPR_CONST, rhs->type);

  ast = lhs;
  lhs = ast->node.expression->lhs;
  TEST_ASSERT_EQUAL(EXPR_CONST, lhs->type);
  rhs = ast->node.expression->rhs;
  TEST_ASSERT_EQUAL(EXPR, rhs->type);
  TEST_ASSERT_EQUAL(OP_SUB, rhs->node.expression->op->op_type);

  ast = rhs;
  lhs = ast->node.expression->lhs;
  TEST_ASSERT_EQUAL(EXPR, lhs->type);
  TEST_ASSERT_EQUAL(OP_MOD, lhs->node.expression->op->op_type);
  rhs = ast->node.expression->rhs;
  TEST_ASSERT_EQUAL(EXPR, rhs->type);
  TEST_ASSERT_EQUAL(OP_NEG, rhs->node.expression->op->op_type);
  rhs = rhs->node.expression->lhs;
  TEST_ASSERT_EQUAL(EXPR_CONST, rhs->type);

  ast = lhs;
  lhs = ast->node.expression->lhs;
  TEST_ASSERT_EQUAL(EXPR_CONST, lhs->type);
  rhs = ast->node.expression->rhs;
  TEST_ASSERT_EQUAL(EXPR_CONST, rhs->type);

  ast_destroy(root);
  destroy_tokens(&tokens);
}

void test_parser_bit_operators(void) {
  char text[] = "int main(void){ return 1 << 2 >> 3 | 4 ^ 5 & 6;}";
  array tokens = lex(text, "test.c");
  ast_node* tree = parse(&tokens);
  ast_node* root = tree;
  prettyprint_ast(tree);

  TEST_ASSERT_EQUAL(AST_STMNT, tree->node_type);
  ast_statement_item* item = array_at(&tree->node.statement->items, 0);
  TEST_ASSERT_EQUAL(STMT_RET, item->type);

  ast_expression* ast = item->expression;
  TEST_ASSERT_EQUAL(EXPR, ast->type);
  TEST_ASSERT_EQUAL(OP_BITOR, ast->node.expression->op->op_type);

  ast_expression* lhs = ast->node.expression->lhs;
  TEST_ASSERT_EQUAL(AST_EXPR, lhs->type);
  TEST_ASSERT_EQUAL(OP_SHR, lhs->node.expression->op->op_type);
  ast_expression* rhs = ast->node.expression->rhs;
  TEST_ASSERT_EQUAL(AST_EXPR, rhs->type);
  TEST_ASSERT_EQUAL(OP_BITXOR, rhs->node.expression->op->op_type);

  {
    ast_expression* ast = lhs;
    ast_expression* lhs = ast->node.expression->lhs;
    TEST_ASSERT_EQUAL(EXPR, lhs->type);
    TEST_ASSERT_EQUAL(OP_SHL, lhs->node.expression->op->op_type);
    ast_expression* rhs = ast->node.expression->rhs;
    TEST_ASSERT_EQUAL(EXPR_CONST, rhs->type);

    ast = lhs;
    lhs = ast->node.expression->lhs;
    TEST_ASSERT_EQUAL(EXPR_CONST, lhs->type);
    rhs = ast->node.expression->rhs;
    TEST_ASSERT_EQUAL(EXPR_CONST, rhs->type);
  }

  ast = rhs;
  lhs = ast->node.expression->lhs;
  TEST_ASSERT_EQUAL(EXPR_CONST, lhs->type);
  rhs = ast->node.expression->rhs;
  TEST_ASSERT_EQUAL(EXPR, rhs->type);
  TEST_ASSERT_EQUAL(OP_BITAND, rhs->node.expression->op->op_type);

  ast = rhs;
  lhs = ast->node.expression->lhs;
  TEST_ASSERT_EQUAL(EXPR_CONST, lhs->type);
  rhs = ast->node.expression->rhs;
  TEST_ASSERT_EQUAL(EXPR_CONST, rhs->type);

  ast_destroy(root);
  destroy_tokens(&tokens);
}

void test_parser_relational_and_logical_operators(void) {
  char text[] = "int main(void){ return 1 < 2 >= 3 || 4 && 5 != 6;}";
  array tokens = lex(text, "test.c");
  ast_node* tree = parse(&tokens);
  ast_node* root = tree;
  prettyprint_ast(tree);

  TEST_ASSERT_EQUAL(AST_STMNT, tree->node_type);
  ast_statement_item* item = array_at(&tree->node.statement->items, 0);
  TEST_ASSERT_EQUAL(STMT_RET, item->type);

  ast_expression* ast = item->expression;
  TEST_ASSERT_EQUAL(EXPR, ast->type);
  TEST_ASSERT_EQUAL(OP_OR, ast->node.expression->op->op_type);

  ast_expression* lhs = ast->node.expression->lhs;
  TEST_ASSERT_EQUAL(EXPR, lhs->type);
  TEST_ASSERT_EQUAL(OP_GE, lhs->node.expression->op->op_type);
  ast_expression* rhs = ast->node.expression->rhs;
  TEST_ASSERT_EQUAL(EXPR, rhs->type);
  TEST_ASSERT_EQUAL(OP_AND, rhs->node.expression->op->op_type);

  {
    ast_expression* ast = lhs;
    ast_expression* lhs = ast->node.expression->lhs;
    TEST_ASSERT_EQUAL(EXPR, lhs->type);
    TEST_ASSERT_EQUAL(OP_LT, lhs->node.expression->op->op_type);
    ast_expression* rhs = ast->node.expression->rhs;
    TEST_ASSERT_EQUAL(EXPR_CONST, rhs->type);

    ast = lhs;
    lhs = ast->node.expression->lhs;
    TEST_ASSERT_EQUAL(EXPR_CONST, lhs->type);
    rhs = ast->node.expression->rhs;
    TEST_ASSERT_EQUAL(EXPR_CONST, rhs->type);
  }

  ast = rhs;
  lhs = ast->node.expression->lhs;
  TEST_ASSERT_EQUAL(EXPR_CONST, lhs->type);
  rhs = ast->node.expression->rhs;
  TEST_ASSERT_EQUAL(EXPR, rhs->type);
  TEST_ASSERT_EQUAL(OP_NE, rhs->node.expression->op->op_type);

  ast = rhs;
  lhs = ast->node.expression->lhs;
  TEST_ASSERT_EQUAL(EXPR_CONST, lhs->type);
  rhs = ast->node.expression->rhs;
  TEST_ASSERT_EQUAL(EXPR_CONST, rhs->type);

  ast_destroy(root);
  destroy_tokens(&tokens);
}

void test_parser_assignment(void) {
  char text[] =
      "int main(void){ int var1 = 1; int var2 = 2; var1 = var1 + var2; return "
      "var1;}";
  array tokens = lex(text, "test.c");
  ast_node* tree = parse(&tokens);
  ast_node* root = tree;
  prettyprint_ast(tree);

  TEST_ASSERT_EQUAL(AST_STMNT, tree->node_type);
  TEST_ASSERT_EQUAL(4, tree->node.statement->items.size);

  ast_statement_item* item = array_at(&tree->node.statement->items, 0);
  TEST_ASSERT_EQUAL(STMT_DECL, item->type);
  ast_declaration* decl = item->declaration;
  token* ident = decl->identifier;
  TEST_ASSERT_EQUAL_STRING_LEN("var1", ident->loc, ident->size);
  ast_expression* expr = decl->expression;
  TEST_ASSERT_EQUAL(EXPR_CONST, expr->type);

  item = array_at(&tree->node.statement->items, 1);
  TEST_ASSERT_EQUAL(STMT_DECL, item->type);
  decl = item->declaration;
  ident = decl->identifier;
  TEST_ASSERT_EQUAL_STRING_LEN("var2", ident->loc, ident->size);
  expr = decl->expression;
  TEST_ASSERT_EQUAL(EXPR_CONST, expr->type);

  item = array_at(&tree->node.statement->items, 2);
  TEST_ASSERT_EQUAL(STMT_EXPR, item->type);
  expr = item->expression;
  TEST_ASSERT_EQUAL(EXPR, expr->type);
  TEST_ASSERT_EQUAL(OP_ASSIGN, expr->node.expression->op->op_type);

  ast_expression* lhs = expr->node.expression->lhs;
  TEST_ASSERT_EQUAL(EXPR_VAR, lhs->type);
  token* var = lhs->node.variable->tok;
  TEST_ASSERT_EQUAL_STRING_LEN("var1", var->loc, var->size);

  ast_expression* rhs = expr->node.expression->rhs;
  TEST_ASSERT_EQUAL(EXPR, rhs->type);
  TEST_ASSERT_EQUAL(OP_ADD, rhs->node.expression->op->op_type);

  lhs = rhs->node.expression->lhs;
  TEST_ASSERT_EQUAL(EXPR_VAR, lhs->type);
  var = lhs->node.variable->tok;
  TEST_ASSERT_EQUAL_STRING_LEN("var1", var->loc, var->size);

  rhs = rhs->node.expression->rhs;
  TEST_ASSERT_EQUAL(EXPR_VAR, rhs->type);
  var = rhs->node.variable->tok;
  TEST_ASSERT_EQUAL_STRING_LEN("var2", var->loc, var->size);

  item = array_at(&tree->node.statement->items, 3);
  TEST_ASSERT_EQUAL(STMT_RET, item->type);
  expr = item->expression;
  TEST_ASSERT_EQUAL(EXPR_VAR, expr->type);
  var = expr->node.variable->tok;
  TEST_ASSERT_EQUAL_STRING_LEN("var1", var->loc, var->size);

  ast_destroy(root);
  destroy_tokens(&tokens);
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_parser_basic);
  RUN_TEST(test_parser_unary_expression);
  RUN_TEST(test_parser_binary_expression);
  RUN_TEST(test_parser_bit_operators);
  RUN_TEST(test_parser_relational_and_logical_operators);
  RUN_TEST(test_parser_assignment);
  return UNITY_END();
}
