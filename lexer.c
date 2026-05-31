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
#include "unicode.h"

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
  if (is_keyword(tok->loc, tok->size)) {
    tok->token_type = TK_KEYWRD;
  }
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
    return s;
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
  ++s;  // Skip the exponent character.
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
  bool has_invalid_oct = false;
  while (isdigit(*s)) {
    has_invalid_oct |= !is_oct_digit(*s);
    ++s;
  }
  if (*s != '.' && !is_dec_exp_char(*s)) {
    // Decimal or octal integer
    bool is_oct = *start == '0';
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

//! Enum whose integer value denotes character width. Each width corresponds to
//! an unicode encoding.
//! - 1 byte width: UTF-8
//! - 2 byte width: UTF-16
//! - 4 byte width: UTF-32
typedef enum char_width {
  //! 1 byte wide character. UTF-8.
  CW_UTF8 = 1,
  //! 2 byte wide character. UTF-16.
  CW_UTF16 = 2,
  //! 4 byte wide character. UTF-32.
  CW_UTF32 = 4,
} char_width;

//! If `s` matches a hex escape sequence (characters after the "\x" prefix),
//! returns `s` after skipping the sequence. `dst` will contain the translated
//! integer value of the hex escape sequence after the function returns.
//! `alerts` will point contain an error message if `s` is an invalid hex escape
//! sequence. A hex escape sequence is invalid if it is empty or has more digits
//! than permitted by `char_width`. `char_width` is the width of characters
//! in`s` in bytes. Note that an invalid sequence is still consumed.
static const char* consume_hex_escape_sequence(const char* s, uint32_t* dst,
                                               char_width char_width,
                                               token* tok,
                                               alert_queue* alerts) {
  uint32_t c = 0;
  size_t len = 0;
  size_t max_len = char_width * 2;
  while (isxdigit(*s)) {
    c <<= 4;
    char digit = tolower(*s);
    if (isdigit(digit)) {
      c |= digit - '0';
    } else {
      c |= (digit - 'a' + 10);
    }
    ++s;
    ++len;
  }
  if (len == 0) {
    // No hex digits is an error.
    alert_queue_push_error(alerts, tok,
                           "\\x used with no following hex digits.");
  } else if (len > max_len) {
    alert_queue_push_error(alerts, tok, "hex escape sequence out of range.");
  }
  *dst = c;
  return s;
}

//! If `s` matches a octal escape sequence (characters after the "\x" prefix),
//! returns `s` after skipping the sequence. `dst` will contain the translated
//! integer value of the octal escape sequence after the function returns.
//! `alerts` will contain an error message if `s` is an invalid octal escape
//! sequence.  A octal escape sequence is invalid if it has more digits than
//! permitted by `char_width`. `char_width` is the width of characters in`s` in
//! bytes. Note that an invalid sequence is still consumed
static const char* consume_oct_escape_sequence(const char* s, uint32_t* dst,
                                               char_width char_width,
                                               token* tok,
                                               alert_queue* alerts) {
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
  if (char_width == CW_UTF8 && c > 255u) {
    alert_queue_push_error(alerts, tok, "octal escape sequence out of range.");
  }
  *dst = c;
  return s;
}

//! Given a character `c` which is the character after the slash ('\') in an
//! character escape sequence, returns its corresponding escape character. For
//! example, given 't', return '\t'. If the escape character is not supported,
//! `alerts` will contain a warning.
static char get_escape_char(char c, token* tok, alert_queue* alerts) {
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
      alert_queue_push_warning(alerts, tok, "unknown escape sequence '\\%c'.",
                               c);
      return c;
  }
}

//! If `s` matches an escape sequence (characters after the slash '\') whose
//! characters have width `char_width`, returns `s` after skipping the sequence.
//! `dst` will contain the translated escape sequence in integer form after the
//! function returns. `alerts` will contain an error message if the escape
//! sequence is invalid. It is assumed that `s` does not start with the slash
//! '\'. It is guaranteed that the return result is never `NULL`.
//!
//! Some details on escape sequences:
//! - As an example, '\t', '\v', etc are legitimate escape sequences. We skip 1
//! character. We return `s` + 1. `dst` will contain the ASCII code of '\t',
//! '\v', etc.
//!
//! - Sequences like '\y', '\o', '\z', ... does not contain supported escape
//! characters, but we still treat them as if they are escape characters. We
//! skip 1 character and return `s` + 1. `dst` will simply contain the letter
//! after the slash. E.g., for '\y', `dst` will contain 'y'.
//!
//! - Invalid sequences are octal or hex escape sequences whose value is out of
//! the range permitted by `char_width`. E.g., '\777' is invalid if char_width
//! is 1 byte.
static const char* consume_escape_sequence(const char* s, uint32_t* dst,
                                           char_width char_width, token* tok,
                                           alert_queue* alerts) {
  if (*s == 'x') {
    ++s;
    return consume_hex_escape_sequence(s, dst, char_width, tok, alerts);
  }
  if (is_oct_digit(*s)) {
    return consume_oct_escape_sequence(s, dst, char_width, tok, alerts);
  }
  // No additional encoding is required because all escape characters are ASCII.
  // UTF-8, UTF-16, and UTF-32 are ASCII compatible.
  *dst = get_escape_char(*s, tok, alerts);
  return s + 1;
}

