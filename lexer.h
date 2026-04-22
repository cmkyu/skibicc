#ifndef SKIBICC_LEXER_H
#define SKIBICC_LEXER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum token_type {
  TK_UNKNOWN,
  TK_KEYWRD,
  TK_IDENT,
  TK_ICONST,
  TK_FCONST,
  TK_STRLIT,
  TK_PUNCT,
} token_type;

typedef struct token {
  token_type token_type;
  union constant {
    uint64_t int_val;
    long double float_val;
    char* str_val;
  } constant;

  const char* loc;
  size_t size;
  size_t line_num;
  size_t col_num;
} token;

//! Reads a file from `path` and returns its content.
char* read_file(char* path);

//! Returns the length of the identifier starting at the character pointed to
//! by `s`. Returns 0 if it is not an identifier.
bool lex_identifier(const char* s, token* tok);

//! Returns true if the string starting at `s` of length `len` is a keyword.
//! Otherwise returns false. It is assumed that `s` must be an identifier, i.e.,
//! lex_identifier(s) > 0.
bool is_keyword(const char* s, size_t len);

//! Returns the length of the numerical constant (integer and float) starting at
//! the character pointed to by `s`. Returns 0 if it is not a constant.
bool lex_numeric_constant(const char* s, token* tok);

//! Returns the length of the punctuator starting at the character pointed to by
//! `s`. Returns 0 if it is not a punctuator.
//! Digraphs and trigrpans are not supported.
bool lex_punctuator(const char* s, token* tok);

//! Returns the length of the character constant starting at the character
//! pointed to by `s`. Returns 0 if it is not a character constant.
//!
//! Some notes on the form of character literal:
//! - If preceded by 'L', 'u' or 'U', it is a wide char and can only have 1
//! character.
//! - For non-wide char, multiple characters like like '1234' 'abcd' 'asdf\x214'
//! are supported. They are translated into integer byte by byte.
//! - Unsupported escape sequences will get translated into the character after
//! the slash, so for example given '\o', it is translated into 'o'.
uint64_t lex_char_constant(const char* s, token* tok);

//! Returns the length of the string literal starting at the character pointed
//! to by `s`. Returns 0 if it is not a string literal.
//! - Unsupported escape sequences will get translated into the character after
//! the slash, so for example given '\o', it is translated into 'o'.
uint64_t lex_string_literal(const char* s);

#endif  // SKIBICC_LEXER_H
