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
  if (!isxdigit(*s) && *s != '.') {
    return NULL;
  }
  if (s[0] == '.' && !isxdigit(s[1])) {
    return NULL;
  }

  while (isxdigit(*s)) {
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
    while (isxdigit(*s)) {
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

typedef enum char_width {
  CW_1 = 1,
  CW_2 = 2,
  CW_4 = 4,
} char_width;

//! If `s` matches a hex escape sequence (characters after the "\x" prefix),
//! returns `s` after skipping the sequence. Exits with an error if `s` is an
//! invalid hex escape sequence. A hex escape sequence is invalid if it is
//! empty. `char_width` is the width of characters in`s` in bytes.
static const char* consume_hex_escape_sequence(const char* s, uint32_t* dst,
                                               char_width char_width) {
  uint32_t c = 0;
  size_t len = 0;
  size_t max_len = char_width * 2;
  while (isxdigit(*s)) {
    c <<= 4;
    char digit = tolower(*s);
    if (isdigit(digit)) {
      c |= digit - '0';
    } else {
      c |= digit - 'a';
    }
    ++s;
    ++len;
    if (len > max_len) {
      error("error: hex escape sequence out of range.");
    }
  }
  if (len == 0) {
    // No hex digits is an error.
    // TODO: Make this error message nicer with file name, line:col number, etc.
    error("error: \\x used with no following hex digits.");
  }
  *dst = c;
  return s;
}

static const char* consume_oct_escape_sequence(const char* s, uint32_t* dst,
                                               char_width char_width) {
  uint32_t c = 0;
  size_t len = 0;
  // Octal escape sequence has at most 3 characters.
  while (is_oct_digit(*s) && len <= 3) {
    c <<= 3;
    c |= *s - '0';
    ++s;
    ++len;
  }
  // 1 byte holds at most an integer value of 255.
  if (char_width == CW_1 && c > '\377') {
    error("error: octal escape sequence out of range.");
  }
  *dst = c;
  return s;
}

static char get_escape_char(char c) {
  switch (c) {
    case '\'':
      return '\'';
    case '\"':
      return '\"';
    case '?':
      return '\?';
    case '\\':
      return '\\';
    case 'a':
      return '\a';
    case 'b':
      return '\b';
    case 'f':
      return '\f';
    case 'n':
      return '\n';
    case 'r':
      return '\r';
    case 't':
      return '\t';
    case 'v':
      return '\v';
    case 'e':
      // Non-standard '\e' character. Supported by GCC and clang.
      return '\033';
    default:
      // TODO: make this warning message nicer.
      fprintf(stderr, "warning: unknown escape sequence \\%c\n", c);
      return c;
  }
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
//! - Sequences like '\x' are invalid hex escape sequences, we return `NULL`.
static const char* consume_escape_sequence(const char* s, uint32_t* dst,
                                           char_width char_width) {
  if (*s == 'x') {
    ++s;
    return consume_hex_escape_sequence(s, dst, char_width);
  }
  if (is_oct_digit(*s)) {
    return consume_oct_escape_sequence(s, dst, char_width);
  }
  *dst = get_escape_char(*s);
  return s + 1;
}

//! Returns true if `c` is a character literal prefix. Otherwise returns false.
static bool is_char_prefix(char c) { return c == 'L' || c == 'u' || c == 'U'; }

//! If `s` matches the "body" of a wide character literal, that is, the content
//! between the single quotes ("'"), returns `s` after skipping the body (note
//! that it does NOT skip the terminating single quote). Returns `NULL` if `s`
//! contains no character body, or if `s` does not have a terminating single
//! quote character, or if `s` contains more than 1 character. It is assumed
//! that `s` does not start with a single quote.
static const char* consume_wide_char_body(const char* s, uint32_t* dst) {
  if (!(is_char_prefix(s[0]) && s[1] == '\'')) {
    return NULL;
  }

  // Prefix u: 2 byte character. Prefix U or L: 4 byte character.
  char prefix = s[0];
  char_width char_width = CW_2;
  if (prefix == 'U' || prefix == 'L') {
    char_width = CW_4;
  }
  s += 2;

  if (*s == '\'' || *s == '\0' || *s == '\n') {
    // Empty char body.
    return NULL;
  }

  if (*s == '\\') {
    ++s;
    ASSIGN_OR_RETURN(s, consume_escape_sequence(s, dst, char_width));
  } else {
    // TODO: need a UTF-8 decode method.
    *dst = (uint32_t)*s;
    ++s;
  }
  // Wide char cannot have more than 1 character.
  if (*s != '\'') {
    error("error: %s character literal may not contain multiple characters.",
          prefix == 'L' ? "wide" : "unicode");
  }
  return s;
}

static const char* consume_char_body(const char* s, uint32_t* dst) {
  if (*s != '\'') {
    return NULL;
  }
  ++s;
  if (*s == '\'') {
    // Empty char.
    return NULL;
  }

  size_t len = 0;
  while (*s != '\0' && *s != '\n') {
    if (*s == '\'') {
      if (len > 0) {
        // TODO: make this warning pretty.
        fprintf(stderr, "warning: multi-character character constant\n");
      }
      // End of char literal.
      return s;
    }
    if (*s == '\\') {
      ++s;
      ASSIGN_OR_RETURN(s, consume_escape_sequence(s, dst, CW_1));
      continue;
    }
    // TODO: need a UTF-8 decode method.
    *dst = (uint32_t)*s;
    ++s;
    ++len;
  }
  // Unterminated string/char literal.
  return NULL;
}

bool lex_char_constant(const char* s, token* tok) {
  uint32_t res = 0;
  const char* start = s;
  s = consume_char_body(s, &res);
  if (s) {
    s = consume_wide_char_body(s, &res);
  }
  if (!s) {
    return false;
  }
  // Skip the terminating single quote "'". It is guaranteed to exist after the
  // null check.
  ++s;
  tok->token_type = TK_ICONST;
  tok->constant.int_val = (uint64_t)res;
  tok->loc = start;
  tok->size = s - start;
  return true;
}

static void copy_char(uint32_t c, void* dst, char_width char_width) {
  switch (char_width) {
    case CW_1:
      *(uint8_t*)dst = (uint8_t)c;
      break;
    case CW_2:
      *(uint16_t*)dst = (uint16_t)c;
      break;
    case CW_4:
      *(uint32_t*)dst = c;
      break;
    default:
      error("FATAL: Unexpected char_width enum: %d\n", char_width);
  }
}

//! If `s` matches the "body" of a string literal, that is, the content between
//! the double quotes ('"'), returns `s` after skipping the body (note that it
//! does NOT skip the terminating `quote`). Returns `NULL` if `s` is not a
//! string literal.
static const char* consume_string_body(const char* s, array* arr,
                                       char_width char_width) {
  if (*s != '\"') {
    return NULL;
  }
  ++s;

  while (*s != '\0' && *s != '\n') {
    if (*s == '\"') {
      // End of string literal.
      void* dst = array_push_back(arr);
      copy_char('\0', dst, char_width);
      return s;
    }

    uint32_t c = 0;
    void* dst = array_push_back(arr);
    if (*s == '\\') {
      ++s;
      ASSIGN_OR_RETURN(s, consume_escape_sequence(s, &c, char_width));
    } else {
      // TODO: need a UTF decode method.
      c = (uint32_t)*s;
      ++s;
    }
    copy_char(c, dst, char_width);
  }
  // Unterminated string literal.
  return NULL;
}

bool lex_string_literal(const char* s, token* tok) {
  const char* start = s;
  char_width char_width = CW_1;
  // Handle prefixes.
  if (strncmp(s, "u8", 2) == 0) {
    // u8: 1 byte character, UTF-8
    char_width = CW_1;
    s += 2;
  } else if (s[0] == 'u') {
    // u: 2 byte character, UTF-16
    char_width = CW_2;
    ++s;
  } else if (s[0] == 'U' || s[0] == 'L') {
    // U, L: 4 byte character, UTF-32
    char_width = CW_4;
    ++s;
  }
  array arr;
  array_init(&arr, /*item_size=*/char_width);

  s = consume_string_body(s, &arr, char_width);
  if (!s) {
    return 0;
  }
  // Skip the terminating double quote.
  ++s;
  tok->token_type = TK_STRLIT;
  tok->constant.str_val = arr.buf;
  tok->loc = start;
  tok->size = s - start;
  return true;
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
