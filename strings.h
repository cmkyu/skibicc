#include <stdbool.h>
#include <stdint.h>

#ifndef SKIBICC_STRINGS_H
#define SKIBICC_STRINGS_H

// Concatenates the passed strings and return the result. `n` is the number of
// arguments. Returns NULL in case of failure.
// The caller is responsible for deallocating the returned string.
char* string_concat(int n, ...);

// Replaces the file extension of `path` with `ext`. Returns true if successful,
// otherwise returns false.
bool replace_ext(char** path, const char* ext);

#endif  // SKIBICC_STRINGS_H
