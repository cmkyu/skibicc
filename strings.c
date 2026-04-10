#include "strings.h"

#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

char* string_concat(int n, ...) {
  size_t len = 0;
  size_t lens[n];
  va_list args;
  va_start(args, n);
  for (int i = 0; i < n; ++i) {
    const char* s = va_arg(args, const char*);
    size_t l = strlen(s);
    len += l;
    lens[i] = l;
  }
  va_end(args);

  char* cur = malloc(len + 1);
  if (!cur) {
    exit(1);
  }
  char* res = cur;
  va_start(args, n);
  for (int i = 0; i < n; ++i) {
    const char* s = va_arg(args, const char*);
    memcpy(cur, s, lens[i]);
    cur += lens[i];
  }
  va_end(args);
  *cur = '\0';
  return res;
}
