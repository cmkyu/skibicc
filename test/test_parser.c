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
  ast_destroy(ast);
  destroy_tokens(&tokens);
}

void test_parser_unary_expression(void) {
  char text[] = "int main(void){ return ++ + - (&foo)++--;}";
  array tokens = lex(text, "test.c");
  ast_node* ast = parse(&tokens);
  ast_node* root = ast;
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

  ast_destroy(root);
  destroy_tokens(&tokens);
}

void test_parser_binary_expression(void) {
  char text[] = "int main(void){ return -1 + 2 * (3 % 6 - -5) / 4 - 1;}";
  array tokens = lex(text, "test.c");
  ast_node* ast = parse(&tokens);
  ast_node* root = ast;
  prettyprint_ast(ast);

  TEST_ASSERT_EQUAL(AST_RETSTMNT, ast->node_type);

  ast = ast->node.statement->expression;
  TEST_ASSERT_EQUAL(AST_EXPR, ast->node_type);
  TEST_ASSERT_EQUAL(OP_SUB, ast->node.expression->op->op_type);

  ast_node* lhs = ast->node.expression->lhs;
  TEST_ASSERT_EQUAL(AST_EXPR, lhs->node_type);
  TEST_ASSERT_EQUAL(OP_ADD, lhs->node.expression->op->op_type);
  ast_node* rhs = ast->node.expression->rhs;
  TEST_ASSERT_EQUAL(AST_CONST, rhs->node_type);

  ast = lhs;
  lhs = ast->node.expression->lhs;
  TEST_ASSERT_EQUAL(AST_EXPR, lhs->node_type);
  TEST_ASSERT_EQUAL(OP_NEG, lhs->node.expression->op->op_type);
  lhs = lhs->node.expression->lhs;
  TEST_ASSERT_EQUAL(AST_CONST, lhs->node_type);
  rhs = ast->node.expression->rhs;
  TEST_ASSERT_EQUAL(AST_EXPR, rhs->node_type);
  TEST_ASSERT_EQUAL(OP_DIV, rhs->node.expression->op->op_type);

  ast = rhs;
  lhs = ast->node.expression->lhs;
  TEST_ASSERT_EQUAL(AST_EXPR, lhs->node_type);
  TEST_ASSERT_EQUAL(OP_MUL, lhs->node.expression->op->op_type);
  rhs = ast->node.expression->rhs;
  TEST_ASSERT_EQUAL(AST_CONST, rhs->node_type);

  ast = lhs;
  lhs = ast->node.expression->lhs;
  TEST_ASSERT_EQUAL(AST_CONST, lhs->node_type);
  rhs = ast->node.expression->rhs;
  TEST_ASSERT_EQUAL(AST_EXPR, rhs->node_type);
  TEST_ASSERT_EQUAL(OP_SUB, rhs->node.expression->op->op_type);

  ast = rhs;
  lhs = ast->node.expression->lhs;
  TEST_ASSERT_EQUAL(AST_EXPR, lhs->node_type);
  TEST_ASSERT_EQUAL(OP_MOD, lhs->node.expression->op->op_type);
  rhs = ast->node.expression->rhs;
  TEST_ASSERT_EQUAL(AST_EXPR, rhs->node_type);
  TEST_ASSERT_EQUAL(OP_NEG, rhs->node.expression->op->op_type);
  rhs = rhs->node.expression->lhs;
  TEST_ASSERT_EQUAL(AST_CONST, rhs->node_type);

  ast = lhs;
  lhs = ast->node.expression->lhs;
  TEST_ASSERT_EQUAL(AST_CONST, lhs->node_type);
  rhs = ast->node.expression->rhs;
  TEST_ASSERT_EQUAL(AST_CONST, rhs->node_type);

  ast_destroy(root);
  destroy_tokens(&tokens);
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_parser_basic);
  RUN_TEST(test_parser_unary_expression);
  RUN_TEST(test_parser_binary_expression);
  return UNITY_END();
}
