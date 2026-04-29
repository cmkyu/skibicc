#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uchar.h>

#include "../lexer.h"
#include "../unity/unity.h"

void setUp(void) {}

void tearDown(void) {}

static void verify_tok_str(token* tok, const char* expected) {
  char* actual = malloc(tok->size);
  if (!actual) {
    printf("FAILED: malloc() failed\n");
    exit(1);
  }
  memcpy(actual, tok->loc, tok->size);
  TEST_ASSERT_EQUAL_STRING_LEN(expected, actual, strlen(expected));
  free(actual);
}

static void verify_identifier(token* tok, const char* expected) {
  TEST_ASSERT_EQUAL(TK_IDENT, tok->token_type);
  verify_tok_str(tok, expected);
}

static void verify_keyword(token* tok, const char* expected) {
  TEST_ASSERT_EQUAL(TK_KEYWRD, tok->token_type);
  verify_tok_str(tok, expected);
}

static void verify_integer_constant(token* tok, const char* expected_str,
                                    uint64_t expected) {
  TEST_ASSERT_EQUAL(TK_ICONST, tok->token_type);
  TEST_ASSERT_EQUAL_UINT64(expected, tok->constant.int_val);
  verify_tok_str(tok, expected_str);
}

static void verify_punctuator(token* tok, const char* expected) {
  TEST_ASSERT_EQUAL(TK_PUNCT, tok->token_type);
  verify_tok_str(tok, expected);
}

static void verify_float_constant(token* tok, const char* expected_str,
                                  double expected) {
  TEST_ASSERT_EQUAL(TK_FCONST, tok->token_type);
  TEST_ASSERT_EQUAL_DOUBLE(expected, tok->constant.float_val);
  verify_tok_str(tok, expected_str);
}

static void verify_char_constant(token* tok, const char* expected_str,
                                 char expected) {
  TEST_ASSERT_EQUAL(TK_ICONST, tok->token_type);
  TEST_ASSERT_EQUAL_CHAR(expected, tok->constant.int_val);
  verify_tok_str(tok, expected_str);
}

static void verify_char16_constant(token* tok, const char* expected_str,
                                   char16_t expected) {
  TEST_ASSERT_EQUAL(TK_ICONST, tok->token_type);
  TEST_ASSERT_EQUAL_UINT16(expected, tok->constant.int_val);
  verify_tok_str(tok, expected_str);
}

static void verify_char32_constant(token* tok, const char* expected_str,
                                   char32_t expected) {
  TEST_ASSERT_EQUAL(TK_ICONST, tok->token_type);
  TEST_ASSERT_EQUAL_UINT32(expected, tok->constant.int_val);
  verify_tok_str(tok, expected_str);
}

static void verify_string(token* tok, const char* expected_str,
                          const char* expected) {
  TEST_ASSERT_EQUAL(TK_STRLIT, tok->token_type);
  TEST_ASSERT_EQUAL_STRING(expected, tok->constant.str_val);
  verify_tok_str(tok, expected_str);
}

static void verify_utf8_string(token* tok, const char* expected_str,
                               const uint8_t* expected) {
  TEST_ASSERT_EQUAL(TK_STRLIT, tok->token_type);
  TEST_ASSERT_EQUAL_STRING(expected, tok->constant.str_val);
  verify_tok_str(tok, expected_str);
}

static void verify_utf16_string(token* tok, const char* expected_str,
                                const char16_t* expected) {
  TEST_ASSERT_EQUAL(TK_STRLIT, tok->token_type);
  TEST_ASSERT_EQUAL_STRING(expected, tok->constant.str_val);
  verify_tok_str(tok, expected_str);
}

static void verify_utf32_string(token* tok, const char* expected_str,
                                const char32_t* expected) {
  TEST_ASSERT_EQUAL(TK_STRLIT, tok->token_type);
  TEST_ASSERT_EQUAL_STRING(expected, (const char32_t*)tok->constant.str_val);
  verify_tok_str(tok, expected_str);
}

