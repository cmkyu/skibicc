#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../hashmap.h"
#include "../unity/unity.h"

void setUp(void) {}

void tearDown(void) {}

void test_hashmap_simple(void) {
  hashmap map;
  hashmap_init(&map);

  char key[] = "foo";
  char data[] = "bar";
  hashmap_entry entry = {
      .key = key,
      .key_size = sizeof(key),
      .data = data,
      .data_size = sizeof(data),
  };

  TEST_ASSERT_TRUE(hashmap_insert(&map, &entry));
  TEST_ASSERT_EQUAL(1, map.size);
  hashmap_entry* res = hashmap_get(&map, key, sizeof(key));
  TEST_ASSERT_TRUE(res);
  TEST_ASSERT_EQUAL_STRING(key, res->key);
  TEST_ASSERT_EQUAL(sizeof(key), res->key_size);
  TEST_ASSERT_EQUAL_STRING(data, res->data);
  TEST_ASSERT_EQUAL(sizeof(data), res->data_size);

  res = hashmap_get(&map, "foobar", 7);
  TEST_ASSERT_FALSE(res);

  entry = hashmap_remove(&map, "foo", 4);
  TEST_ASSERT_EQUAL(0, map.size);
  TEST_ASSERT_EQUAL_STRING("foo", entry.key);
  TEST_ASSERT_EQUAL(4, entry.key_size);
  TEST_ASSERT_EQUAL_STRING("bar", entry.data);
  TEST_ASSERT_EQUAL(4, entry.data_size);
  res = hashmap_get(&map, "foo", 4);
  TEST_ASSERT_FALSE(res);

  entry = hashmap_remove(&map, "foo", 4);
  TEST_ASSERT_EQUAL(0, entry.key);
  TEST_ASSERT_EQUAL(0, entry.key_size);
  TEST_ASSERT_EQUAL(0, entry.data);
  TEST_ASSERT_EQUAL(0, entry.data_size);

  // Nothing to destroy. Shouldn't crash.
  hashmap_destroy(&map);
}

void test_hashmap_stress(void) {
  // Test data courtesy of:
  // https://github.com/dwyl/english-words/blob/master/words.txt
  FILE* fp = fopen("./test/data/words.txt", "r");
  if (fp == NULL) {
    exit(1);
  }

  // Test insertion.
  char* line = NULL;
  ssize_t read;
  size_t len;
  hashmap map;
  hashmap_init(&map);
  while ((read = getline(&line, &len, fp)) != -1) {
    bool* data = malloc(sizeof(bool));
    *data = true;
    hashmap_entry entry = {
        .key = strdup(line),
        .key_size = read,
        .data = data,
        .data_size = sizeof(bool),
    };
    TEST_ASSERT_TRUE(hashmap_insert(&map, &entry));
  }
  TEST_ASSERT_EQUAL(466550, map.size);
  free(line);
  line = NULL;

  // Test get.
  rewind(fp);
  while ((read = getline(&line, &len, fp)) != -1) {
    hashmap_entry* entry = hashmap_get(&map, line, read);
    TEST_ASSERT_TRUE(entry);
    TEST_ASSERT_EQUAL_STRING(line, entry->key);
    TEST_ASSERT_EQUAL(read, entry->key_size);
    TEST_ASSERT_TRUE(*(bool*)entry->data);
    TEST_ASSERT_EQUAL(sizeof(bool), entry->data_size);
  }
  TEST_ASSERT_EQUAL(466550, map.size);
  free(line);
  line = NULL;

  // Test remove.
  rewind(fp);
  size_t i = 0;
  // words.txt has >400K lines. Remove the first 400K from the hashmap.
  size_t remove_threshold = 400000;
  while ((read = getline(&line, &len, fp)) != -1) {
    if (i < remove_threshold) {
      hashmap_entry entry = hashmap_remove(&map, line, read);
      TEST_ASSERT_EQUAL_STRING(line, entry.key);
      TEST_ASSERT_EQUAL(read, entry.key_size);
      TEST_ASSERT_TRUE(*(bool*)entry.data);
      TEST_ASSERT_EQUAL(sizeof(bool), entry.data_size);
      free(entry.key);
      free(entry.data);
    }
    ++i;
  }
  TEST_ASSERT_EQUAL(66550, map.size);
  free(line);
  line = NULL;

  i = 0;
  // Check if removal was successful.
  while ((read = getline(&line, &len, fp)) != -1) {
    hashmap_entry* entry = hashmap_get(&map, line, read);
    if (i < remove_threshold) {
      TEST_ASSERT_FALSE(entry);
    } else {
      TEST_ASSERT_TRUE(entry);
      TEST_ASSERT_EQUAL_STRING(line, entry->key);
      TEST_ASSERT_EQUAL(read, entry->key_size);
      TEST_ASSERT_TRUE(*(bool*)entry->data);
      TEST_ASSERT_EQUAL(sizeof(bool), entry->data_size);
    }
    ++i;
  }
  TEST_ASSERT_EQUAL(66550, map.size);
  free(line);
  line = NULL;

  hashmap_destroy(&map);
  fclose(fp);
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_hashmap_simple);
  RUN_TEST(test_hashmap_stress);
  return UNITY_END();
}
