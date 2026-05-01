//!@file
//!@brief Header file for the lexer.

#ifndef SKIBICC_LEXER_H
#define SKIBICC_LEXER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "array.h"

//! Represents types of tokens.
typedef enum token_type {
  //! Unknown token.
  TK_UNKNOWN,
  //! Keyword.
  TK_KEYWRD,
  //! Identifier.
  TK_IDENT,
  //! Integer constant, including character literals.
  TK_ICONST,
  //! Floating point constant.
  TK_FCONST,
  //! String literal.
  TK_STRLIT,
  //! Punctuator
  TK_PUNCT,
} token_type;

// !Represents a single lexical token.
typedef struct token {
  //! Type of the token.
  token_type token_type;
  //! Value of the constant/literal.
  union constant {
    //! Value of the integer constant. Set if `token_type` is `TK_ICONST`.
    uint64_t int_val;
    //! Value of the floating point constant. Set if `token_type` is
    //! `TK_FCONST`.
    long double float_val;
    //! Value of the string literal. Set if `token_type` is `TK_STRLIT`.
    char* str_val;
  } constant;

  //! Pointer to the starting character of the token in the source code buffer.
  const char* loc;
  //! Length of the string that constitutes the token.
  size_t size;
  //! Line number of the token in the source code.
  size_t line_num;
  //! Column number of the token in the source code.
  size_t col_num;
} token;

//! Reads a file from `path` and returns its content.
char* read_file(char* path);

//! Given a source code buffer `s`, returns an array of tokens.
array lex(const char* s);

// ------------ Exposed for testing only ------------

//! Returns true if there is an identifier token starting at the character
//! pointed to by `s`, and populates `tok` as an identifier. Otherwise returns
//! false, and `tok` is not changed.
bool lex_identifier(const char* s, token* tok);

//! Returns true if the string starting at `s` of length `len` is a keyword.
//! Otherwise returns false. It is assumed that `s` must be an identifier.
bool is_keyword(const char* s, size_t len);

//! Returns true if there is a numerical constant (integer and float) token
//! starting at the character pointed to by `s`, and populates `tok` as an
//! numerica constant. Otherwise returns false, and `tok` is not changed.
bool lex_numeric_constant(const char* s, token* tok);

//! Returns true if there is a punctuator token starting at the character
//! pointed to by `s`, and populates `tok` as an punctuator. Otherwise returns
//! false, and `tok` is not changed.
bool lex_punctuator(const char* s, token* tok);

//! Returns true if there is a character literal token starting at the character
//! pointed to by `s`, and populates `tok` as an character literal. Otherwise
//! returns false, and `tok` is not changed.
//!
//! Some notes on the form of character literal:
//! - If preceded by 'L', 'u' or 'U', it is a wide char and can only have 1
//! character.
//! - No prefix: UTF-8 character. 'u' prefix: UTF-16 character. 'U' or 'L'
//! prefix: UTF-32 character.
//! - For non-wide char, multiple characters like like '1234' 'abcd' 'asdf\x214'
//! are supported. They are translated into integer byte by byte, but only the
//! last 4 bytes will be kept.
//! - Unsupported escape sequences will get translated into the character after
//! the slash, so for example given '\o', it is translated into 'o'.
bool lex_char_literal(const char* s, token* tok);

//! Returns the length of the string literal starting at the character pointed
//! to by `s`. Returns 0 if it is not a string literal.
//!
//! - 'u8' prefix or no prefix: UTF-8 string. 'u' prefix: UTF-16 string. 'U' or
//! 'L' prefix: UTF-32 string.
//! - Unsupported escape sequences will get translated into the character after
//! the slash, so for example given '\o', it is translated into 'o'.
bool lex_string_literal(const char* s, token* tok);

array lex(const char* s);

#endif  // SKIBICC_LEXER_H