void test_lex_identifier(void) {
  token tok;
  memset(&tok, 0, sizeof(token));

  TEST_ASSERT_TRUE(lex_identifier("main", &tok));
  verify_identifier(&tok, "main");
  TEST_ASSERT_TRUE(lex_identifier("m123", &tok));
  verify_identifier(&tok, "m123");
  TEST_ASSERT_TRUE(lex_identifier("m12n", &tok));
  verify_identifier(&tok, "m12n");

  TEST_ASSERT_TRUE(lex_identifier("foobarbaz", &tok));
  verify_identifier(&tok, "foobarbaz");
  TEST_ASSERT_TRUE(lex_identifier("FooBarbaZ", &tok));
  verify_identifier(&tok, "FooBarbaZ");
  TEST_ASSERT_TRUE(lex_identifier("fo4bAr7Az", &tok));
  verify_identifier(&tok, "fo4bAr7Az");
  TEST_ASSERT_TRUE(lex_identifier("fo_4bAr7_Az", &tok));
  verify_identifier(&tok, "fo_4bAr7_Az");
  TEST_ASSERT_TRUE(lex_identifier("_fo_4bAr7_Az", &tok));
  verify_identifier(&tok, "_fo_4bAr7_Az");
  TEST_ASSERT_TRUE(lex_identifier("__fo_4bAr7_Az__", &tok));
  verify_identifier(&tok, "__fo_4bAr7_Az__");
  TEST_ASSERT_TRUE(lex_identifier("____fo_4bAr7_Az", &tok));
  verify_identifier(&tok, "____fo_4bAr7_Az");
  TEST_ASSERT_TRUE(lex_identifier("____fo_4bAr7_Az___", &tok));
  verify_identifier(&tok, "____fo_4bAr7_Az___");
  TEST_ASSERT_TRUE(lex_identifier("_1234", &tok));
  verify_identifier(&tok, "_1234");
  TEST_ASSERT_TRUE(lex_identifier("_1234_", &tok));
  verify_identifier(&tok, "_1234_");
  TEST_ASSERT_TRUE(lex_identifier("__1234", &tok));
  verify_identifier(&tok, "__1234");
  TEST_ASSERT_TRUE(lex_identifier("__1234__", &tok));
  verify_identifier(&tok, "__1234__");
  TEST_ASSERT_TRUE(lex_identifier("___1234", &tok));
  verify_identifier(&tok, "___1234");
  TEST_ASSERT_TRUE(lex_identifier("___1234___", &tok));
  verify_identifier(&tok, "___1234___");

  TEST_ASSERT_TRUE(lex_identifier("foobar;thisdoesnotcount", &tok));
  verify_identifier(&tok, "foobar");
  TEST_ASSERT_TRUE(lex_identifier("foobar thisdoesnotcount", &tok));
  verify_identifier(&tok, "foobar");
  TEST_ASSERT_TRUE(lex_identifier("foobar{thisdoesnotcount}", &tok));
  verify_identifier(&tok, "foobar");
  TEST_ASSERT_TRUE(lex_identifier("foobar/thisdoesnotcount", &tok));
  verify_identifier(&tok, "foobar");

  TEST_ASSERT_FALSE(lex_identifier("", &tok));
  TEST_ASSERT_FALSE(lex_identifier(";thisdoesnotcount", &tok));
  TEST_ASSERT_FALSE(lex_identifier(" thisdoesnotcount}", &tok));
  TEST_ASSERT_FALSE(lex_identifier("{thisdoesnotcount}", &tok));
  TEST_ASSERT_FALSE(lex_identifier("/thisdoesnotcount", &tok));

  TEST_ASSERT_FALSE(lex_identifier("123456", &tok));
  TEST_ASSERT_FALSE(lex_identifier("0xab12c", &tok));
  TEST_ASSERT_FALSE(lex_identifier("01234", &tok));
  TEST_ASSERT_FALSE(lex_identifier("123foobar", &tok));
}

void test_lex_decimal_integer(void) {
  token tok;
  memset(&tok, 0, sizeof(token));

  TEST_ASSERT_TRUE(lex_numeric_constant("234", &tok));
  verify_integer_constant(&tok, "234", 234);
  TEST_ASSERT_TRUE(lex_numeric_constant("90283746512379567", &tok));
  verify_integer_constant(&tok, "90283746512379567", 90283746512379567);
  TEST_ASSERT_TRUE(lex_numeric_constant("234;", &tok));
  verify_integer_constant(&tok, "234", 234);
  TEST_ASSERT_TRUE(lex_numeric_constant("123)", &tok));
  verify_integer_constant(&tok, "123", 123);
  TEST_ASSERT_TRUE(lex_numeric_constant("456/123", &tok));
  verify_integer_constant(&tok, "456", 456);
  TEST_ASSERT_TRUE(lex_numeric_constant("567+456", &tok));
  verify_integer_constant(&tok, "567", 567);
  TEST_ASSERT_TRUE(lex_numeric_constant("789-456", &tok));
  verify_integer_constant(&tok, "789", 789);
  TEST_ASSERT_TRUE(lex_numeric_constant("123*456", &tok));
  verify_integer_constant(&tok, "123", 123);
  TEST_ASSERT_TRUE(lex_numeric_constant("567,456", &tok));
  verify_integer_constant(&tok, "567", 567);

  TEST_ASSERT_FALSE(lex_numeric_constant("", &tok));
  TEST_ASSERT_FALSE(lex_numeric_constant(";123", &tok));
  TEST_ASSERT_FALSE(lex_numeric_constant("foobar123", &tok));
  TEST_ASSERT_FALSE(lex_numeric_constant("123foobar", &tok));
  TEST_ASSERT_FALSE(lex_numeric_constant("__123", &tok));
  TEST_ASSERT_FALSE(lex_numeric_constant("__123__", &tok));
  TEST_ASSERT_FALSE(lex_numeric_constant("thisdoes123notcount", &tok));

  TEST_ASSERT_FALSE(lex_numeric_constant("ull", &tok));
  TEST_ASSERT_FALSE(lex_numeric_constant("ull123", &tok));
  TEST_ASSERT_FALSE(lex_numeric_constant("123ull123", &tok));
  TEST_ASSERT_FALSE(lex_numeric_constant("123ullull", &tok));
  TEST_ASSERT_FALSE(lex_numeric_constant("123ullthisdoesnotcount", &tok));

  TEST_ASSERT_TRUE(lex_numeric_constant("674u", &tok));
  verify_integer_constant(&tok, "674u", 674);
  TEST_ASSERT_TRUE(lex_numeric_constant("1937Ull", &tok));
  verify_integer_constant(&tok, "1937Ull", 1937);
  TEST_ASSERT_TRUE(lex_numeric_constant("67489LL", &tok));
  verify_integer_constant(&tok, "67489LL", 67489);
  TEST_ASSERT_TRUE(lex_numeric_constant("2937ul", &tok));
  verify_integer_constant(&tok, "2937ul", 2937);
  TEST_ASSERT_TRUE(lex_numeric_constant("1937LLu", &tok));
  verify_integer_constant(&tok, "1937LLu", 1937);
  TEST_ASSERT_TRUE(lex_numeric_constant("2937Lu", &tok));
  verify_integer_constant(&tok, "2937Lu", 2937);
  TEST_ASSERT_TRUE(lex_numeric_constant("1937ull;", &tok));
  verify_integer_constant(&tok, "1937ull", 1937);
  TEST_ASSERT_TRUE(lex_numeric_constant("2937lu;thisdoesnotcount", &tok));
  verify_integer_constant(&tok, "2937lu", 2937);
}

