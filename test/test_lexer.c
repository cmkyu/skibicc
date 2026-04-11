#include "../lexer.h"
#include "../unity/unity.h"
#include "unity_internals.h"

void setUp(void) {}

void tearDown(void) {}

void test_lex_identifier(void) {
  TEST_ASSERT_EQUAL(4, lex_identifier("main"));
  TEST_ASSERT_EQUAL(4, lex_identifier("m123"));
  TEST_ASSERT_EQUAL(4, lex_identifier("m12n"));

  TEST_ASSERT_EQUAL(9, lex_identifier("foobarbaz"));
  TEST_ASSERT_EQUAL(9, lex_identifier("FooBarbaZ"));
  TEST_ASSERT_EQUAL(9, lex_identifier("fo4bAr7Az"));
  TEST_ASSERT_EQUAL(11, lex_identifier("fo_4bAr7_Az"));
  TEST_ASSERT_EQUAL(12, lex_identifier("_fo_4bAr7_Az"));
  TEST_ASSERT_EQUAL(15, lex_identifier("__fo_4bAr7_Az__"));
  TEST_ASSERT_EQUAL(15, lex_identifier("____fo_4bAr7_Az"));
  TEST_ASSERT_EQUAL(18, lex_identifier("____fo_4bAr7_Az___"));
  TEST_ASSERT_EQUAL(5, lex_identifier("_1234"));
  TEST_ASSERT_EQUAL(6, lex_identifier("_1234_"));
  TEST_ASSERT_EQUAL(6, lex_identifier("__1234"));
  TEST_ASSERT_EQUAL(8, lex_identifier("__1234__"));
  TEST_ASSERT_EQUAL(7, lex_identifier("___1234"));
  TEST_ASSERT_EQUAL(10, lex_identifier("___1234___"));

  TEST_ASSERT_EQUAL(6, lex_identifier("foobar;thisdoesnotcount"));
  TEST_ASSERT_EQUAL(6, lex_identifier("foobar thisdoesnotcount"));
  TEST_ASSERT_EQUAL(6, lex_identifier("foobar{thisdoesnotcount}"));
  TEST_ASSERT_EQUAL(6, lex_identifier("foobar/thisdoesnotcount"));

  TEST_ASSERT_EQUAL(0, lex_identifier(";thisdoesnotcount"));
  TEST_ASSERT_EQUAL(0, lex_identifier(" thisdoesnotcount}"));
  TEST_ASSERT_EQUAL(0, lex_identifier("{thisdoesnotcount}"));
  TEST_ASSERT_EQUAL(0, lex_identifier("/thisdoesnotcount"));

  TEST_ASSERT_EQUAL(0, lex_identifier("123456"));
  TEST_ASSERT_EQUAL(0, lex_identifier("0xab12c"));
  TEST_ASSERT_EQUAL(0, lex_identifier("01234"));
  TEST_ASSERT_EQUAL(0, lex_identifier("123foobar"));
}

void test_lex_constant_decimal(void) {
  TEST_ASSERT_EQUAL(3, lex_constant("234"));
  TEST_ASSERT_EQUAL(17, lex_constant("90283746512379567"));
  TEST_ASSERT_EQUAL(3, lex_constant("234;"));
  TEST_ASSERT_EQUAL(3, lex_constant("234)"));
  TEST_ASSERT_EQUAL(3, lex_constant("234/123"));
  TEST_ASSERT_EQUAL(3, lex_constant("234+456"));
  TEST_ASSERT_EQUAL(3, lex_constant("234-456"));
  TEST_ASSERT_EQUAL(3, lex_constant("234*456"));
  TEST_ASSERT_EQUAL(3, lex_constant("234,456"));

  TEST_ASSERT_EQUAL(0, lex_constant(";123"));
  TEST_ASSERT_EQUAL(0, lex_constant("foobar123"));
  TEST_ASSERT_EQUAL(0, lex_constant("123foobar"));
  TEST_ASSERT_EQUAL(0, lex_constant("__123"));
  TEST_ASSERT_EQUAL(0, lex_constant("__123__"));
  TEST_ASSERT_EQUAL(0, lex_constant("thisdoes123notcount"));

  TEST_ASSERT_EQUAL(0, lex_constant("ull"));
  TEST_ASSERT_EQUAL(0, lex_constant("ull123"));
  TEST_ASSERT_EQUAL(0, lex_constant("123ull123"));
  TEST_ASSERT_EQUAL(0, lex_constant("123ullthisdoesnotcount"));

  TEST_ASSERT_EQUAL(4, lex_constant("674u"));
  TEST_ASSERT_EQUAL(7, lex_constant("1937Ull"));
  TEST_ASSERT_EQUAL(7, lex_constant("67489LL"));
  TEST_ASSERT_EQUAL(6, lex_constant("2937ul"));
  TEST_ASSERT_EQUAL(7, lex_constant("1937LLu"));
  TEST_ASSERT_EQUAL(6, lex_constant("2937Lu"));
  TEST_ASSERT_EQUAL(7, lex_constant("2937ull;"));
  TEST_ASSERT_EQUAL(6, lex_constant("2937lu;thisdoesnotcount"));
}

