//!@file
//!@brief Header file for generic dynamic array.

#ifndef SKIBICC_ARRAY_H
#define SKIBICC_ARRAY_H

#include <stddef.h>

//! Basic dynamic array for storing generic elements. Note that this struct is
//! not suitable for general use outside of this project.
typedef struct array {
  //! Buffer storing the elements.
  void* buf;
  //! Number of elements inside the array.
  size_t size;
  //! Size of the items inside the array.
  size_t item_size;
  //! Size of the `buf` buffer.
  size_t capacity;
} array;

//! Initialize an array `arr` which contains items of `item_size`.
void array_init(array* arr, size_t item_size);

//! Returns the item at `index` of `arr`. Note that NO bound checking is
//! performed.
void* array_at(array* arr, size_t index);

//! Inserts an element into the back of `arr` and returns a pointer to the
//! inserted element.
void* array_push_back(array* arr);

//! Destroys and frees the content of `arr`.
void array_destroy(array* arr);

#endif  // SKIBICC_ARRAY_H