void test_lex_octal_integer(void) {
  token tok;
  memset(&tok, 0, sizeof(token));

  TEST_ASSERT_TRUE(lex_numeric_constant("0", &tok));
  verify_integer_constant(&tok, "0", 0);
  TEST_ASSERT_TRUE(lex_numeric_constant("0000", &tok));
  verify_integer_constant(&tok, "0000", 0000);
  TEST_ASSERT_TRUE(lex_numeric_constant("0234", &tok));
  verify_integer_constant(&tok, "0234", 0234);
  TEST_ASSERT_TRUE(lex_numeric_constant("0027374651237567", &tok));
  verify_integer_constant(&tok, "0027374651237567", 0027374651237567);
  TEST_ASSERT_TRUE(lex_numeric_constant("0234;", &tok));
  verify_integer_constant(&tok, "0234", 0234);
  TEST_ASSERT_TRUE(lex_numeric_constant("0456)", &tok));
  verify_integer_constant(&tok, "0456", 0456);
  TEST_ASSERT_TRUE(lex_numeric_constant("0234/123", &tok));
  verify_integer_constant(&tok, "0234", 0234);
  TEST_ASSERT_TRUE(lex_numeric_constant("0567+456", &tok));
  verify_integer_constant(&tok, "0567", 0567);
  TEST_ASSERT_TRUE(lex_numeric_constant("0234-456", &tok));
  verify_integer_constant(&tok, "0234", 0234);
  TEST_ASSERT_TRUE(lex_numeric_constant("0567*456", &tok));
  verify_integer_constant(&tok, "0567", 0567);
  TEST_ASSERT_TRUE(lex_numeric_constant("0234,456", &tok));
  verify_integer_constant(&tok, "0234", 0234);

  TEST_ASSERT_FALSE(lex_numeric_constant(";0123", &tok));
  TEST_ASSERT_FALSE(lex_numeric_constant("foobar0123", &tok));
  TEST_ASSERT_FALSE(lex_numeric_constant("0123foobar", &tok));
  TEST_ASSERT_FALSE(lex_numeric_constant("__0123", &tok));
  TEST_ASSERT_FALSE(lex_numeric_constant("__0123__", &tok));
  TEST_ASSERT_FALSE(lex_numeric_constant("thisdoes0123notcount", &tok));

  TEST_ASSERT_FALSE(lex_numeric_constant("01239", &tok));
  TEST_ASSERT_FALSE(lex_numeric_constant("0123977ul", &tok));

  TEST_ASSERT_FALSE(lex_numeric_constant("ull0123", &tok));
  TEST_ASSERT_FALSE(lex_numeric_constant("0123ull123", &tok));
  TEST_ASSERT_FALSE(lex_numeric_constant("0123ullull", &tok));
  TEST_ASSERT_FALSE(lex_numeric_constant("0123ullthisdoesnotcount", &tok));

  TEST_ASSERT_TRUE(lex_numeric_constant("0674u", &tok));
  verify_integer_constant(&tok, "0674u", 0674);
  TEST_ASSERT_TRUE(lex_numeric_constant("0137Ull", &tok));
  verify_integer_constant(&tok, "0137Ull", 0137);
  TEST_ASSERT_TRUE(lex_numeric_constant("06747LL", &tok));
  verify_integer_constant(&tok, "06747LL", 06747);
  TEST_ASSERT_TRUE(lex_numeric_constant("0237ul", &tok));
  verify_integer_constant(&tok, "0237ul", 0237);
  TEST_ASSERT_TRUE(lex_numeric_constant("0137LLu", &tok));
  verify_integer_constant(&tok, "0137LLu", 0137);
  TEST_ASSERT_TRUE(lex_numeric_constant("0237Lu", &tok));
  verify_integer_constant(&tok, "0237Lu", 0237);
  TEST_ASSERT_TRUE(lex_numeric_constant("0137U;", &tok));
  verify_integer_constant(&tok, "0137U", 0137);
  TEST_ASSERT_TRUE(lex_numeric_constant("0237ULL;thisdoesnotcount", &tok));
  verify_integer_constant(&tok, "0237ULL", 0237);
}

