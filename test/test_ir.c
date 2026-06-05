#include <stdbool.h>

#include "../array.h"
#include "../ir.h"
#include "../lexer.h"
#include "../parser.h"
#include "../prettyprint.h"
#include "../unity/unity.h"

void setUp(void) {}

void tearDown(void) {}

void test_ir_basic(void) {
  char text[] = "int main(void){ return -2;}";
  array tokens = lex(text, "test.c");
  ast_node* ast = parse(&tokens);
  ir_node* ir = emit_ir(ast);
  prettyprint_ir(ir);

  const char* name = ir->function_definition->name;
  TEST_ASSERT_EQUAL_STRING("main", name);

  array* instructions = ir->function_definition->instructions;
  TEST_ASSERT_EQUAL(2, instructions->size);

  ir_instruction* inst = array_at(instructions, 0);
  TEST_ASSERT_EQUAL(IR_ARITH, inst->instruction_type);
  TEST_ASSERT_EQUAL(OP_NEG, inst->op->op_type);

  TEST_ASSERT_EQUAL(true, inst->lhs->is_constant);
  ast_node* node = inst->lhs->val.constant;
  TEST_ASSERT_EQUAL(AST_CONST, node->node_type);
  token* tok = node->node.consant->tok;
  TEST_ASSERT_EQUAL(TK_ICONST, tok->token_type);
  TEST_ASSERT_EQUAL(2, tok->constant.int_val);

  TEST_ASSERT_EQUAL(false, inst->dst->is_constant);
  TEST_ASSERT_EQUAL_STRING("1_", inst->dst->val.var_name.data);

  inst = array_at(instructions, 1);
  TEST_ASSERT_EQUAL(IR_RETURN, inst->instruction_type);
  TEST_ASSERT_EQUAL_STRING("1_", inst->lhs->val.var_name.data);

  ir_destroy(ir);
  ast_destroy(ast);
  destroy_tokens(&tokens);
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_ir_basic);
  return UNITY_END();
}
