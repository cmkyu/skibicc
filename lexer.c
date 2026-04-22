//!@file
//!@brief Source file for the lexer.

#include "lexer.h"

#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "array.h"
#include "errors.h"
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

bool lex_identifier(const char* s, token* tok) {
  if (!isalpha(*s) && *s != '_') {
    return false;
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
  tok->token_type = TK_IDENT;
  tok->loc = start;
  tok->size = s - start;
  return true;
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
//!
//! Examples:
//! - Valid suffix: 1234ull;
//! - No suffix at all: 1234;
//! - Invalid suffix: 1234ulla (integer must end at word boundary, i.e.,
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
//!
//! Examples:
//! - Valid suffix: 12.34f;
//! - No suffix at all: 12.34;
//! - Invalid suffix: 12.34fa (float must end at word boundary, i.e., must be
//! followed by a character for whom is_word_char() is false)
static const char* consume_float_suffix(const char* s) {
  char c = tolower(*s);
  if (c == 'f' || c == 'l') {
    ++s;
  }
  return is_word_char(*s) ? NULL : s;
}

//! Assigns the value of `exp` to `x`. If `exp` evaluates to NULL, returns NULL.
#define ASSIGN_OR_RETURN(x, exp) \
  do {                           \
    x = exp;                     \
    if (!x) {                    \
      return NULL;               \
    }                            \
  } while (0)

//! If `s` matches a hex integer or float, returns `s` after skipping the hex
//! number. Returns `NULL` if `s` is not a valid hex number. It is assumed that
//! `s` starts with a valid hex prefix "0x" or "0X".
static const char* consume_hex(const char* s, token* tok) {
  const char* start = s;
  s += 2;
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
    ASSIGN_OR_RETURN(s, consume_int_suffix(s));
    tok->token_type = TK_ICONST;
    // TODO: check for overflow by comparing str_end with s before
    // consume_int_suffix.
    tok->constant.int_val = strtoull(start, NULL, /*__base=*/16);
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
  ASSIGN_OR_RETURN(s, consume_exponent(s));
  ASSIGN_OR_RETURN(s, consume_float_suffix(s));
  tok->token_type = TK_FCONST;
  tok->constant.float_val = strtold(start, NULL);
  return s;
}

//! Returns 1 if `c` matches [0-7], otherwise returns 0.
static int is_oct_digit(int c) { return c >= '0' && c <= '7'; }

//! Returns true if `c` is the character 'e' or 'E'. Otherwise returns false.
static bool is_dec_exp_char(char c) { return tolower(c) == 'e'; }

//! If `s` matches a decimal integer, a decimal float or an octal integer,
//! returns `s` after skipping the number. Returns `NULL` if `s` is not a valid
//! number.
static const char* consume_dec_or_oct(const char* s, token* tok) {
  if (!isdigit(*s) && *s != '.') {
    return NULL;
  }
  if (s[0] == '.' && !isdigit(s[1])) {
    return NULL;
  }

  const char* start = s;
  bool is_oct = *start == '0';
  bool has_invalid_oct = false;
  while (isdigit(*s)) {
    has_invalid_oct |= !is_oct_digit(*s);
    ++s;
  }
  if (*s != '.' && !is_dec_exp_char(*s)) {
    // Decimal or octal integer
    if (is_oct && has_invalid_oct) {
      return NULL;
    }
    ASSIGN_OR_RETURN(s, consume_int_suffix(s));
    tok->token_type = TK_ICONST;
    tok->constant.int_val = strtoull(start, NULL, /*__base=*/is_oct ? 8 : 10);
    return s;
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
    ASSIGN_OR_RETURN(s, consume_exponent(s));
  }
  ASSIGN_OR_RETURN(s, consume_float_suffix(s));
  tok->token_type = TK_FCONST;
  tok->constant.float_val = strtold(start, NULL);
  return s;
}

bool lex_numeric_constant(const char* s, token* tok) {
  const char* start = s;
  if (s[0] == '0' && tolower(s[1]) == 'x') {
    s = consume_hex(s, tok);
  } else {
    s = consume_dec_or_oct(s, tok);
  }
  if (!s) {
    return false;
  }

  tok->loc = start;
  tok->size = s - start;
  return true;
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

bool lex_punctuator(const char* s, token* tok) {
  if (!ispunct(*s)) {
    return false;
  }
  for (size_t i = 0; i < PUNCTUATORS_SIZE; ++i) {
    const char* punct = PUNCTUATORS[i];
    size_t len = strlen(punct);
    if (strncmp(s, punct, len) == 0) {
      tok->token_type = TK_PUNCT;
      tok->loc = s;
      tok->size = len;
      return len;
    }
  }
  return false;
}

//! Returns true if `c` is a character literal prefix. Otherwise returns false.
static bool is_char_prefix(char c) { return c == 'L' || tolower(c) == 'u'; }

//! Returns true if `c` is a valid escape character. Otherwise returns false.
static bool is_escape_char(char c) {
  return c == '\'' || c == '\"' || c == '?' || c == '\\' || c == 'a' ||
         c == 'b' || c == 'e' || c == 'f' || c == 'n' || c == 'r' || c == 't' ||
         c == 'v';
}

//! If `s` matches a hex escape sequence (characters after the "\x" prefix),
//! returns `s` after skipping the sequence. Exits with an error if `s` is an
//! invalid hex escape sequence. A hex escape sequence is invalid if it is
//! empty.
static const char* consume_hex_escape_sequence(const char* s) {
  size_t len = 0;
  while (is_hex_digit(*s)) {
    ++s;
    ++len;
  }
  if (len == 0) {
    // No hex digits is an error.
    // TODO: Make this error message nicer with file name, line:col number, etc.
    error("error: \\x used with no following hex digits.");
  }
  return s;
}

//! If `s` matches an escape sequence (characters after the slash '\'), returns
//! `s` after skipping the sequence. Returns `NULL` if `s` contains an hex
//! escape sequence longer than `max_hex_len`. It is assumed that `s` does not
//! start with the slash '\'. It is guaranteed that the return result is either
//! `NULL` or `s` advanced by at least 1 character.
//!
//! Some details on escape sequences:
//! - '\t', '\v', etc are legitimate escape sequences. We skip 1 character. We
//! return `s` + 1.
//!
//! - '\0', '\123', etc are legitimate octal escape sequences. We skip all
//! digits. Sequences like '\y', '\o', '\z', ... does not contain supported
//! escape characters, but we still treat them as if they are escape characters.
//! We skip 1 character and return `s` + 1.
//!
//! - Sequences like '\1234', '\0000', '\129',... have a prefix that is valid
//! octal escape sequence. We will skip that sequence, and the remaining digits
//! or characters will be treated as normal characters.
//!
//! - Sequences like '\x', '\xabc' are invalid hex escape sequences, we return
//! `NULL`.
static const char* consume_escape_sequence(const char* s) {
  if (is_escape_char(*s)) {
    ++s;
    return s;
  }
  if (*s == 'x') {
    ++s;
    return consume_hex_escape_sequence(s);
  }
  if (!is_oct_digit(*s)) {
    // TODO: this would be a good place to issue a warning about unknown escape
    // sequence. This is a case of "weird" escape character. So given something
    // like '\p', '\9', we should translate it into 'p' and '9'.
    return s + 1;
  }
  size_t len = 0;
  // Octal escape sequence has at most 3 characters.
  while (is_oct_digit(*s) && len <= 3) {
    ++s;
    ++len;
  }
  return s;
}

//! If `s` matches the "body" of a character or string literal, that is, the
//! content between the `quote`s, returns `s` after skipping the body
//! (note that it does NOT skip the terminating `quote`). Returns `NULL`
//! if `s` does not have a terminating `quote` character. It is assumed that `s`
//! does not start with a `quote` character and that `s` is non-empty.
static const char* consume_quoted_body(const char* s, char quote) {
  while (*s != '\0' && *s != '\n') {
    if (*s == quote) {
      // End of string/char literal.
      return s;
    }
    if (*s == '\\') {
      ++s;
      ASSIGN_OR_RETURN(s, consume_escape_sequence(s));
      continue;
    }
    ++s;
  }
  // Unterminated string/char literal.
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
    ASSIGN_OR_RETURN(s, consume_escape_sequence(s));
  } else {
    ++s;
  }
  // Wide char cannot have more than 1 character.
  return *s == '\'' ? s : NULL;
}

uint64_t lex_char_constant(const char* s, token* tok) {
  const char* start = s;
  if (is_char_prefix(s[0]) && s[1] == '\'') {
    // Prefix u: max 4 hex digits. Prefix U or L: max 8 hex digits.
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
  if (*s == '\'') {
    // Empty char.
    return 0;
  }
  // TODO: Octal escape sequence has allowed range too, depending on the prefix.
  // (u: \377, U/L: \777). We should just convert them into numerical values
  // first and then check the range.
  s = consume_quoted_body(s, /*quote=*/'\'');
  if (!s) {
    return 0;
  }
  // Skip the terminating single quote "'". It is guaranteed to exist due to the
  // check in `consume_quoted_body`.
  ++s;
  return s - start;
}

uint64_t lex_string_literal(const char* s) {
  const char* start = s;
  // String prefixes: u8, u, U, L.
  // u8, u: max 2 hex digits
  // U, L: max 8 hex digits
  if (s[0] == 'U' || s[0] == 'L') {
    ++s;
  } else if (strncmp(s, "u8", 2) == 0) {
    s += 2;
  } else if (s[0] == 'u') {
    ++s;
  }

  if (*s != '\"') {
    return 0;
  }
  ++s;
  s = consume_quoted_body(s, /*quote=*/'\"');
  if (!s) {
    return 0;
  }
  // Skip the terminating double quote. It is guaranteed to exist due to the
  // check in `consume_quoted_body`.
  ++s;
  return s - start;
}

array lex(char* s, size_t size) {
  array tokens;
  array_init(&tokens, sizeof(token));

  size_t line_num = 0;
  size_t col_num = 0;
  while (*s) {
    if (isspace(*s)) {
      ++col_num;
      continue;
    }
    if (*s == '\n') {
      ++line_num;
      col_num = 0;
      continue;
    }

    token* tok = array_push_back(&tokens);
    memset(tok, 0, sizeof(token));
    tok->token_type = TK_UNKNOWN;
    tok->line_num = line_num;
    tok->col_num = col_num;

    if (lex_identifier(s, tok)) {
      tok->token_type = is_keyword(s, tok->size) ? TK_KEYWRD : TK_IDENT;
      col_num += tok->size;
      continue;
    }

    if (lex_numeric_constant(s, tok)) {
      col_num += tok->size;
      continue;
    }

    // TODO: literals should go here.

    if (lex_punctuator(s, tok)) {
      col_num += tok->size;
      continue;
    }
  }

  return tokens;
}
