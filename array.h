#ifndef SKIBICC_ARRAY_H
#define SKIBICC_ARRAY_H

#include <stddef.h>

typedef struct array {
  void* buf;
  size_t size;
  size_t item_size;
  size_t capacity;
} array;

void array_init(array* arr, size_t item_size);

void* array_at(array* arr, size_t index);

void* array_push_back(array* arr);

#endif  // SKIBICC_ARRAY_H
