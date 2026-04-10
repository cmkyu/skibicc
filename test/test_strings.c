#include <stdlib.h>
#include <string.h>

#include "../strings.h"
#include "../unity/unity.h"

void setUp(void) {}

void tearDown(void) {}

void test_string_concat(void) {
  TEST_ASSERT_EQUAL_STRING("", string_concat(1, ""));
  TEST_ASSERT_EQUAL_STRING("abc", string_concat(1, "abc"));
  TEST_ASSERT_EQUAL_STRING("abc", string_concat(2, "abc", ""));
  TEST_ASSERT_EQUAL_STRING("abcdef", string_concat(3, "abc", "d", "ef"));

  TEST_ASSERT_EQUAL_STRING("", string_concat(5, "", "", "", "", ""));
  TEST_ASSERT_EQUAL_STRING("foobarbazFooBaz",
                           string_concat(5, "foo", "bar", "baz", "Foo", "Baz"));
  TEST_ASSERT_EQUAL_STRING(
      "foobarbazFooBaz",
      string_concat(7, "foo", "bar", "", "baz", "Foo", "Baz", ""));

  TEST_ASSERT_EQUAL_STRING(
      "cpp -E myfile.c -o myfile.i",
      string_concat(5, "cpp ", "-E ", "myfile.c ", "-o ", "myfile.i"));
  TEST_ASSERT_EQUAL_STRING("ld -o myfile /lib/crt0.o hello.o -lc",
                           string_concat(6, "ld ", "-o ", "myfile ",
                                         "/lib/crt0.o ", "hello.o ", "-lc"));
}

void test_replace_last(void) {
  char* p1 = malloc(1024);

  strcpy(p1, "../path/to/lexer.c");
  TEST_ASSERT_TRUE(replace_ext(&p1, "rst"));
  TEST_ASSERT_EQUAL_STRING("../path/to/lexer.rst", p1);

  strcpy(p1, "./path/to/lexer.c");
  TEST_ASSERT_TRUE(replace_ext(&p1, "rst"));
  TEST_ASSERT_EQUAL_STRING("./path/to/lexer.rst", p1);

  strcpy(p1, "/path/to/lexer.c");
  TEST_ASSERT_TRUE(replace_ext(&p1, "rst"));
  TEST_ASSERT_EQUAL_STRING("/path/to/lexer.rst", p1);

  strcpy(p1, "../path/to/.lexer.c");
  TEST_ASSERT_TRUE(replace_ext(&p1, "rst"));
  TEST_ASSERT_EQUAL_STRING("../path/to/.lexer.rst", p1);

  strcpy(p1, "../path/to/.lexer");
  TEST_ASSERT_TRUE(replace_ext(&p1, "rst"));
  TEST_ASSERT_EQUAL_STRING("../path/to/.lexer.rst", p1);

  strcpy(p1, "../.lexer");
  TEST_ASSERT_TRUE(replace_ext(&p1, "rst"));
  TEST_ASSERT_EQUAL_STRING("../.lexer.rst", p1);

  strcpy(p1, "./.lexer");
  TEST_ASSERT_TRUE(replace_ext(&p1, "rst"));
  TEST_ASSERT_EQUAL_STRING("./.lexer.rst", p1);

  strcpy(p1, "/.lexer");
  TEST_ASSERT_TRUE(replace_ext(&p1, "rst"));
  TEST_ASSERT_EQUAL_STRING("/.lexer.rst", p1);

  strcpy(p1, ".lexer");
  TEST_ASSERT_TRUE(replace_ext(&p1, "rst"));
  TEST_ASSERT_EQUAL_STRING(".lexer.rst", p1);

  strcpy(p1, ".lexer.c");
  TEST_ASSERT_TRUE(replace_ext(&p1, "rst"));
  TEST_ASSERT_EQUAL_STRING(".lexer.rst", p1);

  strcpy(p1, ".lexer.c.d");
  TEST_ASSERT_TRUE(replace_ext(&p1, "rst"));
  TEST_ASSERT_EQUAL_STRING(".lexer.c.rst", p1);

  strcpy(p1, "..lexer");
  TEST_ASSERT_TRUE(replace_ext(&p1, "rst"));
  TEST_ASSERT_EQUAL_STRING("..lexer.rst", p1);

  strcpy(p1, "..lexer.c");
  TEST_ASSERT_TRUE(replace_ext(&p1, "rst"));
  TEST_ASSERT_EQUAL_STRING("..lexer.rst", p1);

  strcpy(p1, "...c");
  TEST_ASSERT_TRUE(replace_ext(&p1, "rst"));
  TEST_ASSERT_EQUAL_STRING("...c.rst", p1);

  strcpy(p1, "...");
  TEST_ASSERT_TRUE(replace_ext(&p1, "rst"));
  TEST_ASSERT_EQUAL_STRING("....rst", p1);

  free(p1);
}

void test_replace_ext_alloc(void) {
  char* p1 = malloc(6);
  strcpy(p1, "foo.c");
  TEST_ASSERT_TRUE(replace_ext(&p1, "rst"));
  TEST_ASSERT_EQUAL_STRING("foo.rst", p1);

  char* p2 = malloc(4);
  strcpy(p2, "foo");
  TEST_ASSERT_TRUE(replace_ext(&p2, "rst"));
  TEST_ASSERT_EQUAL_STRING("foo.rst", p2);

  char* p3 = malloc(5);
  strcpy(p3, ".foo");
  TEST_ASSERT_TRUE(replace_ext(&p3, "rst"));
  TEST_ASSERT_EQUAL_STRING(".foo.rst", p3);

  char* p4 = malloc(7);
  strcpy(p4, ".foo.c");
  TEST_ASSERT_TRUE(replace_ext(&p4, "rst"));
  TEST_ASSERT_EQUAL_STRING(".foo.rst", p4);

  free(p1);
  free(p2);
  free(p3);
  free(p4);
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_string_concat);
  RUN_TEST(test_replace_last);
  return UNITY_END();
}
