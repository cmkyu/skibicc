#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Reads a file from `path` and returns its content.
char* read_file(char* path);

// Returns the length of the identifier starting at the character pointed to
// by `s`. Returns 0 if it is not an identifier.
// An identifier is defined as the following (PCRE syntax):
// [a-zA-Z_]\w*\b
uint64_t lex_identifier(const char* s);

// Returns true if the string starting at `s` of length `len` is a keyword.
// Otherwise returns false. It is assumed that `s` must be an identifier, i.e.,
// lex_identifier(s) > 0.
bool is_keyword(const char* s, size_t len);

// Returns the length of the numerical constant (integer and float) starting at
// the character pointed to by `s`. Returns 0 if it is not a constant. Character
// constants are not supported yet.
uint64_t lex_numeric_constant(const char* s);

// Returns the length of the punctuator starting at the character pointed to by
// `s`. Returns 0 if it is not a punctuator.
// Digraphs and trigrpans are not supported.
uint64_t lex_punctuator(const char* s);
