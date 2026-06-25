#include <stdbool.h>

#include "../array.h"
#include "../ir.h"
#include "../lexer.h"
#include "../parser.h"
#include "../prettyprint.h"
#include "../unity/unity.h"

void setUp(void) {}

void tearDown(void) {}

void verify_constant(ir_val* val, uint64_t expected) {
  TEST_ASSERT_EQUAL(true, val->is_constant);
  TEST_ASSERT_EQUAL(expected, val->val.constant);
}

void verify_var_name(ir_val* val, const char* expected) {
  TEST_ASSERT_EQUAL(false, val->is_constant);
  TEST_ASSERT_EQUAL_STRING(expected, val->val.var_name.data);
}

void test_ir_unary_ops(void) {
  char text[] = "int main(void){ return -~2;}";
  array tokens = lex(text, "test.c");
  ast_node* ast = parse(&tokens);
  ir_node* ir = emit_ir(ast);
  prettyprint_ir(ir);

  const char* name = ir->function_definition->name;
  TEST_ASSERT_EQUAL_STRING("main", name);

  array* instructions = ir->function_definition->instructions;
  TEST_ASSERT_EQUAL(3, instructions->size);

  ir_instruction* inst = array_at(instructions, 0);
  TEST_ASSERT_EQUAL(IR_UNARY, inst->instruction_type);
  TEST_ASSERT_EQUAL(OP_BITNOT, inst->op->op_type);
  verify_constant(inst->lhs, 2);
  verify_var_name(inst->dst, "1_");

  inst = array_at(instructions, 1);
  TEST_ASSERT_EQUAL(IR_UNARY, inst->instruction_type);
  TEST_ASSERT_EQUAL(OP_NEG, inst->op->op_type);
  verify_var_name(inst->lhs, "1_");
  verify_var_name(inst->dst, "2_");

  inst = array_at(instructions, 2);
  TEST_ASSERT_EQUAL(IR_RETURN, inst->instruction_type);
  verify_var_name(inst->lhs, "2_");

  ir_destroy(ir);
  ast_destroy(ast);
  destroy_tokens(&tokens);
}

void test_ir_binary_ops(void) {
  char text[] = "int main(void){ return 1 + 2 * 3 - 4;}";
  array tokens = lex(text, "test.c");
  ast_node* ast = parse(&tokens);
  ir_node* ir = emit_ir(ast);
  prettyprint_ir(ir);

  const char* name = ir->function_definition->name;
  TEST_ASSERT_EQUAL_STRING("main", name);

  array* instructions = ir->function_definition->instructions;
  TEST_ASSERT_EQUAL(4, instructions->size);

  ir_instruction* inst = array_at(instructions, 0);
  TEST_ASSERT_EQUAL(IR_BINARY, inst->instruction_type);
  TEST_ASSERT_EQUAL(OP_MUL, inst->op->op_type);
  verify_constant(inst->lhs, 2);
  verify_constant(inst->rhs, 3);
  verify_var_name(inst->dst, "1_");

  inst = array_at(instructions, 1);
  TEST_ASSERT_EQUAL(IR_BINARY, inst->instruction_type);
  TEST_ASSERT_EQUAL(OP_ADD, inst->op->op_type);
  verify_constant(inst->lhs, 1);
  verify_var_name(inst->rhs, "1_");
  verify_var_name(inst->dst, "2_");

  inst = array_at(instructions, 2);
  TEST_ASSERT_EQUAL(IR_BINARY, inst->instruction_type);
  TEST_ASSERT_EQUAL(OP_SUB, inst->op->op_type);
  verify_var_name(inst->lhs, "2_");
  verify_constant(inst->rhs, 4);
  verify_var_name(inst->dst, "3_");

  inst = array_at(instructions, 3);
  TEST_ASSERT_EQUAL(IR_RETURN, inst->instruction_type);
  verify_var_name(inst->lhs, "3_");

  ir_destroy(ir);
  ast_destroy(ast);
  destroy_tokens(&tokens);
}

void test_ir_bit_ops(void) {
  char text[] = "int main(void){ return 1 & 2 >> 3 | 4;}";
  array tokens = lex(text, "test.c");
  ast_node* ast = parse(&tokens);
  ir_node* ir = emit_ir(ast);
  prettyprint_ir(ir);

  const char* name = ir->function_definition->name;
  TEST_ASSERT_EQUAL_STRING("main", name);

  array* instructions = ir->function_definition->instructions;
  TEST_ASSERT_EQUAL(4, instructions->size);

  ir_instruction* inst = array_at(instructions, 0);
  TEST_ASSERT_EQUAL(IR_BINARY, inst->instruction_type);
  TEST_ASSERT_EQUAL(OP_SHR, inst->op->op_type);
  verify_constant(inst->lhs, 2);
  verify_constant(inst->rhs, 3);
  verify_var_name(inst->dst, "1_");

  inst = array_at(instructions, 1);
  TEST_ASSERT_EQUAL(IR_BINARY, inst->instruction_type);
  TEST_ASSERT_EQUAL(OP_BITAND, inst->op->op_type);
  verify_constant(inst->lhs, 1);
  verify_var_name(inst->rhs, "1_");
  verify_var_name(inst->dst, "2_");

  inst = array_at(instructions, 2);
  TEST_ASSERT_EQUAL(IR_BINARY, inst->instruction_type);
  TEST_ASSERT_EQUAL(OP_BITOR, inst->op->op_type);
  verify_var_name(inst->lhs, "2_");
  verify_constant(inst->rhs, 4);
  verify_var_name(inst->dst, "3_");

  inst = array_at(instructions, 3);
  TEST_ASSERT_EQUAL(IR_RETURN, inst->instruction_type);
  verify_var_name(inst->lhs, "3_");

  ir_destroy(ir);
  ast_destroy(ast);
  destroy_tokens(&tokens);
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_ir_unary_ops);
  RUN_TEST(test_ir_binary_ops);
  RUN_TEST(test_ir_bit_ops);
  return UNITY_END();
}
