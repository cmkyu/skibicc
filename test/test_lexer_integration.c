#include <stdlib.h>
#include <string.h>

#include "../array.h"
#include "../lexer.h"
#include "../unity/unity.h"

void setUp(void) {}

void tearDown(void) {}

void add_token(array* tokens, token_type type) {
  token* dst = (token*)array_push_back(tokens);
  dst->token_type = type;
}

array get_expected_tokens(void) {
  array tokens;
  array_init(&tokens, sizeof(token));

  // Declaration of printf
  add_token(&tokens, TK_KEYWRD);  // void
  add_token(&tokens, TK_IDENT);   // printf
  add_token(&tokens, TK_PUNCT);   // (
  add_token(&tokens, TK_KEYWRD);  // const
  add_token(&tokens, TK_KEYWRD);  // char
  add_token(&tokens, TK_PUNCT);   // *
  add_token(&tokens, TK_IDENT);   // format
  add_token(&tokens, TK_PUNCT);   // ,
  add_token(&tokens, TK_PUNCT);   // ...
  add_token(&tokens, TK_PUNCT);   // )
  add_token(&tokens, TK_PUNCT);   // ;

  // Main function
  add_token(&tokens, TK_KEYWRD);  // int
  add_token(&tokens, TK_IDENT);   // main
  add_token(&tokens, TK_PUNCT);   // (
  add_token(&tokens, TK_KEYWRD);  // int
  add_token(&tokens, TK_IDENT);   // argc
  add_token(&tokens, TK_PUNCT);   // ,
  add_token(&tokens, TK_KEYWRD);  // char
  add_token(&tokens, TK_PUNCT);   // *
  add_token(&tokens, TK_IDENT);   // argv
  add_token(&tokens, TK_PUNCT);   // [
  add_token(&tokens, TK_PUNCT);   // ]
  add_token(&tokens, TK_PUNCT);   // )
  add_token(&tokens, TK_PUNCT);   // {
  add_token(&tokens, TK_IDENT);   // printf
  add_token(&tokens, TK_PUNCT);   // (
  add_token(&tokens, TK_STRLIT);  // "Hello, world!"
  add_token(&tokens, TK_PUNCT);   // )
  add_token(&tokens, TK_PUNCT);   // ;
  add_token(&tokens, TK_KEYWRD);  // return
  add_token(&tokens, TK_ICONST);  // 0
  add_token(&tokens, TK_PUNCT);   // ;
  add_token(&tokens, TK_PUNCT);   // }
  return tokens;
}

void test_lexer_basic(void) {
  char* buf = read_file("./test/data/lexer_test_1.c");
  TEST_ASSERT_TRUE(buf);

  array tokens = lex(buf);
  array expected_tokens = get_expected_tokens();
  char* expected_texts[] = {"void",   "printf", "(",
                            "const",  "char",   "*",
                            "format", ",",      "...",
                            ")",      ";",      "int",
                            "main",   "(",      "int",
                            "argc",   ",",      "char",
                            "*",      "argv",   "[",
                            "]",      ")",      "{",
                            "printf", "(",      "\"Hello, world!\"",
                            ")",      ";",      "return",
                            "0",      ";",      "}"};

  TEST_ASSERT_EQUAL(expected_tokens.size, tokens.size);
  for (size_t i = 0; i < expected_tokens.size; ++i) {
    token* expected_token = array_at(&expected_tokens, i);
    token* actual_token = array_at(&tokens, i);
    TEST_ASSERT_EQUAL(expected_token->token_type, actual_token->token_type);

    const char* expected_text = expected_texts[i];
    TEST_ASSERT_EQUAL(strlen(expected_text), actual_token->size);
    TEST_ASSERT_EQUAL_STRING_LEN(expected_text, actual_token->loc,
                                 actual_token->size);
  }

  free(buf);
  array_destroy(&tokens);
  array_destroy(&expected_tokens);
}

void test_lexer_bootstrap(void) {
  // Can we lex our own main c file without crashing?
  char* buf = read_file("./test/data/lexer_test_2.c");
  TEST_ASSERT_TRUE(buf);
  array tokens = lex(buf);
  free(buf);
  array_destroy(&tokens);
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_lexer_basic);
  RUN_TEST(test_lexer_bootstrap);
  return UNITY_END();
}
