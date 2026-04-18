#ifndef SKIBICC_HASHMAP_H
#define SKIBICC_HASHMAP_H

#include <stdbool.h>
#include <stddef.h>

//! Represents the <key, value> pairs inside a `hashmap`. `key` and `data`
//! may refer to either heap or stack memory. Do NOT use `hashmap_destroy()`
//! if `key` or `data` refers to stack memory.
typedef struct hashmap_entry {
  //! Key of the entry. May point to either stack or head memory.
  void* key;
  //! Size of `key` in bytes.
  size_t key_size;
  //! Value of the entry. May point to either stack or heap memory.
  void* data;
  //! Size of `data` in bytes.
  size_t data_size;
} hashmap_entry;

//! A hashmap with open addressing and linear probing. Note that the
//! implementation does not focus on performance, and so this struct is not
//! suitable for general use outside of this project.
typedef struct hashmap {
  //! Dynamic array for holding `hashmap_entry`s.
  hashmap_entry* arr;
  //! Number of entries in the map.
  size_t size;
  //! Capacity of the dynamic array `arr`.
  size_t capacity;
} hashmap;

//! Initializes `map`.
void hashmap_init(hashmap* map);

//! Inserts `entry` into `map`. Returns true if the insertion is successful.
//! Otherwise (i.e., `entry` already exists in `map`), returns false.
bool hashmap_insert(hashmap* map, const hashmap_entry* entry);

//! Returns the hashmap entry in `map` that has `key`. `key_size` is the size of
//! `key` in bytes. Returns NULL if `key` is not found in `map`.
hashmap_entry* hashmap_get(hashmap* map, const void* key, size_t key_size);

//! Removes the hashmap entry in `map` that has `key`. `key_size` is the size of
//! `key` in bytes. Returns the removed entry. If `key` does not exist in `map`,
//! returns an entry whose members are all set to 0.
//! It's the caller's responsibility to free the key and the data of the removed
//! entry.
hashmap_entry hashmap_remove(hashmap* map, const void* key, size_t key_size);

//! Frees all of `map`'s contents. Do not use this if your data does not live on
//! the heap.
//! It's the caller's responsibility to free `map` itself.
void hashmap_destroy(hashmap* map);

#endif
