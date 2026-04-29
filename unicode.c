#include "unicode.h"

#include <stdint.h>

#include "array.h"
#include "errors.h"

// Credit: https://github.com/rui314/chibicc/blob/main/tokenize.c#L270
void encode_utf16(uint32_t c, array* arr) {
  if (c <= 0xFFFF) {
    uint16_t* item = array_push_back(arr);
    *item = (uint16_t)c;
    return;
  }

  c -= 0x10000;
  uint16_t w1 = ((c >> 10) & 0x3FF) + 0xD800;
  uint16_t w2 = (c & 0x3FF) + 0xDC00;

  uint16_t* item = array_push_back(arr);
  *item = w1;
  item = array_push_back(arr);
  *item = w2;
}

void encode_utf32(uint32_t c, array* arr) {
  uint32_t* item = array_push_back(arr);
  *item = c;
}

// Credit: https://github.com/rui314/chibicc/blob/main/unicode.c#L37
const char* decode_utf8(const char* s, uint32_t* out) {
  uint8_t c = *s;
  if (c <= 0x7F) {
    *out = (uint32_t)c;
    return s + 1;
  }

  uint32_t res = 0;
  size_t len = 1;
  if (c >= 0xF0) {  // Binary 11110000
    res = c & 0x07;
    len = 4;
  } else if (c >= 0xE0) {  // Binary 11100000
    res = c & 0x0F;
    len = 3;
  } else if (c >= 0xC0) {  // Binary 11000000
    res = c & 0x1F;
    len = 2;
  } else {
    error("FATAL: invalid utf8 encoding: 0x%x", c);
  }

  for (size_t i = 1; i < len; ++i) {
    ++s;
    c = *s;
    res = (res << 6) | (c & 0x3F);
  }
  // Skip the last consumed character.
  ++s;
  *out = res;
  return s;
}