//! Returns true if `c` is a character literal prefix. Otherwise returns false.
static bool is_char_prefix(char c) { return c == 'L' || c == 'u' || c == 'U'; }

//! If `s` matches a wide character literal, that is, returns `s` after skipping
//! the literal. Returns `NULL` if `s` is not a wide character literal. `dst`
//! will contain the integer value of the wide character after the function
//! returns. `alerts` will contain an error message if `s` does not contain
//! exactly 1 character or is unterminated.
static const char* consume_wide_char_body(const char* s, uint32_t* dst,
                                          token* tok, alert_queue* alerts) {
  if (!(is_char_prefix(s[0]) && s[1] == '\'')) {
    return NULL;
  }

  // Prefix u: 2 byte character. Prefix U or L: 4 byte character.
  char prefix = s[0];
  char_width char_width = CW_UTF16;
  if (prefix == 'U' || prefix == 'L') {
    char_width = CW_UTF32;
  }
  s += 2;

  if (*s == '\'') {
    ++s;
    alert_queue_push_error(alerts, tok, "character literal cannot be empty.");
    return s;
  }

  if (*s == '\\') {
    ++s;
    ASSIGN_OR_RETURN(s,
                     consume_escape_sequence(s, dst, char_width, tok, alerts));
  } else {
    s = decode_utf8(s, dst);
  }
  if (*s == '\'') {
    ++s;
    return s;
  }

  // Wide char cannot have more than 1 character.
  alert_queue_push_error(
      alerts, tok,
      prefix == 'L'
          ? "wide character literal may not contain multiple characters."
          : "unicode character literal may not contain multiple characters.");
  while (*s != '\0' && *s != '\n') {
    if (*s == '\'') {
      ++s;
      return s;
    }
    ++s;
  }
  alert_queue_push_error(alerts, tok, "unterminated character literal.");
  return s;
}

//! If `s` matches a character literal, returns `s` after skipping the literal.
//! Returns `NULL` if `s` is not a character literal. `dst` will contain the
//! integer value of the character literal after the function returns.
//! Multi-character literals are supported, but only the last 4 bytes will be
//! kept.
//!
//! The following `alerts` are possible:
//! The character literal has invalid escape sequence, is empty or is not
//! properly terminated.
//! The literal contains multiple characters or escape sequences that are not
//! supported.
static const char* consume_char_body(const char* s, uint32_t* dst, token* tok,
                                     alert_queue* alerts) {
  if (*s != '\'') {
    return NULL;
  }
  ++s;
  if (*s == '\'') {
    ++s;
    alert_queue_push_error(alerts, tok, "character literal cannot be empty.");
    return s;
  }

  size_t len = 0;
  uint32_t res = 0;
  while (*s != '\0' && *s != '\n') {
    if (*s == '\'') {
      if (len > 1) {
        alert_queue_push_warning(alerts, tok, "multi-byte character literal.");
      }
      // End of char literal.
      ++s;
      *dst = res;
      return s;
    }

    uint8_t c = 0;
    if (*s == '\\') {
      // Escape sequence.
      ++s;
      uint32_t escape = 0;
      ASSIGN_OR_RETURN(
          s, consume_escape_sequence(s, &escape, CW_UTF8, tok, alerts));
      c = (uint8_t)escape;
    } else {
      // Regular character.
      c = *s;
      ++s;
    }
    res = (res << 8) | c;
    ++len;
  }
  alert_queue_push_error(alerts, tok, "unterminated character literal.");
  return s;
}

bool lex_char_literal(const char* s, token* tok, alert_queue* alerts) {
  uint32_t res = 0;
  const char* start = s;
  s = consume_char_body(start, &res, tok, alerts);
  if (!s) {
    s = consume_wide_char_body(start, &res, tok, alerts);
  }
  if (!s) {
    return false;
  }
  tok->token_type = TK_ICONST;
  tok->constant.int_val = (uint64_t)res;
  tok->loc = start;
  tok->size = s - start;
  return true;
}

