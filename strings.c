#include "strings.h"

#include <stdarg.h>
#include <stdbool.h>
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
    return NULL;
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

bool replace_ext(char** path, const char* ext) {
  size_t len = strlen(*path);
  // allocate some extra in case we need to add a dot ('.')
  char* s = realloc(*path, len + strlen(ext) + 2);
  if (!s) {
    return false;
  }

  // get the filename
  char* cur = strrchr(s, '/');
  if (!cur) {
    cur = s;
  } else {
    ++cur;
  }
  // filename may begin with dots
  while (*cur == '.') {
    ++cur;
  }

  char* dot = strrchr(cur, '.');
  if (!dot) {
    dot = s + len;
    strcpy(dot, ".");
  }
  ++dot;
  strcpy(dot, ext);
  *path = s;
  return true;
}
