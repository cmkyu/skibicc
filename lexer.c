#include <stdio.h>
#include "status.h"

static char* read_file(char* p) {
  char* out_buf = NULL;
  size_t out_size;
  FILE* out_stream = open_memstream(&out_buf, &out_size); 
  if (!out_stream) {
    goto cleanup;
  }

  FILE* f = fopen(p, "r");
  if (!f) {
    goto cleanup;
  }
 
  int read_res = 1;
  while(read_res) {
    char buf[4096];
    read_res = fread(buf,  1, sizeof(buf), f);
    if (read_res != 0) {
      fwrite(buf, 1, read_res, out_stream);
    }
  }
  fflush(out_stream);

cleanup:
  fclose(out_stream);
  fclose(f);
  return out_buf;
}

int main(int argc, char* argv[]) {
  char* res = read_file("test.txt");
  printf("Got file: %s", res);
  return 0;
}
