#include <getopt.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lexer.h"

static struct option opts[] = {
    {"lex", no_argument, NULL, 1},
    {"parse", no_argument, NULL, 2},
    {"codegen", no_argument, NULL, 3},
    {0, 0, 0, 0},
};

typedef enum compiler_option {
  LEX,
  PARSE,
  CODEGEN,
  DEFAULT,
} compiler_option;

// Returns a pointer to the first character of `path`'s extension. If `path`
// has no extensions, returns a pointer to the null byte at the end of `path`.
static char* get_ext(char* path) {
  char* dot = strchrnul(path, '.');
  if (*dot == '\0') {
    return dot;
  }
  return dot + 1;
}

// Returns a string with the file extension of `path` replaced with `ext`. If
// `path` has no extension, add `ext` as its extension. Note that `ext` should
// not contain the dot ('.') character.
// The caller is responsible for deallocating the returned string.
static char* replace_ext(const char* path, const char* ext) {
  // Allocate a little extra in case we need to add "."
  char* out_path = malloc(strlen(path) + strlen(ext) + 2);
  if (!out_path) {
    return NULL;
  }
  strcpy(out_path, path);
  char* ext_dst = get_ext(out_path);
  if (*ext_dst) {  // has extension
    strcpy(ext_dst, ext);
  } else {
    strcpy(ext_dst, ".");
    ++ext_dst;
    strcpy(ext_dst, ext);
  }
  return out_path;
}

// Concatenates the passed strings and return the result. `n` is the number of
// arguments. Returns NULL in case of failure.
// The caller is responsible for deallocating the returned string.
static char* string_concat(int n, ...) {
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

// Runs GNU C preprocessor for file specified by `path`. Returns the path to
// the output file. Returns NULL in case of failure.
// The caller is responsible for deallocating the returned string.
static char* run_preprocessor(const char* path) {
  char* out_path = NULL;
  char* command = NULL;

  out_path = replace_ext(path, "i");
  if (!out_path) {
    goto fail;
  }
  // command: cpp -P path -o out_path
  command = string_concat(4, "cpp -P ", path, " -o ", out_path);
  if (!command) {
    goto fail;
  }

  if (system(command) != 0) {
    goto fail;
  }
  free(command);
  return out_path;

fail:
  free(command);
  free(out_path);
  return NULL;
}

// Removes the file `path`
static int rm_file(const char* path) {
  char* command = string_concat(2, "rm ", path);
  int res = system(command);
  free(command);
  return res;
}

int main(int argc, char* argv[]) {
  int c;
  int opt_index;
  compiler_option opt = DEFAULT;
  while (1) {
    c = getopt_long_only(argc, argv, "", opts, &opt_index);
    if (c == -1) {
      break;
    }

    switch (c) {
      case 1:
        opt = LEX;
        break;
      case 2:
        opt = PARSE;
        break;
      case 3:
        opt = CODEGEN;
        break;
      default:
        return 1;
    }
  }

  if (optind >= argc) {
    fprintf(stderr, "Input file path not specified\n");
    return 1;
  }
  printf("Got argument: %s.\n",
         opt == DEFAULT ? "<none>" : opts[opt_index].name);
  char* path = argv[optind];
  printf("Got input file path: %s\n", path);
  char* ext = get_ext(path);
  printf("Got ext: %s\n", ext);

  path = run_preprocessor(path);
  if (!path) {
    return 1;
  }
  char* res = read_file(path);
  printf("Got file: %s\n", res);

  rm_file(path);
  free(path);
  return 0;
}
