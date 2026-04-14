#include <stdbool.h>
#include <stddef.h>

typedef struct hashmap_entry {
  void* key;
  size_t key_size;  // Size of `key` in bytes.
  void* data;
  size_t data_size;  // Size of `data` in bytes.
} hashmap_entry;

typedef struct hashmap {
  hashmap_entry* arr;
  size_t size;  // Number of entries in the map.
  size_t capacity;
} hashmap;

// TODO: Implement deletion.

// Initializes `map`.
void hashmap_init(hashmap* map);

// Inserts `entry` into `map`. Returns true if the insertion is successful.
// Otherwise (i.e., `entry` already exists in `map`), returns false.
bool hashmap_insert(hashmap* map, const hashmap_entry* entry);

// Returns the hashmap entry in `map` that has `key`. `key_size` is the size of
// `key` in bytes. Returns NULL if `key` is not found in `map`.
hashmap_entry* hashmap_get(hashmap* map, void* key, size_t key_size);

// Frees all of `map`'s contents. Do not use this if your data does not live on
// the heap. It's the caller's responsibility to free `map` itself.
void hashmap_destroy(hashmap* map);
