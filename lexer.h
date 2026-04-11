#include <stdint.h>

char* read_file(char* p);

// Returns the length of the identifier starting at the character pointed to
// by `s`. Returns 0 if it is not an identifier.
// An identifier is defined as the following (PCRE syntax):
// [a-zA-Z_]\w*\b
uint64_t lex_identifier(const char* s);

// Returns the length of the constant starting at the character pointed to by
// `s`. Returns 0 if it is not a constant.
// Only decimal, octal and hexadecimal integers are supported for now.
uint64_t lex_constant(const char* s);
