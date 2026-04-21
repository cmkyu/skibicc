#include <stdint.h>

#include "../array.h"
#include "../unity/unity.h"

void setUp(void) {}

void tearDown(void) {}

void test_array(void) {
  array arr;
  array_init(&arr, sizeof(uint64_t));
  for (uint64_t i = 0; i < 10000; ++i) {
    uint64_t* item = array_push_back(&arr);
    *item = i;
  }

  for (uint64_t i = 0; i < 10000; ++i) {
    uint64_t* item = array_at(&arr, i);
    TEST_ASSERT_EQUAL(i, *item);
  }
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_array);
  return UNITY_END();
}
