#include <getopt.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "array.h"
#include "codegen.h"
#include "ir.h"
#include "lexer.h"
#include "parser.h"
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

//! Runs GNU C preprocessor for file specified by `path`. `path` is replaced
//! with the path to the output file (.i file) after execution finishes.
static int run_preprocessor(char* path) {
  char* path_copy = strdup(path);
  replace_ext(&path, "i");

  // command: cpp -P path_copy -o out_path
  char* command = string_concat(4, "cpp -P ", path_copy, " -o ", path);
  int res = system(command);
  free(command);
  free(path_copy);
  return res;
}

//! Runs GNU assembler for file specified by `path`.
static int emit_code(char* path) {
  char* obj_path = strdup(path);
  replace_ext(&obj_path, "o");
  // command: as -o obj_path -c path
  char* command = string_concat(4, "as --64 -o ", obj_path, " -c ", path);
  int res = system(command);
  free(command);
  if (res != 0) {
    free(obj_path);
    return res;
  }

  char* prog_path = strdup(path);
  replace_ext(&prog_path, "out");
  // TODO: this is hard-coded. Write a function to find lib paths.
  command = string_concat(
      5, "ld -o ", prog_path,
      " -m elf_x86_64 /usr/lib/Scrt1.o /usr/lib/crti.o "
      "/lib/gcc/x86_64-pc-linux-gnu/16.1.1/crtbeginS.o "
      "-dynamic-linker /lib64/ld-linux-x86-64.so.2 ",
      obj_path,
      " -lc /lib/gcc/x86_64-pc-linux-gnu/16.1.1/crtendS.o /usr/lib/crtn.o");
  res = system(command);
  free(command);
  free(prog_path);
  free(obj_path);
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

  char* path = strdup(argv[optind]);
  if (run_preprocessor(path) != 0) {
    return 1;
  }
  char* text = read_file(path);
  remove(path);

  array tokens = lex(text);
  if (opt == CO_LEX) {
    goto end;
  }
  ast_node* ast = parse(&tokens);
  if (opt == CO_PARSE) {
    goto end;
  }

  replace_ext(&path, "s");
  FILE* f = fopen(path, "w+");
  emit(f, emit_ir(ast));
  fclose(f);
  emit_code(path);
  // TODO: implement memory clean up for tokens, AST, IR and ASM.
end:
  free(path);
  free(text);
  return 0;
}
