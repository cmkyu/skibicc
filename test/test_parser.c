#include "../array.h"
#include "../lexer.h"
#include "../parser.h"
#include "../prettyprint.h"
#include "../unity/unity.h"

void setUp(void) {}

void tearDown(void) {}

void test_parser_basic(void) {
  char text[] = "int main(void) { return 2; }";
  array tokens = lex(text);
  ast_node* ast = parse(&tokens);
  ast_destroy(ast);
  destroy_tokens(&tokens);
}

void test_parser_unary_expression(void) {
  char text[] = "int main(void){ return ++ + - (&foo)++--;}";
  array tokens = lex(text);
  ast_node* ast = parse(&tokens);
  prettyprint_ast(ast);

  TEST_ASSERT_EQUAL(AST_RETSTMNT, ast->node_type);

  ast = ast->node.statement->expression;
  TEST_ASSERT_EQUAL(AST_EXPR, ast->node_type);
  TEST_ASSERT_EQUAL(OP_PREINC, ast->node.expression->op->op_type);

  ast = ast->node.expression->lhs;
  TEST_ASSERT_EQUAL(AST_EXPR, ast->node_type);
  TEST_ASSERT_EQUAL(OP_POS, ast->node.expression->op->op_type);

  ast = ast->node.expression->lhs;
  TEST_ASSERT_EQUAL(AST_EXPR, ast->node_type);
  TEST_ASSERT_EQUAL(OP_NEG, ast->node.expression->op->op_type);

  ast = ast->node.expression->lhs;
  TEST_ASSERT_EQUAL(AST_EXPR, ast->node_type);
  TEST_ASSERT_EQUAL(OP_POSTDEC, ast->node.expression->op->op_type);

  ast = ast->node.expression->lhs;
  TEST_ASSERT_EQUAL(AST_EXPR, ast->node_type);
  TEST_ASSERT_EQUAL(OP_POSTINC, ast->node.expression->op->op_type);

  ast = ast->node.expression->lhs;
  TEST_ASSERT_EQUAL(AST_EXPR, ast->node_type);
  TEST_ASSERT_EQUAL(OP_ADDROF, ast->node.expression->op->op_type);

  ast = ast->node.expression->lhs;
  TEST_ASSERT_EQUAL(AST_VAR, ast->node_type);

  ast_destroy(ast);
  destroy_tokens(&tokens);
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_parser_basic);
  RUN_TEST(test_parser_unary_expression);
  return UNITY_END();
}
