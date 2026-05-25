#include <stdio.h>
#include <stdlib.h>

#include "../array.h"
#include "../ir.h"
#include "../lexer.h"
#include "../parser.h"
#include "../unity/unity.h"
#include "codegen.h"

static FILE* file = NULL;
static const char* filename = ".test_codegen_tmp.txt";

void setUp(void) { file = fopen(filename, "w+"); }

void tearDown(void) {
  fclose(file);
  remove(filename);
}

void test_codegen_basic(void) {
  char text[] = "int main(void){ return -2;}";
  array tokens = lex(text, "test.c");
  ast_node* ast = parse(&tokens);
  ir_node* ir = emit_ir(ast);

  emit(file, ir);
  ir_destroy(ir);
  ast_destroy(ast);
  destroy_tokens(&tokens);

  fseek(file, 0, SEEK_END);
  long fsize = ftell(file);
  rewind(file);

  char* actual = malloc(fsize + 1);
  fread(actual, fsize, 1, file);
  actual[fsize] = '\0';

  const char* expected =
      ".globl main\n"
      "main:\n"
      "pushq %rbp\n"
      "movq %rsp, %rbp\n"
      "subq $4, %rsp\n"
      "movl $2, -4(%rbp)\n"
      "negl -4(%rbp)\n"
      "movl -4(%rbp), %eax\n"
      "movq %rbp, %rsp\n"
      "popq %rbp\n"
      "ret\n"
      ".section .note.GNU-stack,\"\",@progbits\n";
  TEST_ASSERT_EQUAL_STRING(expected, actual);

  free(actual);
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_codegen_basic);
  return UNITY_END();
}
