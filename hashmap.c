#include "hashmap.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "errors.h"

// Power of 2 for fast calculation of indices from hash.
const size_t INITIAL_SIZE = 256;
// A bit lower than the optimal 0.7 to be conservative.
const double LOAD_FACTOR = 0.6;

// FNV1a hash
// `key` is the key to be hashed. `key_size` is the size of `key` in bytes.
// Credit: https://github.com/aappleby/smhasher/blob/master/src/Hashes.cpp
static uint64_t fnv1a(const void* key, size_t key_size) {
  uint64_t h = 0xcbf29ce484222325ULL;
  const uint8_t* data = (const uint8_t*)key;
  for (size_t i = 0; i < key_size; ++i) {
    h ^= data[i];
    h *= 0x100000001b3ULL;
  }
  return h;
}

// Returns the index of `key` inside the entry array, given the array
// `capacity`. `key_size` is the size of `key` in bytes.
static size_t get_index(const void* key, size_t key_size, size_t capacity) {
  // Equivalent to hash % (map->capacity) if map->capacity is a power of 2.
  // Credit: https://stackoverflow.com/questions/70089037
  return fnv1a(key, key_size) & (capacity - 1);
}

// Returns true if `lhs` and `rhs` have the same key. Otherwise returns false.
static bool is_key_equal(const hashmap_entry* lhs, const hashmap_entry* rhs) {
  return lhs->key_size == rhs->key_size &&
         strncmp(lhs->key, rhs->key, rhs->key_size) == 0;
}

// Returns the value of advancing `index` by 1, wrapping back around if it
// exceeds `capacity`.
static size_t advance_index(size_t index, size_t capacity) {
  // Equivalent to (index + 1) % capacity
  return (index + 1) & (capacity - 1);
}

// Inserts `entry` into `map`, performing linear probing in case of hash
// collision. Returns true if insertion is successful. Otherwise (i.e.,
// `entry`'s key is already present in `map`) returns false. It is assumed that
// `map` has enough capacity and its load factor is below the rehash threshold.
static bool maybe_probe_and_insert(hashmap* map, const hashmap_entry* entry) {
  size_t index = get_index(entry->key, entry->key_size, map->capacity);
  hashmap_entry* arr_entry = &map->arr[index];
  if (arr_entry->key) {
    if (is_key_equal(arr_entry, entry)) {
      // Key already inserted.
      return false;
    }
    // Hash collision. Do linear probing.
    while (true) {
      index = advance_index(index, map->capacity);
      if (!map->arr[index].key) {
        arr_entry = &map->arr[index];
        break;
      }
    }
  }
  *arr_entry = *entry;
  ++map->size;
  return true;
}

// Doubles `map`'s capacity and rehashes its elements based on its new capacity.
static void rehash(hashmap* map) {
  size_t old_capacity = map->capacity;
  map->capacity *= 2;
  hashmap_entry* arr = calloc(map->capacity, sizeof(hashmap_entry));
  if (!arr) {
    error("FATAL: rehash(): calloc() failed.");
  }
  // Swap.
  hashmap_entry* tmp = map->arr;
  map->arr = arr;
  arr = tmp;
  for (size_t i = 0; i < old_capacity; ++i) {
    if (!arr[i].key) {
      continue;
    }
    maybe_probe_and_insert(map, &arr[i]);
  }
  free(arr);
}

void hashmap_init(hashmap* map) {
  map->arr = calloc(INITIAL_SIZE, sizeof(hashmap_entry));
  if (!map->arr) {
    error("FATAL: hashmap_init(): calloc() failed.");
  }
  map->size = 0;
  map->capacity = INITIAL_SIZE;
}

bool hashmap_insert(hashmap* map, const hashmap_entry* entry) {
  if ((double)map->size / map->capacity >= LOAD_FACTOR) {
    rehash(map);
  }
  return maybe_probe_and_insert(map, entry);
}

hashmap_entry* hashmap_get(hashmap* map, const void* key, size_t key_size) {
  size_t index = get_index(key, key_size, map->capacity);
  hashmap_entry* res = &map->arr[index];
  const hashmap_entry needle = {
      .key = (void*)key,
      .key_size = key_size,
  };
  while (res->key && !is_key_equal(res, &needle)) {
    // Probe linearly when we have a hash collision.
    index = advance_index(index, map->capacity);
    res = &map->arr[index];
  }
  return res->key ? res : NULL;
}

hashmap_entry hashmap_remove(hashmap* map, const void* key, size_t key_size) {
  hashmap_entry* entry = hashmap_get(map, key, key_size);
  hashmap_entry res;
  memset(&res, 0, sizeof(hashmap_entry));
  if (!entry) {
    return res;
  }
  // Remove the entry. This creates an empty slot.
  memcpy(&res, entry, sizeof(hashmap_entry));
  memset(entry, 0, sizeof(hashmap_entry));

  size_t index = entry - map->arr;
  hashmap_entry* cur = entry + 1;
  // Move entry whose index (i.e., hash % capacity) is earlier than or equal to
  // the empty slot into the empty slot.
  // See: https://en.wikipedia.org/wiki/Linear_probing#Deletion
  while (cur->key) {
    if (get_index(cur->key, cur->key_size, map->capacity) <= index) {
      *entry = *cur;
      memset(cur, 0, sizeof(hashmap_entry));
      entry = cur;
      index = entry - map->arr;
    }
    ++cur;
  }
  return res;
}

void hashmap_destroy(hashmap* map) {
  for (size_t i = 0; i < map->capacity; ++i) {
    free(map->arr[i].key);
    free(map->arr[i].data);
  }
}
