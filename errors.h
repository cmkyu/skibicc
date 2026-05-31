//!@file
//!@brief Header file for various error printing functions.

#ifndef SKIBICC_ERRORS_H
#define SKIBICC_ERRORS_H

#include "array.h"
#include "lexer.h"

//! A FIFO queue for storing compilation warnings and errors.
typedef struct alert_queue {
  //! Queue for storing warnings and errors.
  array* queue;
} alert_queue;

//! Initializes the alert queue.
alert_queue* alert_queue_init(void);

//! Pushes a warning for token `tok` with a warning message formatted by `fmt`
//! into the queue `q`.
void alert_queue_push_warning(alert_queue* q, token* tok, char* fmt, ...);

//! Pushes an error for token `tok` with an error message formatted by `fmt`
//! into the queue `q`.
void alert_queue_push_error(alert_queue* q, token* tok, char* fmt, ...);

//! Prints all warnings and errors stored inside `q`. Clears the alert queue.
//! After the first error message is printed (if any), exits the program.
void alert_queue_report(alert_queue* q);

//! Frees the alert queue.
void alert_queue_destroy(alert_queue* q);

//! Reports an error and exits.
void error(char* fmt, ...);

//! Reports a formatted compilation error with `tok`, followed by a message
//! `fmt` and exits.
void error_tok_fmt(token* tok, char* fmt, ...);

//! Reports a formatted compilation warning with `tok`, followed by a message
//! `fmt`.
void warn_tok_fmt(token* tok, char* fmt, ...);

//! Delegates to `malloc`, but if `malloc` returns `NULL`, exits the program
//! with an error message.
void* malloc_safe(size_t size);

//! Delegates to `calloc`, but if `calloc` returns `NULL`, exits the program
//! with an error message.
void* calloc_safe(size_t nelem, size_t elsize);

//! Delegates to `realloc`, but if `calloc` returns `NULL`, exits the program
//! with an error message.
void* realloc_safe(void* ptr, size_t size);

#endif  // SKIBICC_ERRORS_H
