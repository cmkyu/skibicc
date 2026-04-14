#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

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
  hashmap_entry* res = hashmap_get(&map, key, sizeof(key));
  TEST_ASSERT_EQUAL_STRING(key, res->key);
  TEST_ASSERT_EQUAL(sizeof(key), res->key_size);
  TEST_ASSERT_EQUAL_STRING(data, res->data);
  TEST_ASSERT_EQUAL(sizeof(data), res->data_size);
}

void test_hashmap_insert_and_get(void) {
  // Test data courtesy of:
  // https://github.com/dwyl/english-words/blob/master/words.txt
  FILE* fp = fopen("./test/data/words.txt", "r");
  if (fp == NULL) {
    exit(1);
  }

  char* line = NULL;
  ssize_t read;
  size_t len;
  hashmap map;
  hashmap_init(&map);
  while ((read = getline(&line, &len, fp)) != -1) {
    bool* data = malloc(sizeof(bool));
    *data = true;
    hashmap_entry entry = {
        .key = line,
        .key_size = read,
        .data = data,
        .data_size = sizeof(bool),
    };
    TEST_ASSERT_TRUE(hashmap_insert(&map, &entry));
    line = NULL;
  }

  line = NULL;
  rewind(fp);
  while ((read = getline(&line, &len, fp)) != -1) {
    hashmap_entry* entry = hashmap_get(&map, line, read);
    TEST_ASSERT_TRUE(entry);
    TEST_ASSERT_EQUAL_STRING(line, entry->key);
    TEST_ASSERT_EQUAL(read, entry->key_size);
    TEST_ASSERT_TRUE(*(bool*)entry->data);
    TEST_ASSERT_EQUAL(sizeof(bool), entry->data_size);
    free(line);
    line = NULL;
  }

  hashmap_destroy(&map);
  fclose(fp);
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_hashmap_simple);
  RUN_TEST(test_hashmap_insert_and_get);
  return UNITY_END();
}
