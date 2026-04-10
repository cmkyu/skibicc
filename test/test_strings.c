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

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_string_concat);
  return UNITY_END();
}
