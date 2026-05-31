//!@file
//!@brief Source file for various error printing functions.

#include "errors.h"

#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "array.h"
#include "lexer.h"

#define COLOR_BOLD "\x1b[1m"
#define COLOR_BOLD_RED "\x1b[1;31m"
#define COLOR_BOLD_YELLOW "\x1b[1;33m"
#define COLOR_RESET "\x1b[0m"

//! Types of alerts.
typedef enum alert_type {
  //! Compilation warning.
  ALERT_WARN,
  //! Compilation error.
  ALERT_ERR,
} alert_type;

//! Represents an error or a warning message.
typedef struct alert {
  //! Type of the alert.
  alert_type alert_type;
  //! The offending token.
  token* tok;
  //! The alert messaage.
  char* alert_msg;
} alert;

alert_queue* alert_queue_init(void) {
  alert_queue* q = malloc_safe(sizeof(alert_queue));
  q->queue = malloc_safe(sizeof(array));
  array_init(q->queue, sizeof(alert));
  return q;
}

static char* fmt_to_msg(char* fmt, va_list args) {
  char* msg;
  size_t size;
  FILE* f = open_memstream(&msg, &size);
  if (!f) {
    error("FATAL: open_memstream() failed.");
  }
  vfprintf(f, fmt, args);
  fclose(f);
  return msg;
}

void alert_queue_push_warning(alert_queue* q, token* tok, char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  char* msg = fmt_to_msg(fmt, args);
  va_end(args);

  alert* alert = array_push_back(q->queue);
  alert->alert_type = ALERT_WARN;
  alert->tok = tok;
  alert->alert_msg = msg;
}

void alert_queue_push_error(alert_queue* q, token* tok, char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  char* msg = fmt_to_msg(fmt, args);
  va_end(args);

  alert* alert = array_push_back(q->queue);
  alert->alert_type = ALERT_ERR;
  alert->tok = tok;
  alert->alert_msg = msg;
}

//! Frees all alert messages inside the queue.
static void alert_queue_free_alerts(alert_queue* q) {
  for (size_t i = 0; i < q->queue->size; ++i) {
    alert* a = array_at(q->queue, i);
    free(a->alert_msg);
  }
}

void alert_queue_report(alert_queue* q) {
  for (size_t i = 0; i < q->queue->size; ++i) {
    alert* a = array_at(q->queue, i);
    switch (a->alert_type) {
      case ALERT_WARN:
        warn_tok_fmt(a->tok, "%s", (char*)a->alert_msg);
        break;
      case ALERT_ERR:
        error_tok_fmt(a->tok, "%s", (char*)a->alert_msg);
        break;
    }
  }
}

void alert_queue_clear(alert_queue* q) {
  alert_queue_free_alerts(q);
  array_clear(q->queue);
}

void alert_queue_destroy(alert_queue* q) {
  alert_queue_free_alerts(q);
  array_destroy(q->queue);
  free(q->queue);
  free(q);
}

void error(char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  vfprintf(stderr, fmt, args);
  va_end(args);
  fprintf(stderr, "\n");
  exit(1);
}

//! Prints a error message header formatted like the following to stderr:
//! <filename>:line_num:col_num: error:
static void error_tok_header(token* tok) {
  fprintf(stderr,
          COLOR_BOLD "<%s>:%zu:%zu:" COLOR_RESET " " COLOR_BOLD_RED
                     "error:" COLOR_RESET " ",
          tok->filename, tok->line_num, tok->col_num);
}

//! Prints a warning message header formatted like the following to stderr:
//! <filename>:line_num:col_num: warning:
static void warn_tok_header(token* tok) {
  fprintf(stderr,
          COLOR_BOLD "<%s>:%zu:%zu:" COLOR_RESET " " COLOR_BOLD_YELLOW
                     "warning:" COLOR_RESET " ",
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

//! Prints an alert message footer formatted like the following to stderr:
//! <line num> |  int foo = @@@;
//!            |            ^~~
//! The offending token and the squiggly line will have `color`.
static void tok_footer(token* tok, const char* color) {
  // Print the line number with the offending line.
  fprintf(stderr, "\t%zu |\t", tok->line_num);
  bool reset = false;
  for (const char* s = tok->line; *s != '\0' && *s != '\n'; ++s) {
    if (s == tok->loc) {
      fprintf(stderr, "%s", color);
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
  fprintf(stderr, "%s", color);
  fputc('^', stderr);
  for (size_t i = 1; i < tok->size; ++i) {
    fputc('~', stderr);
  }
  fprintf(stderr, COLOR_RESET);
  fputc('\n', stderr);
}

void error_tok_fmt(token* tok, char* fmt, ...) {
  error_tok_header(tok);
  va_list args;
  va_start(args, fmt);
  vfprintf(stderr, fmt, args);
  va_end(args);
  fprintf(stderr, "\n");
  tok_footer(tok, COLOR_BOLD_RED);
  exit(1);
}

void warn_tok_fmt(token* tok, char* fmt, ...) {
  warn_tok_header(tok);
  va_list args;
  va_start(args, fmt);
  vfprintf(stderr, fmt, args);
  va_end(args);
  fprintf(stderr, "\n");
  tok_footer(tok, COLOR_BOLD_YELLOW);
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
