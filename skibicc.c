#include <stdio.h>
#include "lexer.h"

int main(int argc, char* argv[]) {
  char* res = read_file("test.txt");
  printf("Got file: %s", res);
  return 0;
}

