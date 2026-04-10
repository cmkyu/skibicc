#include "errors.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

void error(char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  vfprintf(stderr, fmt, args);
  fprintf(stderr, "\n");
  exit(1);
}
