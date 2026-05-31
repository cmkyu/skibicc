//!@file
//!@brief Source file for generic dynamic array.

#include "array.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "errors.h"

//! Initial capacity of the dynamic array.
const size_t INITIAL_CAPACITY = 10;

void array_init(array* arr, size_t item_size) {
  arr->size = 0;
  arr->item_size = item_size;
  arr->capacity = INITIAL_CAPACITY;
  arr->buf = malloc_safe(item_size * INITIAL_CAPACITY);
}

void* array_at(array* arr, size_t index) {
  return (char*)(arr->buf) + (arr->item_size * index);
}

void* array_push_back(array* arr) {
  if (arr->size >= arr->capacity) {
    arr->capacity *= 2;
    void* buf = malloc_safe(arr->item_size * arr->capacity);
    memcpy(buf, arr->buf, arr->size * arr->item_size);
    free(arr->buf);
    arr->buf = buf;
  }
  void* res = array_at(arr, arr->size);
  memset(res, 0, arr->item_size);
  arr->size += 1;
  return res;
}

void array_clear(array* arr) {
  if (arr->size == 0) {
    return;
  }
  memset(arr->buf, 0, arr->size * arr->item_size);
  arr->size = 0;
}

void array_destroy(array* arr) { free(arr->buf); }