void test_lex_hex_integer(void) {
  token tok;
  memset(&tok, 0, sizeof(token));

  TEST_ASSERT_TRUE(lex_numeric_constant("0x0", &tok));
  verify_integer_constant(&tok, "0x0", 0);
  TEST_ASSERT_TRUE(lex_numeric_constant("0X0", &tok));
  verify_integer_constant(&tok, "0X0", 0);
  TEST_ASSERT_TRUE(lex_numeric_constant("0x0000", &tok));
  verify_integer_constant(&tok, "0x0000", 0);
  TEST_ASSERT_TRUE(lex_numeric_constant("0xe", &tok));
  verify_integer_constant(&tok, "0xe", 0xe);
  TEST_ASSERT_TRUE(lex_numeric_constant("0xf", &tok));
  verify_integer_constant(&tok, "0xf", 0xf);
  TEST_ASSERT_TRUE(lex_numeric_constant("0x123", &tok));
  verify_integer_constant(&tok, "0x123", 0x123);
  TEST_ASSERT_TRUE(lex_numeric_constant("0X456", &tok));
  verify_integer_constant(&tok, "0X456", 0x456);
  TEST_ASSERT_TRUE(lex_numeric_constant("0x000456", &tok));
  verify_integer_constant(&tok, "0x000456", 0x456);
  TEST_ASSERT_TRUE(lex_numeric_constant("0x1A2b3C8d9e0f", &tok));
  verify_integer_constant(&tok, "0x1A2b3C8d9e0f", 0x1a2b3c8d9e0f);
  TEST_ASSERT_TRUE(lex_numeric_constant("0Xa8B3e9d0F", &tok));
  verify_integer_constant(&tok, "0Xa8B3e9d0F", 0xa8b3e9d0f);

  TEST_ASSERT_TRUE(lex_numeric_constant("0x234;", &tok));
  verify_integer_constant(&tok, "0x234", 0x234);
  TEST_ASSERT_TRUE(lex_numeric_constant("0x123)", &tok));
  verify_integer_constant(&tok, "0x123", 0x123);
  TEST_ASSERT_TRUE(lex_numeric_constant("0x456/123", &tok));
  verify_integer_constant(&tok, "0x456", 0x456);
  TEST_ASSERT_TRUE(lex_numeric_constant("0xabc+456", &tok));
  verify_integer_constant(&tok, "0xabc", 0xabc);
  TEST_ASSERT_TRUE(lex_numeric_constant("0xdef-456", &tok));
  verify_integer_constant(&tok, "0xdef", 0xdef);
  TEST_ASSERT_TRUE(lex_numeric_constant("0x123*456", &tok));
  verify_integer_constant(&tok, "0x123", 0x123);
  TEST_ASSERT_TRUE(lex_numeric_constant("0x345,456", &tok));
  verify_integer_constant(&tok, "0x345", 0x345);

  TEST_ASSERT_FALSE(lex_numeric_constant(";0x123", &tok));
  TEST_ASSERT_FALSE(lex_numeric_constant("foobar0x123", &tok));
  TEST_ASSERT_FALSE(lex_numeric_constant("0x123foobar", &tok));
  TEST_ASSERT_FALSE(lex_numeric_constant("__0x123", &tok));
  TEST_ASSERT_FALSE(lex_numeric_constant("__0x123__", &tok));
  TEST_ASSERT_FALSE(lex_numeric_constant("thisdoes0x0123notcount", &tok));

  TEST_ASSERT_FALSE(lex_numeric_constant("0x", &tok));
  TEST_ASSERT_FALSE(lex_numeric_constant("0X", &tok));
  TEST_ASSERT_FALSE(lex_numeric_constant("0xu", &tok));
  TEST_ASSERT_FALSE(lex_numeric_constant("0Xull", &tok));
  TEST_ASSERT_FALSE(lex_numeric_constant("0X12340x", &tok));
  TEST_ASSERT_FALSE(lex_numeric_constant("0x1234g", &tok));
  TEST_ASSERT_FALSE(lex_numeric_constant("0x37d82ag73f", &tok));

  TEST_ASSERT_FALSE(lex_numeric_constant("ull0x123", &tok));
  TEST_ASSERT_FALSE(lex_numeric_constant("0x1a23ull0x1f23", &tok));
  TEST_ASSERT_FALSE(lex_numeric_constant("0x12aull0x1b3", &tok));
  TEST_ASSERT_FALSE(lex_numeric_constant("0x123ullthisdoesnotcount", &tok));
  TEST_ASSERT_FALSE(lex_numeric_constant("0x123ullull", &tok));

  TEST_ASSERT_TRUE(lex_numeric_constant("0x674U", &tok));
  verify_integer_constant(&tok, "0x674U", 0x674);
  TEST_ASSERT_TRUE(lex_numeric_constant("0x67eU", &tok));
  verify_integer_constant(&tok, "0x67eU", 0x67e);
  TEST_ASSERT_TRUE(lex_numeric_constant("0x67fULL", &tok));
  verify_integer_constant(&tok, "0x67fULL", 0x67f);
  TEST_ASSERT_TRUE(lex_numeric_constant("0x137llU", &tok));
  verify_integer_constant(&tok, "0x137llU", 0x137);
  TEST_ASSERT_TRUE(lex_numeric_constant("0x6748ll", &tok));
  verify_integer_constant(&tok, "0x6748ll", 0x6748);
  TEST_ASSERT_TRUE(lex_numeric_constant("0x237lu", &tok));
  verify_integer_constant(&tok, "0x237lu", 0x237);
  TEST_ASSERT_TRUE(lex_numeric_constant("0x137uLL", &tok));
  verify_integer_constant(&tok, "0x137uLL", 0x137);
  TEST_ASSERT_TRUE(lex_numeric_constant("0xabcuL", &tok));
  verify_integer_constant(&tok, "0xabcuL", 0xabc);
  TEST_ASSERT_TRUE(lex_numeric_constant("0xdefU;", &tok));
  verify_integer_constant(&tok, "0xdefU", 0xdef);
  TEST_ASSERT_TRUE(lex_numeric_constant("0x12aLLU;thisdoesnotcount", &tok));
  verify_integer_constant(&tok, "0x12aLLU", 0x12a);
}

