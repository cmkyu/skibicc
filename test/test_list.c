#include <stdint.h>
#include <stdlib.h>

#include "../list.h"
#include "../unity/unity.h"
#include "unity_internals.h"

void setUp(void) {}

void tearDown(void) {}

void free_item(void* data) { free(data); }

void test_list_push_front(void) {
  list* lst = list_init();
  for (uint64_t i = 1; i < 10000; ++i) {
    uint64_t* item = malloc(sizeof(uint64_t));
    *item = i;
    list_push_front(lst, item);
  }
  uint64_t* item = malloc(sizeof(uint64_t));
  *item = 0;
  list_push_back(lst, item);

  TEST_ASSERT_EQUAL(10000, lst->size);
  size_t expected = 9999;
  list_node* cur = lst->head;
  while (cur) {
    TEST_ASSERT_EQUAL(expected, *(uint64_t*)cur->data);
    cur = list_next(cur);
    --expected;
  }

  list_destroy(lst, free_item);
}

void test_list_push_back(void) {
  list* lst = list_init();
  for (uint64_t i = 1; i < 10000; ++i) {
    uint64_t* item = malloc(sizeof(uint64_t));
    *item = i;
    list_push_back(lst, item);
  }
  uint64_t* item = malloc(sizeof(uint64_t));
  *item = 0;
  list_push_front(lst, item);

  TEST_ASSERT_EQUAL(10000, lst->size);
  size_t expected = 0;
  list_node* cur = lst->head;
  while (cur) {
    TEST_ASSERT_EQUAL(expected, *(uint64_t*)cur->data);
    cur = list_next(cur);
    ++expected;
  }

  list_destroy(lst, free_item);
}

void test_list_insert(void) {
  list* lst = list_init();
  uint64_t* num = malloc(sizeof(uint64_t));
  *num = 0;
  list_node* cur = list_push_back(lst, num);
  list_node* mid;
  for (uint64_t i = 1; i < 99; ++i) {
    uint64_t* item = malloc(sizeof(uint64_t));
    *item = i;
    cur = list_insert(lst, cur, item);
    if (i == 50) {
      mid = cur;
    }
  }
  num = malloc(sizeof(uint64_t));
  *num = 99;
  list_push_back(lst, num);

  cur = mid;
  for (uint64_t i = 0; i < 50; ++i) {
    uint64_t* item = malloc(sizeof(uint64_t));
    *item = 999;
    cur = list_insert(lst, cur, item);
  }

  TEST_ASSERT_EQUAL(150, lst->size);
  cur = lst->head;
  for (uint64_t i = 0; i <= 50; ++i) {
    TEST_ASSERT_EQUAL(i, *(uint64_t*)cur->data);
    cur = list_next(cur);
  }
  for (uint64_t i = 0; i < 50; ++i) {
    TEST_ASSERT_EQUAL(999, *(uint64_t*)cur->data);
    cur = list_next(cur);
  }
  for (uint64_t i = 51; i < 100; ++i) {
    TEST_ASSERT_EQUAL(i, *(uint64_t*)cur->data);
    cur = list_next(cur);
  }
  list_destroy(lst, free_item);
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_list_push_front);
  RUN_TEST(test_list_push_back);
  RUN_TEST(test_list_insert);
  return UNITY_END();
}
