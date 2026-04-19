//!@file
//!@brief Source file for the lexer.

#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "hashmap.h"

char* read_file(char* path) {
  FILE* f = fopen(path, "r");
  if (!f) {
    fprintf(stderr, "readfile(): failed to open file: %s.\n", path);
    return NULL;
  }

  char* out_buf;
  size_t out_size;
  FILE* out_stream = open_memstream(&out_buf, &out_size);
  if (!out_stream) {
    fprintf(stderr, "readfile(): failed to create output stream.\n");
    fclose(f);
    return NULL;
  }

  while (true) {
    char buf[4096];
    int res = fread(buf, 1, sizeof(buf), f);
    if (res == 0) {
      break;
    }
    fwrite(buf, 1, res, out_stream);
  }
  fflush(out_stream);
  fclose(out_stream);
  fclose(f);
  return out_buf;
}

// TODO: Remember lexing order: String literals and constants must come before
// punctuators. This is because string literals and constants may contain
// characters from punctuators.

//! Returns true if `c` matches [a-zA-Z_]. Otherwise returns false.
static bool is_word_char(char c) { return isalnum(c) || c == '_'; }

uint64_t lex_identifier(const char* s) {
  if (!isalpha(*s) && *s != '_') {
    // TODO: these checks should really be moved to the lexer main loop. Then,
    // if the lex_ methods returns 0, we can emit a nice error message.
    return 0;
  }

  const char* start = s;
  ++s;
  while (true) {
    char c = *s;
    if (!is_word_char(c)) {
      break;
    }
    ++s;
  }
  return s - start;
}

