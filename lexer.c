#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

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
static inline bool is_word_char(char c) { return isalnum(c) || c == '_'; }

uint64_t lex_identifier(const char* s) {
  if (!isalpha(s[0]) && s[0] != '_') {
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

// Returns 1 if `c` matches [0-8], otherwise returns 0.
static int isoctdigit(int c) { return c >= '0' && c <= '8'; }

// Returns 1 if `c` matches [a-fA-F0-9], otherwise returns 0.
static int ishexdigit(int c) {
  return isdigit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

// u or U; l or L; ll or LL; Both u or U and l or L; Both u or U and ll or LL.
// 6 + (2 * 2 + 2 * 2) * 2 = 22
#define INT_SUFFIX_SIZE 22
// Longest first so that we match them first.
static char* INT_SUFFIX[INT_SUFFIX_SIZE] = {
    "uLL", "ull", "ULL", "Ull", "LLu", "llu", "LLU", "llU", "ul", "uL", "Ul",
    "UL",  "lu",  "Lu",  "lU",  "LU",  "ll",  "LL",  "u",   "U",  "l",  "L",
};

// If `s` matches any integer suffixes, returns `s` after skipping the suffix.
// Otherwise returns `s` as-is.
static const char* consume_int_suffix(const char* s) {
  char c = tolower(*s);
  if (c != 'u' && c != 'l') {
    // Fast path.
    return s;
  }
  for (size_t i = 0; i < INT_SUFFIX_SIZE; ++i) {
    char* suffix = INT_SUFFIX[i];
    size_t len = strlen(suffix);
    if (strncmp(suffix, s, len) == 0) {
      s += len;
      return s;
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
  if (is_word_char(*s)) {
    return 0;
  }
  return s - start;
}

uint64_t lex_constant(const char* s) {
  if (strncmp(s, "0x", 2) == 0 || strncmp(s, "0X", 2) == 0) {
    s += 2;
    uint64_t res = lex_integer(s, ishexdigit);
    return res ? res + 2 : 0;
  }
  if (*s == '0') {
    return lex_integer(s, isoctdigit);
  }
  return lex_integer(s, isdigit);
}
