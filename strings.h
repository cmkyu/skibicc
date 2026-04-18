#ifndef SKIBICC_STRINGS_H
#define SKIBICC_STRINGS_H

#include <stdbool.h>
#include <stdint.h>

//! Concatenates the passed strings and return the result. `n` is the number of
//! arguments.
//! The caller is responsible for deallocating the returned string.
char* string_concat(int n, ...);

//! Replaces the file extension of `path` with `ext`. If `path` does not have a
//! file extension, `ext` is appended to the end of `path`.
void replace_ext(char** path, const char* ext);

#endif  // SKIBICC_STRINGS_H