void test_lex_decimal_float(void) {
  token tok;
  memset(&tok, 0, sizeof(token));

  TEST_ASSERT_TRUE(lex_numeric_constant("1234.;", &tok));
  verify_float_constant(&tok, "1234.", 1234.);
  TEST_ASSERT_TRUE(lex_numeric_constant(".2234 ", &tok));
  verify_float_constant(&tok, ".2234", .2234);
  TEST_ASSERT_TRUE(lex_numeric_constant("3234.1234+", &tok));
  verify_float_constant(&tok, "3234.1234", 3234.1234);
  TEST_ASSERT_TRUE(lex_numeric_constant("4234.f;", &tok));
  verify_float_constant(&tok, "4234.f", 4234.);
  TEST_ASSERT_TRUE(lex_numeric_constant(".5234F ", &tok));
  verify_float_constant(&tok, ".5234F", .5234);
  TEST_ASSERT_TRUE(lex_numeric_constant("6234.1234l+", &tok));
  verify_float_constant(&tok, "6234.1234l", 6234.1234);
  TEST_ASSERT_TRUE(lex_numeric_constant("7234.L;", &tok));
  verify_float_constant(&tok, "7234.L", 7234.);

  TEST_ASSERT_TRUE(lex_numeric_constant("8234.e12;", &tok));
  verify_float_constant(&tok, "8234.e12", 8234.e12);
  TEST_ASSERT_TRUE(lex_numeric_constant(".9234E009 ", &tok));
  verify_float_constant(&tok, ".9234E009", .9234e9);
  TEST_ASSERT_TRUE(lex_numeric_constant("4334.1234e33+", &tok));
  verify_float_constant(&tok, "4334.1234e33", 4334.1234e33);
  TEST_ASSERT_TRUE(lex_numeric_constant("8234.e+12;", &tok));
  verify_float_constant(&tok, "8234.e+12", 8234.e12);
  TEST_ASSERT_TRUE(lex_numeric_constant("2234.E-12;", &tok));
  verify_float_constant(&tok, "2234.E-12", 2234.e-12);
  TEST_ASSERT_TRUE(lex_numeric_constant(".9204e+056 ", &tok));
  verify_float_constant(&tok, ".9204e+056", .9204e56);
  TEST_ASSERT_TRUE(lex_numeric_constant(".5234e-56 ", &tok));
  verify_float_constant(&tok, ".5234e-56", .5234e-56);
  TEST_ASSERT_TRUE(lex_numeric_constant("2434.1234e+33+", &tok));
  verify_float_constant(&tok, "2434.1234e+33", 2434.1234e33);
  TEST_ASSERT_TRUE(lex_numeric_constant("1236.1234e-33+", &tok));
  verify_float_constant(&tok, "1236.1234e-33", 1236.1234e-33);
  TEST_ASSERT_TRUE(lex_numeric_constant("5234.E+12f;", &tok));
  verify_float_constant(&tok, "5234.E+12f", 5234.e12);
  TEST_ASSERT_TRUE(lex_numeric_constant(".0234e-56l ", &tok));
  verify_float_constant(&tok, ".0234e-56l", .0234e-56);
  TEST_ASSERT_TRUE(lex_numeric_constant("2284.1234e+33F+", &tok));
  verify_float_constant(&tok, "2284.1234e+33F", 2284.1234e33);
  TEST_ASSERT_TRUE(lex_numeric_constant("1904.1234E-33f+", &tok));
  verify_float_constant(&tok, "1904.1234E-33f", 1904.1234e-33);

  TEST_ASSERT_TRUE(lex_numeric_constant("0.;", &tok));
  verify_float_constant(&tok, "0.", 0.0);
  TEST_ASSERT_TRUE(lex_numeric_constant(".0+", &tok));
  verify_float_constant(&tok, ".0", 0.0);
  TEST_ASSERT_TRUE(lex_numeric_constant(".0000+", &tok));
  verify_float_constant(&tok, ".0000", 0.0);
  TEST_ASSERT_TRUE(lex_numeric_constant("0.00 ", &tok));
  verify_float_constant(&tok, "0.00", 0.0);
  TEST_ASSERT_TRUE(lex_numeric_constant("0.00000f ", &tok));
  verify_float_constant(&tok, "0.00000f", 0.0);
  TEST_ASSERT_TRUE(lex_numeric_constant("0000.00000f ", &tok));
  verify_float_constant(&tok, "0000.00000f", 0.0);
  TEST_ASSERT_TRUE(lex_numeric_constant("0.00000e000f ", &tok));
  verify_float_constant(&tok, "0.00000e000f", 0.0);
  TEST_ASSERT_TRUE(lex_numeric_constant("0000123.000123L+", &tok));
  verify_float_constant(&tok, "0000123.000123L", 0000123.000123);
  TEST_ASSERT_TRUE(lex_numeric_constant("0000123.000123e000l+", &tok));
  verify_float_constant(&tok, "0000123.000123e000l", 0000123.000123e0);

  TEST_ASSERT_FALSE(lex_numeric_constant("0f ", &tok));
  TEST_ASSERT_FALSE(lex_numeric_constant("00000f ", &tok));
  TEST_ASSERT_FALSE(lex_numeric_constant("1234.a;", &tok));
  TEST_ASSERT_FALSE(lex_numeric_constant("1234.e;", &tok));
  TEST_ASSERT_FALSE(lex_numeric_constant("1234a.e12;", &tok));
  TEST_ASSERT_FALSE(lex_numeric_constant("1234.e1a2;", &tok));
  TEST_ASSERT_FALSE(lex_numeric_constant("1234.E/123;", &tok));
  TEST_ASSERT_FALSE(lex_numeric_constant("e", &tok));
  TEST_ASSERT_FALSE(lex_numeric_constant("E", &tok));
  TEST_ASSERT_FALSE(lex_numeric_constant(".", &tok));
  TEST_ASSERT_FALSE(lex_numeric_constant(".e", &tok));
  TEST_ASSERT_FALSE(lex_numeric_constant(".f", &tok));
  TEST_ASSERT_FALSE(lex_numeric_constant(".E12f", &tok));
  TEST_ASSERT_FALSE(lex_numeric_constant(".ef", &tok));
  TEST_ASSERT_FALSE(lex_numeric_constant("1234.1234e-33fa", &tok));
}

