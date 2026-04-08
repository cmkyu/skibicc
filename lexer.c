#include <stdio.h>
#include "status.h"

char* read_file(char* p) {
  FILE* f = fopen(p, "r");
  if (!f) {
    fprintf(stderr, "readfile(): failed to open file: %s.\n", p);
    return NULL;
  }
 
  char* out_buf;
  size_t out_size;
  FILE* out_stream = open_memstream(&out_buf, &out_size); 
  if (!out_stream) {
    fprintf(stderr, "readfile(): failed to create output stream.\n");
    fclose(f);
    return NULL;
  }

  while(1) {
    char buf[4096];
    int res = fread(buf,  1, sizeof(buf), f);
    if (res == 0) {
      break;
    }
    fwrite(buf, 1, res, out_stream);
  }
  fflush(out_stream);
  fclose(out_stream);
  fclose(f);
  return out_buf;
}