//! If `s` matches a string literal, returns `s` after skipping the string
//! literal. Returns `NULL` if `s` is not a string literal. `arr` will contain
//! the content (without the double quotes) of the string literal encoded in
//! UTF-8 after the function returns. `alerts` will contain an error message
//! if the string literal contains invalid escape sequence.
static const char* consume_utf8_str_body(const char* s, array* arr, token* tok,
                                         alert_queue* alerts) {
  if (*s != '\"') {
    return NULL;
  }
  ++s;

  array_init(arr, /*item_size=*/CW_UTF8);
  while (*s != '\0' && *s != '\n') {
    if (*s == '\"') {
      // End of string literal.
      void* dst = array_push_back(arr);
      *(uint8_t*)dst = '\0';
      ++s;
      return s;
    }

    uint8_t c = 0;
    if (*s == '\\') {
      // Escape sequence.
      ++s;
      uint32_t escape = 0;
      ASSIGN_OR_RETURN(
          s, consume_escape_sequence(s, &escape, CW_UTF8, tok, alerts));
      c = (uint8_t)escape;
    } else {
      // Regular character. No transcoding needed since we assume the source
      // code to be in UTF-8.
      c = *s;
      ++s;
    }

    void* dst = array_push_back(arr);
    *(uint8_t*)dst = c;
  }
  alert_queue_push_error(alerts, tok, "unterminated string literal.");
  return s;
}

//! If `s` matches a string literal, returns `s` after skipping the string
//! literal. Returns `NULL` if `s` is not a string literal. `arr` will contain
//! the content (without the double quotes) of the string literal encoded in
//! UTF-16 after the function returns. `alerts` will contain an error message
//! if the string literal contains invalid escape sequence.
static const char* consume_utf16_str_body(const char* s, array* arr, token* tok,
                                          alert_queue* alerts) {
  if (*s != '\"') {
    return NULL;
  }
  ++s;

  array_init(arr, /*item_size=*/CW_UTF16);
  while (*s != '\0' && *s != '\n') {
    if (*s == '\"') {
      // End of string literal.
      void* dst = array_push_back(arr);
      *(uint16_t*)dst = '\0';
      ++s;
      return s;
    }

    if (*s == '\\') {
      // Escape sequence.
      ++s;
      uint32_t c = 0;
      ASSIGN_OR_RETURN(s,
                       consume_escape_sequence(s, &c, CW_UTF16, tok, alerts));
      void* dst = array_push_back(arr);
      *(uint16_t*)dst = (uint16_t)c;
    } else {
      // Regular character.
      uint32_t c = 0;
      s = decode_utf8(s, &c);
      encode_utf16(c, arr);
    }
  }
  alert_queue_push_error(alerts, tok, "unterminated string literal");
  return s;
}

//! If `s` matches a string literal, returns `s` after skipping the string
//! literal. Returns `NULL` if `s` is not a string literal. `arr` will contain
//! the content (without the double quotes) of the string literal encoded in
//! UTF-32 after the function returns. `error_msg` will contain an error message
//! if the string literal contains invalid escape sequence.
static const char* consume_utf32_str_body(const char* s, array* arr, token* tok,
                                          alert_queue* alerts) {
  if (*s != '\"') {
    return NULL;
  }
  ++s;

  array_init(arr, /*item_size=*/CW_UTF32);
  while (*s != '\0' && *s != '\n') {
    if (*s == '\"') {
      // End of string literal.
      void* dst = array_push_back(arr);
      *(uint32_t*)dst = '\0';
      ++s;
      return s;
    }

    if (*s == '\\') {
      // Escape sequence.
      ++s;
      uint32_t c = 0;
      ASSIGN_OR_RETURN(s,
                       consume_escape_sequence(s, &c, CW_UTF32, tok, alerts));
      void* dst = array_push_back(arr);
      *(uint32_t*)dst = c;
    } else {
      // Regular character.
      uint32_t c = 0;
      s = decode_utf8(s, &c);
      encode_utf32(c, arr);
    }
  }
  alert_queue_push_error(alerts, tok, "unterminated string literal");
  return s;
}