void test_lex_hex_float(void) {
  token tok;
  memset(&tok, 0, sizeof(token));

  TEST_ASSERT_TRUE(lex_numeric_constant("0x1a3E.p1;", &tok));
  verify_float_constant(&tok, "0x1a3E.p1", 0x1a3e.p1);
  TEST_ASSERT_TRUE(lex_numeric_constant("0X.2cD4p23 ", &tok));
  verify_float_constant(&tok, "0X.2cD4p23", 0x.2cd4p23);
  TEST_ASSERT_TRUE(lex_numeric_constant("0xA23b.1234p69+", &tok));
  verify_float_constant(&tok, "0xA23b.1234p69", 0xa23b.1234p69);
  TEST_ASSERT_TRUE(lex_numeric_constant("0X4e3B.p78f;", &tok));
  verify_float_constant(&tok, "0X4e3B.p78f", 0x4e3b.p78);
  TEST_ASSERT_TRUE(lex_numeric_constant("0x.5234p00F ", &tok));
  verify_float_constant(&tok, "0x.5234p00F", 0x.5234p0);
  TEST_ASSERT_TRUE(lex_numeric_constant("0Xe2B4.1234p001l+", &tok));
  verify_float_constant(&tok, "0Xe2B4.1234p001l", 0xe2b4.1234p1);
  TEST_ASSERT_TRUE(lex_numeric_constant("0x7f3E.p17L;", &tok));
  verify_float_constant(&tok, "0x7f3E.p17L", 0x7f3e.p17);

  TEST_ASSERT_TRUE(lex_numeric_constant("0xa234.p+12;", &tok));
  verify_float_constant(&tok, "0xa234.p+12", 0xa234.p12);
  TEST_ASSERT_TRUE(lex_numeric_constant("0Xe23F.p-12;", &tok));
  verify_float_constant(&tok, "0Xe23F.p-12", 0xe23f.p-12);
  TEST_ASSERT_TRUE(lex_numeric_constant("0x.920fp+56 ", &tok));
  verify_float_constant(&tok, "0x.920fp+56", 0x.920fp56);
  TEST_ASSERT_TRUE(lex_numeric_constant("0X.5c3Dp-56 ", &tok));
  verify_float_constant(&tok, "0X.5c3Dp-56", 0x.5c3dp-56);
  TEST_ASSERT_TRUE(lex_numeric_constant("0x24a4.1C34p+33+", &tok));
  verify_float_constant(&tok, "0x24a4.1C34p+33", 0x24a4.1c34p33);
  TEST_ASSERT_TRUE(lex_numeric_constant("0Xb2A6.12dFp-33+", &tok));
  verify_float_constant(&tok, "0Xb2A6.12dFp-33", 0xb2a6.12dfp-33);
  TEST_ASSERT_TRUE(lex_numeric_constant("0xe2a4.p+12f;", &tok));
  verify_float_constant(&tok, "0xe2a4.p+12f", 0xe2a4.p12);
  TEST_ASSERT_TRUE(lex_numeric_constant("0x.0dF4p-56l ", &tok));
  verify_float_constant(&tok, "0x.0dF4p-56l", 0x.0df4p-56);
  TEST_ASSERT_TRUE(lex_numeric_constant("0x2d8E.e234p+33F+", &tok));
  verify_float_constant(&tok, "0x2d8E.e234p+33F", 0x2d8e.e234p33);
  TEST_ASSERT_TRUE(lex_numeric_constant("0X1b0f.F234p-33f+", &tok));
  verify_float_constant(&tok, "0X1b0f.F234p-33f", 0x1b0f.f234p-33);

  TEST_ASSERT_TRUE(lex_numeric_constant("0x0.p0{", &tok));
  verify_float_constant(&tok, "0x0.p0", 0x0.p0);
  TEST_ASSERT_TRUE(lex_numeric_constant("0x.0p0;", &tok));
  verify_float_constant(&tok, "0x.0p0", 0x0.p0);
  TEST_ASSERT_TRUE(lex_numeric_constant("0x0.0p0;", &tok));
  verify_float_constant(&tok, "0x0.0p0", 0x0.p0);
  TEST_ASSERT_TRUE(lex_numeric_constant("0X.0p+0+", &tok));
  verify_float_constant(&tok, "0X.0p+0", 0x0.p0);
  TEST_ASSERT_TRUE(lex_numeric_constant("0X.00p00 ", &tok));
  verify_float_constant(&tok, "0X.00p00", 0x0.p0);
  TEST_ASSERT_TRUE(lex_numeric_constant("0x.0000p1+", &tok));
  verify_float_constant(&tok, "0x.0000p1", 0x.0000p1);
  TEST_ASSERT_TRUE(lex_numeric_constant("0x000.0000p-00+", &tok));
  verify_float_constant(&tok, "0x000.0000p-00", 0x000.0000p-00);
  TEST_ASSERT_TRUE(lex_numeric_constant("0x0000abc123.000abc123p12l+", &tok));
  verify_float_constant(&tok, "0x0000abc123.000abc123p12l",
                        0x0000abc123.000abc123p12);

  TEST_ASSERT_FALSE(lex_numeric_constant("0f ", &tok));
  TEST_ASSERT_FALSE(lex_numeric_constant("00000f ", &tok));
  TEST_ASSERT_FALSE(lex_numeric_constant("0x123.456;", &tok));
  TEST_ASSERT_FALSE(lex_numeric_constant("0x1a34.z;", &tok));
  TEST_ASSERT_FALSE(lex_numeric_constant("0x12b4.p;", &tok));
  TEST_ASSERT_FALSE(lex_numeric_constant("0x1a34.a1bc;", &tok));
  TEST_ASSERT_FALSE(lex_numeric_constant("0x1a34.a1bcp;", &tok));
  TEST_ASSERT_FALSE(lex_numeric_constant("0x1a34.a1bcp12ab;", &tok));
  TEST_ASSERT_FALSE(lex_numeric_constant("0x1a34.a1bcpa12;", &tok));
  TEST_ASSERT_FALSE(lex_numeric_constant("0x12z4a.p12;", &tok));
  TEST_ASSERT_FALSE(lex_numeric_constant("0x1234.p/123;", &tok));
  TEST_ASSERT_FALSE(lex_numeric_constant("p", &tok));
  TEST_ASSERT_FALSE(lex_numeric_constant(".p", &tok));
  TEST_ASSERT_FALSE(lex_numeric_constant(".p12f", &tok));
  TEST_ASSERT_FALSE(lex_numeric_constant(".pf", &tok));
  TEST_ASSERT_FALSE(lex_numeric_constant("0xp", &tok));
  TEST_ASSERT_FALSE(lex_numeric_constant("0x.", &tok));
  TEST_ASSERT_FALSE(lex_numeric_constant("0x.p", &tok));
  TEST_ASSERT_FALSE(lex_numeric_constant("0x.p123", &tok));
  TEST_ASSERT_FALSE(lex_numeric_constant("0x1234.1234p-33fa", &tok));
}

