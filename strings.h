#include <stdint.h>

#ifndef SKIBICC_STRINGS_H
#define SKIBICC_STRINGS_H

// Concatenates the passed strings and return the result. `n` is the number of
// arguments. Returns NULL in case of failure.
// The caller is responsible for deallocating the returned string.
char* string_concat(int n, ...);

#endif  // SKIBICC_STRINGS_H