//! C keywords.
static char* KEYWORDS[] = {
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

//! Size of `KEYWORDS`.
static const size_t KEYWORDS_SIZE = sizeof(KEYWORDS) / sizeof(KEYWORDS[0]);

//! Hashmap that stores all the keywords inside `KEYWORDS`. Initialized when
//! `is_keyword` is first called.
static hashmap keywords_map;

bool is_keyword(const char* s, size_t len) {
  if (keywords_map.size == 0) {
    hashmap_init(&keywords_map);
    hashmap_entry entry = {.data = NULL, .data_size = 0};
    for (size_t i = 0; i < KEYWORDS_SIZE; ++i) {
      entry.key = KEYWORDS[i];
      entry.key_size = strlen(KEYWORDS[i]);
      hashmap_insert(&keywords_map, &entry);
    }
  }

  return hashmap_get(&keywords_map, s, len);
}

//! u or U; l or L; ll or LL; Both u or U and l or L; Both u or U and ll or LL.
//! Shorter suffixes may be prefixes of longer ones; we put the longest first so
//! that we match them first.
static char* INT_SUFFIXES[] = {
    "uLL", "ull", "ULL", "Ull", "LLu", "llu", "LLU", "llU", "ul", "uL", "Ul",
    "UL",  "lu",  "Lu",  "lU",  "LU",  "ll",  "LL",  "u",   "U",  "l",  "L",
};

//! Size of `INT_SUFFIXES`.
static const size_t INT_SUFFIXES_SIZE =
    sizeof(INT_SUFFIXES) / sizeof(INT_SUFFIXES[0]);

//! If `s` matches any integer suffixes, returns `s` after skipping the suffix.
//! If there is no suffix at all, returns `s` as-is. If the suffix is invalid,
//! returns `NULL`.
//! Example:
//! Valid suffix: 1234ull;
//! No suffix at all: 1234;
//! Invalid suffix: 1234ulla (integer must end at word boundary, i.e.,
//! must be followed by a character for whom is_word_char() is false)
static const char* consume_int_suffix(const char* s) {
  char c = tolower(*s);
  if (c != 'u' && c != 'l') {
    // Integer must end at word boundary.
    return is_word_char(*s) ? NULL : s;
  }

  for (size_t i = 0; i < INT_SUFFIXES_SIZE; ++i) {
    char* suffix = INT_SUFFIXES[i];
    size_t len = strlen(suffix);
    if (strncmp(suffix, s, len) == 0) {
      s += len;
      break;
    }
  }
  // Integer must end at word boundary.
  return is_word_char(*s) ? NULL : s;
}

//! Returns 1 if `c` matches [a-fA-F0-9], otherwise returns 0.
static int is_hex_digit(int c) {
  return isdigit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

//! Returns true if `c` is the character 'p' or 'P'. Otherwise returns false.
static bool is_hex_exp_char(char c) { return tolower(c) == 'p'; }

//! If `s` matches the exponent part (excluding the 'e'/'p') of a decimal/hex
//! float, return `s` after skipping the exponent part. Returns `NULL` if it is
//! not an exponent part.
static const char* consume_exponent(const char* s) {
  if (*s == '+' || *s == '-') {
    ++s;
  }
  if (!isdigit(*s)) {
    return NULL;
  }
  while (isdigit(*s)) {
    ++s;
  }
  return s;
}

//! If `s` matches any float suffixes, returns `s` after skipping the suffix.
//! If there is no suffix at all, returns `s` as-is. If the suffix is invalid,
//! returns `NULL`.
//! Example:
//! Valid suffix: 12.34f;
//! No suffix at all: 12.34;
//! Invalid suffix: 12.34fa (float must end at word boundary, i.e., must be
//! followed by a character for whom is_word_char() is false)
static const char* consume_float_suffix(const char* s) {
  char c = tolower(*s);
  if (c == 'f' || c == 'l') {
    ++s;
  }
  return is_word_char(*s) ? NULL : s;
}

//! If `s` matches a hex integer or float, returns `s` after skipping the hex
//! number. Returns `NULL` if `s` is not a valid hex number.
static const char* consume_hex(const char* s) {
  if (!is_hex_digit(*s) && *s != '.') {
    return NULL;
  }
  if (s[0] == '.' && !is_hex_digit(s[1])) {
    return NULL;
  }

  while (is_hex_digit(*s)) {
    ++s;
  }
  if (*s != '.' && !is_hex_exp_char(*s)) {
    // Integer
    return consume_int_suffix(s);
  }
  // Float
  if (*s == '.') {
    ++s;
    while (is_hex_digit(*s)) {
      ++s;
    }
  }
  // Hex float requires an exponent.
  if (!is_hex_exp_char(*s)) {
    return NULL;
  }
  ++s;
  s = consume_exponent(s);
  return s ? consume_float_suffix(s) : s;
}

//! Returns 1 if `c` matches [0-8], otherwise returns 0.
static int is_oct_digit(int c) { return c >= '0' && c <= '8'; }

//! Returns true if `c` is the character 'e' or 'E'. Otherwise returns false.
static bool is_dec_exp_char(char c) { return tolower(c) == 'e'; }

//! If `s` matches a decimal integer, a decimal float or an octal integer,
//! returns `s` after skipping the number. Returns `NULL` if `s` is not a valid
//! number.
static const char* consume_dec_or_oct(const char* s) {
  if (!isdigit(*s) && *s != '.') {
    return NULL;
  }
  if (s[0] == '.' && !isdigit(s[1])) {
    return NULL;
  }

  const char* start = s;
  bool has_invalid_oct = false;
  while (isdigit(*s)) {
    has_invalid_oct |= !is_oct_digit(*s);
    ++s;
  }
  if (*s != '.' && !is_dec_exp_char(*s)) {
    // Decimal or octal integer
    if (*start == '0' && has_invalid_oct) {
      return NULL;
    }
    return consume_int_suffix(s);
  }
  // Float
  if (*s == '.') {
    ++s;
    while (isdigit(*s)) {
      ++s;
    }
  }
  if (is_dec_exp_char(*s)) {
    ++s;
    s = consume_exponent(s);
  }
  return s ? consume_float_suffix(s) : s;
}

uint64_t lex_numeric_constant(const char* s) {
  const char* start = s;
  if (s[0] == '0' && tolower(s[1]) == 'x') {
    s = consume_hex(s + 2);
  } else {
    s = consume_dec_or_oct(s);
  }
  return s ? s - start : 0;
}

//! Order matters here. For fast lookup we want the most common punctuators to
//! be at the front, but if it is a prefix of another punctuator, it should be
//! placed behind that punctuator.
static char* PUNCTUATORS[] = {
    ";",  "{",  "}",  "[",   "]",  "(",  ")", ",",   "==", "=",  "...", ".",
    "++", "+=", "+",  "--",  "->", "-=", "-", "*=",  "*",  "/=", "/",   "%=",
    "%",  "?",  ":",  "<<=", "<<", "<=", "<", ">>=", ">>", ">=", ">",   "!=",
    "!",  "&&", "&=", "&",   "||", "|=", "|", "^=",  "^",  "~",  "##",  "#",
};

//! Size of `PUNCTUATORS`.
const size_t PUNCTUATORS_SIZE = sizeof(PUNCTUATORS) / sizeof(PUNCTUATORS[0]);

uint64_t lex_punctuator(const char* s) {
  if (!ispunct(*s)) {
    return 0;
  }
  for (size_t i = 0; i < PUNCTUATORS_SIZE; ++i) {
    const char* punct = PUNCTUATORS[i];
    size_t len = strlen(punct);
    if (strncmp(s, punct, len) == 0) {
      return len;
    }
  }
  return 0;
}

// Returns true if `c` is a character literal prefix. Otherwise returns false.
static bool is_char_prefix(char c) { return c == 'L' || tolower(c) == 'u'; }

// Returns true if `c` is a valid escape character. Otherwise returns false.
static bool is_escape_char(char c) {
  return c == '\'' || c == '\"' || c == '?' || c == '\\' || c == 'a' ||
         c == 'b' || c == 'f' || c == 'n' || c == 'r' || c == 't' || c == 'v';
}

//! If `s` matches an escape sequence (characters after the slash '\'), returns
//! `s` after skipping the sequence. Returns `NULL` if `s` contains an invalid
//! hex escape sequence. It is assumed that `s` does not start with the slash
//! '\'. It is guaranteed that the return result is either `NULL` or `s`
//! advanced by at least 1 character.
//!
//! Some details on escape sequences:
//! '\t', '\n', etc are legitimate escape sequences. We skip 1 character. We
//! return `s` + 1.
//! '\0', '\123', etc are legitimate octal escape sequences. We skip all digits.
//! Sequences like '\v', '\o', '\wyz', ... does not contain supported escape
//! characters, but we still treat them as if they are escape characters. We
//! skip 1 character and return `s` + 1.
//! Sequences like '\1234', '\0000', '\129', '\9',... are not considered as
//! octal escape sequence, we treat them like non-supported escape character and
//! return `s` + 1.
//! Sequences like '\x', '\xabc', '\xyz' are invalid hex escape sequences (they
//! must contain at least 1 and at most 2 hex digits after 'x'), we return
//! `NULL`.
static const char* consume_escape_sequence(const char* s) {
  if (is_escape_char(*s)) {
    ++s;
    return s;
  }
  if (*s == 'x') {
    ++s;
    size_t len = 0;
    while (is_hex_digit(*s)) {
      ++s;
      ++len;
      if (len > 2) {
        // At most 2 hex digits are allowed.
        return NULL;
      }
    }
    // No hex digits is an error.
    return len == 0 ? NULL : s;
  }
  if (!is_oct_digit(*s)) {
    // TODO: this would be a good place to issue a warning about unknown escape
    // sequence. This is a case of "weird" escape character. So given something
    // like '\p', '\9', we should translate it into 'p' and '9'.
    return s + 1;
  }
  const char* start = s;
  size_t len = 0;
  while (is_oct_digit(*s)) {
    ++s;
    ++len;
    if (len > 3) {
      // Not an octal escape sequence if we have no or >3 octal digits.
      // TODO: Remember if we reach here, this is also a case of "weird" escape
      // character. So given something like '\1234', we should translate it into
      // '1234'.
      return start + 1;
    }
  }
  return s;
}

//! If `s` matches the "body" of a character literal, that is, the content
//! between the single quotes ("'"), returns `s` after skipping the body (note
//! that it does NOT skip the terminating single quote). Returns `NULL` if `s`
//! contains no character body, or if `s` does not have a terminating single
//! quote character, or if `s` contains an invalid hex escape sequence. It is
//! assumed that `s` does not start with a single quote.
static const char* consume_char_body(const char* s) {
  if (*s == '\'') {
    // Empty char body.
    return NULL;
  }
  // Consume the rest of the characters.
  while (*s != '\0' && *s != '\n') {
    if (*s == '\'') {
      // End of char literal.
      return s;
    }
    if (*s == '\\') {
      ++s;
      s = consume_escape_sequence(s);
      if (!s) {
        return NULL;
      }
      continue;
    }
    ++s;
  }
  // Unterminated char literal.
  return NULL;
}

//! If `s` matches the "body" of a wide character literal, that is, the content
//! between the single quotes ("'"), returns `s` after skipping the body (note
//! that it does NOT skip the terminating single quote). Returns `NULL` if `s`
//! contains no character body, or if `s` does not have a terminating single
//! quote character, or if `s` contains more than 1 character. It is assumed
//! that `s` does not start with a single quote.
static const char* consume_wide_char_body(const char* s) {
  if (*s == '\'' || *s == '\0' || *s == '\n') {
    // Empty char body.
    return NULL;
  }

  if (*s == '\\') {
    ++s;
    s = consume_escape_sequence(s);
  } else {
    ++s;
  }
  // Wide char cannot have more than 1 character.
  return *s == '\'' ? s : NULL;
}

uint64_t lex_char_constant(const char* s) {
  const char* start = s;
  if (is_char_prefix(s[0]) && s[1] == '\'') {
    s += 2;
    // Consume wide char. Only 1 single character is allowed.
    s = consume_wide_char_body(s);
    // If `s` is NULL, we return NULL. Otherwise we skip the terminating single
    // quote "'". It is guaranteed to exist due to the check in
    // `consume_wide_char_body`.
    return s ? s + 1 - start : 0;
  }

  if (*s != '\'') {
    return 0;
  }
  ++s;
  s = consume_char_body(s);
  if (!s) {
    return 0;
  }
  // Skip the terminating single quote "'". It is guaranteed to exist due to the
  // check in `consume_char_body`.
  ++s;
  return s - start;
}

typedef enum token_type {
  TK_KW,
  TK_IDENT,
  TK_CONST,
  TK_STRLIT,
  TK_PUNCT,
} token_type;

typedef enum constant_type {
  CT_INT,
  CT_FLOAT,
} constant_type;

struct token {
  token_type token_type;
  constant_type constant_type;
  union constant {
    int64_t int_val;
    double float_val;
  } constant;
  char* loc;
  size_t size;
};
