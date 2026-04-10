#include <getopt.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lexer.h"
#include "strings.h"

static struct option opts[] = {
    {"lex", no_argument, NULL, 1},
    {"parse", no_argument, NULL, 2},
    {"codegen", no_argument, NULL, 3},
    {0, 0, 0, 0},
};

typedef enum compiler_option {
  CO_LEX,
  CO_PARSE,
  CO_CODEGEN,
  CO_DEFAULT,
} compiler_option;

// Replaces the file extension in `path` with `ext`. If `path` has no extension,
// add `ext` as its extension. Note that `ext` must contain the dot ('.')
// character.
static void replace_ext(char* path, const char* ext) {
  path = realloc(path, strlen(path) + strlen(ext) + 1);
  if (!path) {
    exit(1);
  }
  char* dot = strrchr(path, '.');
  if (!dot) {
    dot = path + strlen(path);
  }
  strcpy(dot, ext);
}

// Runs GNU C preprocessor for file specified by `path`. `path` is replaced
// with the path to the output file after execution finishes.
static int run_preprocessor(char* path) {
  char* path_copy = NULL;
  char* command = NULL;

  path_copy = malloc(strlen(path) + 1);
  if (!path_copy) {
    exit(1);
  }
  strcpy(path_copy, path);

  replace_ext(path, ".i");
  // command: cpp -P path_copy -o out_path
  command = string_concat(4, "cpp -P ", path_copy, " -o ", path);
  if (!command) {
    exit(1);
  }

  int res = system(command);
  free(command);
  free(path_copy);
  return res;
}

// Removes the file `path`
static int rm_file(const char* path) {
  // command: rm path
  char* command = string_concat(2, "rm ", path);
  int res = system(command);
  free(command);
  return res;
}

int main(int argc, char* argv[]) {
  int c;
  int opt_index;
  compiler_option opt = CO_DEFAULT;
  while (1) {
    c = getopt_long_only(argc, argv, "", opts, &opt_index);
    if (c == -1) {
      break;
    }

    switch (c) {
      case 1:
        opt = CO_LEX;
        break;
      case 2:
        opt = CO_PARSE;
        break;
      case 3:
        opt = CO_CODEGEN;
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
         opt == CO_DEFAULT ? "<none>" : opts[opt_index].name);

  char* arg = argv[optind];
  char* path = malloc(strlen(arg) + 1);
  if (!path) {
    exit(1);
  }
  strcpy(path, argv[optind]);
  printf("Got input file path: %s\n", path);

  run_preprocessor(path);
  char* res = read_file(path);
  printf("Got file: %s\n", res);
  rm_file(path);

  free(path);
  return 0;
}