void test_lex_keyword(void) {
  char* keywords[] = {
      "auto",     "if",       "unsigned",
      "break",    "inline",   "void",
      "case",     "int",      "volatile",
      "char",     "long",     "while",
      "const",    "register", "_Alignas",
      "continue", "restrict", "_Alignof",
      "default",  "return",   "_Atomic",
      "do",       "short",    "_Bool",
      "double",   "signed",   "_Complex",
      "else",     "sizeof",   "_Generic",
      "enum",     "static",   "_Imaginary",
      "extern",   "struct",   "_Noreturn",
      "float",    "switch",   "_Static_assert",
      "for",      "typedef",  "_Thread_local",
      "goto",     "union",
  };

  token tok;
  memset(&tok, 0, sizeof(token));

  for (size_t i = 0; i < sizeof(keywords) / sizeof(keywords[0]); ++i) {
    const char* word = keywords[i];
    lex_identifier(word, &tok);
    verify_keyword(&tok, word);
  }

  TEST_ASSERT_TRUE(lex_identifier("break;", &tok));
  verify_keyword(&tok, "break");
  TEST_ASSERT_TRUE(lex_identifier("switch{this does not count}", &tok));
  verify_keyword(&tok, "switch");
  TEST_ASSERT_TRUE(lex_identifier("for(int i = 0;i < 2;++i){}", &tok));
  verify_keyword(&tok, "for");

  TEST_ASSERT_FALSE(is_keyword("breakit;", 7));
  TEST_ASSERT_FALSE(is_keyword("foobar", 5));
  TEST_ASSERT_FALSE(is_keyword("123456", 6));
}

void test_lex_punctuator(void) {
  char* punctuators[] = {
      "[",  "]",  "(",  ")",  "{",  "}",  ".",  "&",  "*",  "+",   "-",   "~",
      "!",  "/",  "%",  "<",  ">",  "^",  "|",  "?",  ":",  ";",   "=",   ",",
      "#",  "->", "++", "--", "&&", "||", "*=", "/=", "%=", "+=",  "-=",  "&=",
      "^=", "|=", "<=", ">=", "==", "!=", "<<", ">>", "##", "<<=", ">>=", "...",
  };

  token tok;
  memset(&tok, 0, sizeof(token));

  for (size_t i = 0; i < sizeof(punctuators) / sizeof(punctuators[0]); ++i) {
    const char* punct = punctuators[i];
    TEST_ASSERT_TRUE(lex_punctuator(punct, &tok));
    verify_punctuator(&tok, punct);
  }

  TEST_ASSERT_TRUE(lex_punctuator("<<=;", &tok));
  verify_punctuator(&tok, "<<=");
  TEST_ASSERT_TRUE(lex_punctuator("<<==>>", &tok));
  verify_punctuator(&tok, "<<=");
  TEST_ASSERT_TRUE(lex_punctuator("{this does not count}", &tok));
  verify_punctuator(&tok, "{");
  TEST_ASSERT_TRUE(lex_punctuator("[123456]", &tok));
  verify_punctuator(&tok, "[");
  TEST_ASSERT_TRUE(lex_punctuator("||0||1", &tok));
  verify_punctuator(&tok, "||");
  TEST_ASSERT_TRUE(lex_punctuator("||||0||1", &tok));
  verify_punctuator(&tok, "||");
  TEST_ASSERT_TRUE(lex_punctuator("<=x=>", &tok));
  verify_punctuator(&tok, "<=");
  TEST_ASSERT_TRUE(lex_punctuator("+2-3/5&6%7", &tok));
  verify_punctuator(&tok, "+");
  TEST_ASSERT_TRUE(lex_punctuator("(+2-3)/(5&6)%7", &tok));
  verify_punctuator(&tok, "(");

  TEST_ASSERT_FALSE(lex_punctuator("123456", &tok));
  TEST_ASSERT_FALSE(lex_punctuator("foobar", &tok));
  TEST_ASSERT_FALSE(lex_punctuator("x;(12+34)", &tok));
}