bool lex_string_literal(const char* s, token* tok, alert_queue* alerts) {
  const char* start = s;
  array arr;
  // Handle prefixes.
  if (strncmp(s, "u8", 2) == 0) {
    // u8: 1 byte character, UTF-8
    s += 2;
    s = consume_utf8_str_body(s, &arr, tok, alerts);
  } else if (s[0] == 'u') {
    // u: 2 byte character, UTF-16
    ++s;
    s = consume_utf16_str_body(s, &arr, tok, alerts);
  } else if (s[0] == 'U' || s[0] == 'L') {
    // U, L: 4 byte character, UTF-32
    ++s;
    s = consume_utf32_str_body(s, &arr, tok, alerts);
  } else {
    // No prefix: 1 byte character, UTF-8
    s = consume_utf8_str_body(s, &arr, tok, alerts);
  }

  if (!s) {
    return false;
  }
  tok->token_type = TK_STRLIT;
  tok->constant.str_val = arr.buf;
  tok->loc = start;
  tok->size = s - start;
  return true;
}

//! Internal states of the lexer.
typedef struct lexer {
  //! Current line number.
  size_t line_num;
  //! Current column number.
  size_t col_num;
  //! Filename of the source code file.
  const char* filename;
  //! Pointer to the current character.
  const char* cur;
  //! Pointer to the current line.
  const char* line;
  //! Tokens produced by the lexer.
  array tokens;
  //! Queue of compilation warnings and errors.
  alert_queue* alert_queue;
} lexer;

//! Creates the lexer given a `source` code buffer read from `filename`.
static lexer* lexer_init(const char* source, const char* filename) {
  lexer* l = malloc_safe(sizeof(lexer));
  array_init(&l->tokens, sizeof(token));

  l->line_num = 1;
  l->col_num = 1;
  l->filename = filename;
  l->cur = source;
  l->line = source;
  l->alert_queue = alert_queue_init();
  return l;
}

//! Adds a token to the back of the lexer token array and returns the added
//! token. The caller is responsible for populating the token's type and value.
static token* lexer_add_token(lexer* l) {
  token* tok = array_push_back(&l->tokens);
  tok->token_type = TK_UNKNOWN;
  tok->filename = l->filename;
  tok->line = l->line;
  tok->line_num = l->line_num;
  tok->col_num = l->col_num;
  return tok;
}

//! Skips the current character if it is a whitespace. Returns true if the
//! character is skipped. Otherwise returns false.
static bool lexer_skip_whitespace(lexer* l) {
  if (*l->cur == '\n') {
    ++l->line_num;
    l->col_num = 1;
    ++l->cur;
    l->line = l->cur;
    return true;
  }
  if (isspace(*l->cur)) {
    ++l->col_num;
    ++l->cur;
    return true;
  }
  return false;
}

//! Adances the current character to just behind `tok`.
static void lexer_advance(lexer* l, token* tok) {
  l->cur = tok->loc + tok->size;
  l->col_num += tok->size;
}

//! Frees the lexer, but keeps the token array intact. This is intentional so
//! that the tokens can be processed by the next stage.
static void lexer_destroy(lexer* l) {
  alert_queue_destroy(l->alert_queue);
  free(l);
}

array lex(const char* s, const char* filename) {
  lexer* l = lexer_init(s, filename);
  while (*l->cur) {
    if (lexer_skip_whitespace(l)) {
      continue;
    }

    token* tok = lexer_add_token(l);
    // Order is important here. String literals and constants must come before
    // identifiers and punctuators. This is because string literals and
    // constants may contain prefixes that qualify as identifiers or
    // punctuators.
    bool res = false;
    res = lex_numeric_constant(l->cur, tok);
    if (!res) {
      res = lex_char_literal(l->cur, tok, l->alert_queue);
    }
    if (!res) {
      res = lex_string_literal(l->cur, tok, l->alert_queue);
    }
    if (!res) {
      res = lex_identifier(l->cur, tok);
    }
    if (!res) {
      res = lex_punctuator(l->cur, tok);
    }
    if (!res) {
      // TODO: Maybe let the parser worry about it.
      tok->size = 1;
      alert_queue_push_error(l->alert_queue, tok, "unexpected token.");
    }
    alert_queue_report(l->alert_queue);
    lexer_advance(l, tok);
  }

  // Inserts an EOF token at the end.
  token* tok = lexer_add_token(l);
  tok->token_type = TK_EOF;
  tok->loc = "EOF";

  // This is a cheap shallow copy.
  array tokens = l->tokens;
  // Lexer destructor does not destroy the token array, so this is fine.
  lexer_destroy(l);
  return tokens;
}

void destroy_tokens(array* tokens) {
  for (size_t i = 0; i < tokens->size; ++i) {
    token* tok = array_at(tokens, i);
    if (tok->token_type == TK_STRLIT) {
      free(tok->constant.str_val);
    }
  }
  array_destroy(tokens);
}
