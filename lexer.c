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
 
  while(1) {
    char buf[4096];
    int res = fread(buf,  1, sizeof(buf), f);
    if (res == 0) {
      break;
    }
    fwrite(buf, 1, res, out_stream);
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
