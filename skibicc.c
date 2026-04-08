#include <stdio.h>
#include "lexer.h"
#include <getopt.h>

static struct option opts[] = {
  {"lex", no_argument, NULL, 0 },
  {"parse", no_argument, NULL, 1 },
  {"codegen", no_argument, NULL, 2 },
};

typedef enum compiler_option {
  LEX,
  PARSE,
  CODEGEN,
  DEFAULT,
} compiler_option;

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
      case 0:
        opt = LEX;
        break;
      case 1:
        opt = PARSE;
        break;
      case 2:
        opt = CODEGEN;
        break;
      default:
        fprintf(stderr, "Unknown argument code: %c.\n", c);
        break;
    }
  }

  printf("Got argument: %s.\n", opts[opt_index].name);

  if (optind >= argc) {
    fprintf(stderr, "Input file path not specified\n");
    return 1;
  }

  char* path = argv[optind];
  printf("Got input file path: %s\n", path);
  char* res = read_file(path);
  printf("Got file: %s", res);
  return 0;
}