void test_lex_constant_octal(void) {
  TEST_ASSERT_EQUAL(1, lex_constant("0"));
  TEST_ASSERT_EQUAL(4, lex_constant("0234"));
  TEST_ASSERT_EQUAL(16, lex_constant("0028374651237567"));
  TEST_ASSERT_EQUAL(4, lex_constant("0234;"));
  TEST_ASSERT_EQUAL(4, lex_constant("0234)"));
  TEST_ASSERT_EQUAL(4, lex_constant("0234/123"));
  TEST_ASSERT_EQUAL(4, lex_constant("0234+456"));
  TEST_ASSERT_EQUAL(4, lex_constant("0234-456"));
  TEST_ASSERT_EQUAL(4, lex_constant("0234*456"));
  TEST_ASSERT_EQUAL(4, lex_constant("0234,456"));

  TEST_ASSERT_EQUAL(0, lex_constant(";0123"));
  TEST_ASSERT_EQUAL(0, lex_constant("foobar0123"));
  TEST_ASSERT_EQUAL(0, lex_constant("0123foobar"));
  TEST_ASSERT_EQUAL(0, lex_constant("__0123"));
  TEST_ASSERT_EQUAL(0, lex_constant("__0123__"));
  TEST_ASSERT_EQUAL(0, lex_constant("thisdoes0123notcount"));

  TEST_ASSERT_EQUAL(0, lex_constant("01239"));
  TEST_ASSERT_EQUAL(0, lex_constant("0123977ul"));

  TEST_ASSERT_EQUAL(0, lex_constant("ull0123"));
  TEST_ASSERT_EQUAL(0, lex_constant("0123ull123"));
  TEST_ASSERT_EQUAL(0, lex_constant("0123ullthisdoesnotcount"));

  TEST_ASSERT_EQUAL(5, lex_constant("0674u"));
  TEST_ASSERT_EQUAL(7, lex_constant("0137Ull"));
  TEST_ASSERT_EQUAL(7, lex_constant("06748LL"));
  TEST_ASSERT_EQUAL(6, lex_constant("0237ul"));
  TEST_ASSERT_EQUAL(7, lex_constant("0137LLu"));
  TEST_ASSERT_EQUAL(6, lex_constant("0237Lu"));
  TEST_ASSERT_EQUAL(5, lex_constant("0237U;"));
  TEST_ASSERT_EQUAL(7, lex_constant("0237ULL;thisdoesnotcount"));
}

void test_lex_constant_hexadecial(void) {
  TEST_ASSERT_EQUAL(3, lex_constant("0x0"));
  TEST_ASSERT_EQUAL(3, lex_constant("0X0"));
  TEST_ASSERT_EQUAL(5, lex_constant("0x123"));
  TEST_ASSERT_EQUAL(5, lex_constant("0X456"));
  TEST_ASSERT_EQUAL(8, lex_constant("0x000456"));
  TEST_ASSERT_EQUAL(14, lex_constant("0x1A2b3C8d9e0f"));
  TEST_ASSERT_EQUAL(11, lex_constant("0Xa8B3e9d0F"));

  TEST_ASSERT_EQUAL(5, lex_constant("0x234;"));
  TEST_ASSERT_EQUAL(5, lex_constant("0x234)"));
  TEST_ASSERT_EQUAL(5, lex_constant("0x234/123"));
  TEST_ASSERT_EQUAL(5, lex_constant("0x234+456"));
  TEST_ASSERT_EQUAL(5, lex_constant("0x234-456"));
  TEST_ASSERT_EQUAL(5, lex_constant("0x234*456"));
  TEST_ASSERT_EQUAL(5, lex_constant("0x234,456"));

  TEST_ASSERT_EQUAL(0, lex_constant(";0x123"));
  TEST_ASSERT_EQUAL(0, lex_constant("foobar0x123"));
  TEST_ASSERT_EQUAL(0, lex_constant("0x123foobar"));
  TEST_ASSERT_EQUAL(0, lex_constant("__0x123"));
  TEST_ASSERT_EQUAL(0, lex_constant("__0x123__"));
  TEST_ASSERT_EQUAL(0, lex_constant("thisdoes0x0123notcount"));

  TEST_ASSERT_EQUAL(0, lex_constant("0x"));
  TEST_ASSERT_EQUAL(0, lex_constant("0X"));
  TEST_ASSERT_EQUAL(0, lex_constant("0xu"));
  TEST_ASSERT_EQUAL(0, lex_constant("0Xull"));
  TEST_ASSERT_EQUAL(0, lex_constant("0X12340x"));
  TEST_ASSERT_EQUAL(0, lex_constant("0x1234g"));
  TEST_ASSERT_EQUAL(0, lex_constant("0x37d82ag73f"));

  TEST_ASSERT_EQUAL(0, lex_constant("ull0x123"));
  TEST_ASSERT_EQUAL(0, lex_constant("0x1a23ull0x1f23"));
  TEST_ASSERT_EQUAL(0, lex_constant("0x12aull0x1b3"));
  TEST_ASSERT_EQUAL(0, lex_constant("0x123ullthisdoesnotcount"));

  TEST_ASSERT_EQUAL(6, lex_constant("0x674U"));
  TEST_ASSERT_EQUAL(8, lex_constant("0x137llU"));
  TEST_ASSERT_EQUAL(8, lex_constant("0x6748ll"));
  TEST_ASSERT_EQUAL(7, lex_constant("0x237lu"));
  TEST_ASSERT_EQUAL(8, lex_constant("0x137uLL"));
  TEST_ASSERT_EQUAL(7, lex_constant("0x237uL"));
  TEST_ASSERT_EQUAL(6, lex_constant("0x237U;"));
  TEST_ASSERT_EQUAL(8, lex_constant("0x237LLU;thisdoesnotcount"));
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_lex_identifier);
  RUN_TEST(test_lex_constant_decimal);
  RUN_TEST(test_lex_constant_octal);
  RUN_TEST(test_lex_constant_hexadecial);
  return UNITY_END();
}
