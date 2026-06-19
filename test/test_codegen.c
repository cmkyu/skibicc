#include <stdio.h>
#include <stdlib.h>

#include "../array.h"
#include "../ir.h"
#include "../lexer.h"
#include "../parser.h"
#include "../unity/unity.h"
#include "codegen.h"

static FILE* out_file = NULL;
static const char* out_filename = ".test_codegen_tmp.txt";

char* read_test_file(const char* filename) {
  FILE* f = fopen(filename, "r");
  fseek(f, 0, SEEK_END);
  long fsize = ftell(f);
  rewind(f);

  char* content = malloc(fsize + 1);
  fread(content, fsize, 1, f);
  content[fsize] = '\0';
  fclose(f);
  return content;
}

void setUp(void) { out_file = fopen(out_filename, "w+"); }

void tearDown(void) { remove(out_filename); }

void test_codegen_basic(void) {
  char text[] = "int main(void){ return -2;}";
  array tokens = lex(text, "test.c");
  ast_node* ast = parse(&tokens);
  ir_node* ir = emit_ir(ast);

  emit(out_file, ir);
  fclose(out_file);
  ir_destroy(ir);
  ast_destroy(ast);
  destroy_tokens(&tokens);

  char* actual = read_test_file(out_filename);
  char* expected = read_test_file("./test/data/codegen_test_basic.txt");
  TEST_ASSERT_EQUAL_STRING(expected, actual);

  free(actual);
  free(expected);
}

void test_codegen_binary_ops(void) {
  char text[] = "int main(void){ return 1 + 2 * 3 / 4 - 5 % 6;}";
  array tokens = lex(text, "test.c");
  ast_node* ast = parse(&tokens);
  ir_node* ir = emit_ir(ast);

  emit(out_file, ir);
  fclose(out_file);
  ir_destroy(ir);
  ast_destroy(ast);
  destroy_tokens(&tokens);

  char* actual = read_test_file(out_filename);
  char* expected = read_test_file("./test/data/codegen_test_binary_ops.txt");
  TEST_ASSERT_EQUAL_STRING(expected, actual);

  free(actual);
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_codegen_basic);
  RUN_TEST(test_codegen_binary_ops);
  return UNITY_END();
}