void test_char_constant(void) {
  token tok;
  memset(&tok, 0, sizeof(token));

  TEST_ASSERT_TRUE(lex_char_literal("'a'", &tok));
  verify_char_constant(&tok, "'a'", 'a');
  TEST_ASSERT_TRUE(lex_char_literal("'\\a'", &tok));
  verify_char_constant(&tok, "'\\a'", '\a');
  TEST_ASSERT_TRUE(lex_char_literal("'\\b'", &tok));
  verify_char_constant(&tok, "'\\b'", '\b');
  TEST_ASSERT_TRUE(lex_char_literal("'\\e'", &tok));
  verify_char_constant(&tok, "'\\e'", /* '\e' */ '\033');
  TEST_ASSERT_TRUE(lex_char_literal("'\\f'", &tok));
  verify_char_constant(&tok, "'\\f'", '\f');
  TEST_ASSERT_TRUE(lex_char_literal("'\\n'", &tok));
  verify_char_constant(&tok, "'\\n'", '\n');
  TEST_ASSERT_TRUE(lex_char_literal("'\\t'", &tok));
  verify_char_constant(&tok, "'\\t'", '\t');
  TEST_ASSERT_TRUE(lex_char_literal("'\\v'", &tok));
  verify_char_constant(&tok, "'\\v'", '\v');
  TEST_ASSERT_TRUE(lex_char_literal("'\\\\'", &tok));
  verify_char_constant(&tok, "'\\\\'", '\\');
  TEST_ASSERT_TRUE(lex_char_literal("'\\\''", &tok));
  verify_char_constant(&tok, "'\\\''", '\'');
  TEST_ASSERT_TRUE(lex_char_literal("'\\\"'", &tok));
  verify_char_constant(&tok, "'\\\"'", '\"');
  TEST_ASSERT_TRUE(lex_char_literal("'\\?'", &tok));
  verify_char_constant(&tok, "'\\?'", '\?');
  TEST_ASSERT_TRUE(lex_char_literal("'\\123'", &tok));
  verify_char_constant(&tok, "'\\123'", '\123');
  TEST_ASSERT_TRUE(lex_char_literal("'\\xcf'", &tok));
  verify_char_constant(&tok, "'\\xcf'", '\xcf');

  TEST_ASSERT_TRUE(lex_char_literal("u'\\xcdef'", &tok));
  verify_char16_constant(&tok, "u'\\xcdef'", u'\xcdef');
  TEST_ASSERT_TRUE(lex_char_literal("u'\xe4\xb8\x96'", &tok));
  verify_char16_constant(&tok, "u'\xe4\xb8\x96'", u'世');
  TEST_ASSERT_TRUE(lex_char_literal("U'\\xabcdef12'", &tok));
  verify_char32_constant(&tok, "U'\\xabcdef12'", U'\xabcdef12');
  TEST_ASSERT_TRUE(lex_char_literal("L'\\xa3456789'", &tok));
  verify_char32_constant(&tok, "L'\\xa3456789'", L'\xa3456789');

  TEST_ASSERT_TRUE(lex_char_literal("'abc'", &tok));
  verify_char_constant(&tok, "'abc'", 'c');
  TEST_ASSERT_TRUE(lex_char_literal("'\\z'", &tok));
  verify_char_constant(&tok, "'\\z'", 'z');
  TEST_ASSERT_TRUE(lex_char_literal("'\\z\\s\\u'", &tok));
  verify_char_constant(&tok, "'\\z\\s\\u'", 'u');
  TEST_ASSERT_TRUE(lex_char_literal("'\\123\\123'", &tok));
  verify_char_constant(&tok, "'\\123\\123'", '\123');
  TEST_ASSERT_TRUE(lex_char_literal("'\\xab\\xcd'", &tok));
  verify_char_constant(&tok, "'\\xab\\xcd'", '\xcd');

  TEST_ASSERT_FALSE(lex_char_literal("''", &tok));
}

void test_string_literal(void) {
  token tok;
  memset(&tok, 0, sizeof(token));

  TEST_ASSERT_TRUE(lex_string_literal("\"Hello, world!\"", &tok));
  verify_string(&tok, "\"Hello, world!\"", "Hello, world!");
  TEST_ASSERT_TRUE(lex_string_literal("\"He\\tllo, \\nworl\\fd!\"", &tok));
  verify_string(&tok, "\"He\\tllo, \\nworl\\fd!\"", "He\tllo, \nworl\fd!");
  TEST_ASSERT_TRUE(lex_string_literal("\"Hi\\123\\123Hi\"", &tok));
  verify_string(&tok, "\"Hi\\123\\123Hi\"", "Hi\123\123Hi");
  TEST_ASSERT_TRUE(lex_string_literal("u8\"Hello, world!\"", &tok));

  // The cast is just to make compiler warnings happy. No real impacts.
  verify_utf8_string(&tok, "u8\"Hello, world!\"",
                     (const uint8_t*)u8"Hello, world!");
  TEST_ASSERT_TRUE(lex_string_literal("u8\"\\xaa\\xabHi\"", &tok));
  verify_utf8_string(&tok, "u8\"\\xaa\\xabHi\"",
                     (const uint8_t*)u8"\xaa\xabHi");

  TEST_ASSERT_TRUE(lex_string_literal("u\"Hi\\xaabb\\x12Hi\"", &tok));
  verify_utf16_string(&tok, "u\"Hi\\xaabb\\x12Hi\"", u"Hi\xaabb\x12Hi");
  TEST_ASSERT_TRUE(lex_string_literal("u\"\xe4\xb8\x96\xe7\x95\x8c\"", &tok));
  verify_utf16_string(&tok, "u\"\xe4\xb8\x96\xe7\x95\x8c\"", u"世界");

  TEST_ASSERT_TRUE(lex_string_literal("U\"Hello\\xabcdef12\\x1235678\"", &tok));
  verify_utf32_string(&tok, "U\"Hello\\xabcdef12\\x1235678\"",
                      U"Hello\xabcdef12\x1235678");
  TEST_ASSERT_TRUE(lex_string_literal("U\"\xe4\xb8\x96\xe7\x95\x8c\"", &tok));
  verify_utf32_string(&tok, "U\"\xe4\xb8\x96\xe7\x95\x8c\"", U"世界");
  TEST_ASSERT_TRUE(
      lex_string_literal("L\"Hello\\xabcdef12\\x1234578world\"", &tok));
  verify_utf32_string(&tok, "L\"Hello\\xabcdef12\\x1234578world\"",
                      (const uint32_t*)L"Hello\xabcdef12\x1234578world");

  TEST_ASSERT_TRUE(lex_string_literal("\"\"", &tok));
  verify_string(&tok, "\"\"", "");
  TEST_ASSERT_TRUE(lex_string_literal("u8\"\"", &tok));
  verify_string(&tok, "u8\"\"", "");
  TEST_ASSERT_TRUE(lex_string_literal("u\"\"", &tok));
  verify_string(&tok, "u\"\"", "");
  TEST_ASSERT_TRUE(lex_string_literal("U\"\"", &tok));
  verify_string(&tok, "U\"\"", "");
  TEST_ASSERT_TRUE(lex_string_literal("L\"\"", &tok));
  verify_string(&tok, "L\"\"", "");
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_lex_identifier);
  RUN_TEST(test_lex_decimal_integer);
  RUN_TEST(test_lex_octal_integer);
  RUN_TEST(test_lex_hex_integer);
  RUN_TEST(test_lex_decimal_float);
  RUN_TEST(test_lex_hex_float);
  RUN_TEST(test_lex_keyword);
  RUN_TEST(test_lex_punctuator);
  RUN_TEST(test_char_constant);
  RUN_TEST(test_string_literal);
  return UNITY_END();
}
