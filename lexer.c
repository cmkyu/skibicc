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

// Returns true if `c` matches [a-zA-Z_]. Otherwise returns false.
static bool iswordchar(char c) { return isalnum(c) || c == '_'; }

uint64_t lex_identifier(const char* s) {
  if (!isalpha(s[0]) && s[0] != '_') {
    return 0;
  }

  const char* start = s;
  ++s;
  while (true) {
    char c = *s;
    if (!iswordchar(c)) {
      break;
    }
    ++s;
  }
  return s - start;
}

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

static const size_t KEYWORDS_SIZE = sizeof(KEYWORDS) / sizeof(KEYWORDS[0]);

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

// Returns 1 if `c` matches [0-8], otherwise returns 0.
static int isoctdigit(int c) { return c >= '0' && c <= '8'; }

// Returns 1 if `c` matches [a-fA-F0-9], otherwise returns 0.
static int ishexdigit(int c) {
  return isdigit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

// Returns true if `s1` starts with `s2`, otherwise returns false. `n` is the
// length of `s2`.
static bool startswith(const char* s1, const char* s2, size_t n) {
  return strncmp(s1, s2, n) == 0;
}

// u or U; l or L; ll or LL; Both u or U and l or L; Both u or U and ll or LL.
// Longest first so that we match them first.
static char* INT_SUFFIXES[] = {
    "uLL", "ull", "ULL", "Ull", "LLu", "llu", "LLU", "llU", "ul", "uL", "Ul",
    "UL",  "lu",  "Lu",  "lU",  "LU",  "ll",  "LL",  "u",   "U",  "l",  "L",
};

static const size_t INT_SUFFIXES_SIZE =
    sizeof(INT_SUFFIXES) / sizeof(INT_SUFFIXES[0]);

// If `s` matches any integer suffixes, returns `s` after skipping the suffix.
// Otherwise returns `s` as-is.
static const char* consume_int_suffix(const char* s) {
  char c = tolower(*s);
  if (c != 'u' && c != 'l') {
    // Fast path.
    return s;
  }
  for (size_t i = 0; i < INT_SUFFIXES_SIZE; ++i) {
    char* suffix = INT_SUFFIXES[i];
    size_t len = strlen(suffix);
    if (startswith(s, suffix, len)) {
      return s + len;
    }
  }
  return s;
}

// Returns the length of the integer starting at the character pointed to by
// `s`. Returns 0 if it is not a an integer.
// `fdigit` is the predicate used to test if characters in `s` belong in the
// integer.
// `s` should point to the integer after its prefix has been stripped. ('0' for
// octal and '0x'/'0X' for hexadecimal.
static uint64_t lex_integer(const char* s, int (*fdigit)(int)) {
  const char* start = s;
  while (fdigit(*s)) {
    ++s;
  }
  if (s == start) {
    return 0;
  }
  s = consume_int_suffix(s);
  // Integer must end at word boundary.
  if (iswordchar(*s)) {
    return 0;
  }
  return s - start;
}

uint64_t lex_constant(const char* s) {
  if (s[0] == '0') {
    if (tolower(s[1]) == 'x') {
      s += 2;
      uint64_t res = lex_integer(s, ishexdigit);
      return res ? res + 2 : 0;
    }
    return lex_integer(s, isoctdigit);
  }
  return lex_integer(s, isdigit);
}

// Order matters here. For fast lookup we want the most common punctuators to
// be at the front, but if it is a prefix of another punctuator, it should be
// placed behind that punctuator.
static char* PUNCTUATORS[] = {
    ";",  "{",  "}",  "[",   "]",  "(",  ")", ",",   "==", "=",  "...", ".",
    "++", "+=", "+",  "--",  "->", "-=", "-", "*=",  "*",  "/=", "/",   "%=",
    "%",  "?",  ":",  "<<=", "<<", "<=", "<", ">>=", ">>", ">=", ">",   "!=",
    "!",  "&&", "&=", "&",   "||", "|=", "|", "^=",  "^",  "~",  "##",  "#",
};

const size_t PUNCTUATORS_SIZE = sizeof(PUNCTUATORS) / sizeof(PUNCTUATORS[0]);

uint64_t lex_punctuator(const char* s) {
  if (!ispunct(*s)) {
    return 0;
  }
  for (size_t i = 0; i < PUNCTUATORS_SIZE; ++i) {
    const char* punct = PUNCTUATORS[i];
    size_t len = strlen(punct);
    if (startswith(s, punct, len)) {
      return len;
    }
  }
  return 0;
}
