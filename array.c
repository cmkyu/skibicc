#include <stddef.h>
#include <stdlib.h>

#include "errors.h"

typedef struct array {
  void* buf;
  size_t size;
  size_t item_size;
  size_t capacity;
} array;

const size_t INITIAL_CAPACITY = 10;

void array_init(array* arr, size_t item_size) {
  arr->size = 0;
  arr->item_size = item_size;
  arr->capacity = INITIAL_CAPACITY;
  arr->buf = calloc(INITIAL_CAPACITY, item_size);
  if (!arr->buf) {
    error("FATAL: array_init(): calloc() failed.");
  }
}

void* array_at(array* arr, size_t index) {
  return (char*)arr + (arr->item_size * index);
}

void* array_push_back(array* arr) {
  if (arr->size >= arr->capacity) {
    arr->capacity *= 2;
    arr->buf = realloc(arr->buf, arr->capacity);
    if (!arr->buf) {
      error("FATAL: array_push_back(): realloc() failed.");
    }
  }
  void* res = array_at(arr, arr->size);
  arr->size += 1;
  return res;
}
