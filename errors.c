//!@file
//!@brief Source file for various error printing functions.

#include "errors.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lexer.h"

#define COLOR_BOLD "\x1b[1m"
#define COLOR_BOLD_RED "\x1b[1;31m"
#define COLOR_RESET "\x1b[0m"

void error(char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  vfprintf(stderr, fmt, args);
  fprintf(stderr, "\n");
  exit(1);
}

//! Prints a error message header formatted like the following to stderr:
//! <filename>:line_num:col_num: error:
static void error_tok_header(token* tok) {
  fprintf(stderr,
          COLOR_BOLD "<%s>:%zu:%zu:" COLOR_RESET " " COLOR_BOLD_RED
                     "error:" COLOR_RESET,
          tok->filename, tok->line_num, tok->col_num);
}

//! Returns the number of digits of `num` in base 10.
static size_t get_num_digits(size_t num) {
  size_t res = 1;
  while (num >= 10) {
    num /= 10;
    ++res;
  }
  return res;
}

//! Prints a error message footer formatted like the following to stderr:
//! <line num> |  int foo = @@@;
//!            |            ^~~
static void error_tok_footer(token* tok) {
  // Print the line number with the offending line.
  fprintf(stderr, "\t%zu |\t", tok->line_num);
  bool reset = false;
  for (const char* s = tok->line; *s != '\0' && *s != '\n'; ++s) {
    if (s == tok->loc) {
      fprintf(stderr, COLOR_BOLD_RED);
    }
    if (s >= tok->loc + tok->size && !reset) {
      fprintf(stderr, COLOR_RESET);
      reset = true;
    }
    fputc(*s, stderr);
  }
  if (!reset) {
    fprintf(stderr, COLOR_RESET);
  }
  fputc('\n', stderr);

  // Print the '|' separator with padding.
  size_t num_digits = get_num_digits(tok->line_num);
  fputc('\t', stderr);
  for (size_t i = 0; i < num_digits; ++i) {
    fputc(' ', stderr);
  }
  fprintf(stderr, " |\t");
  for (size_t i = 1; i < tok->col_num; ++i) {
    fputc(' ', stderr);
  }

  // Print the squiggly line.
  fprintf(stderr, COLOR_BOLD_RED);
  fputc('^', stderr);
  for (size_t i = 1; i < tok->size; ++i) {
    fputc('~', stderr);
  }
  fprintf(stderr, COLOR_RESET);
  fputc('\n', stderr);
}

void error_tok_fmt(token* tok, char* fmt, ...) {
  error_tok_header(tok);
  fprintf(stderr, " ");
  va_list args;
  va_start(args, fmt);
  vfprintf(stderr, fmt, args);
  fprintf(stderr, "\n");
  error_tok_footer(tok);
  exit(1);
}

void* malloc_safe(size_t size) {
  void* ptr = malloc(size);
  if (!ptr) {
    error("FATAL: malloc() failed.");
  }
  return ptr;
}

void* calloc_safe(size_t nelem, size_t elsize) {
  void* ptr = calloc(nelem, elsize);
  if (!ptr) {
    error("FATAL: calloc() failed.");
  }
  return ptr;
}

void* realloc_safe(void* ptr, size_t size) {
  void* p = realloc(ptr, size);
  if (!p) {
    error("FATAL: realloc() failed.");
  }
  return p;
}
