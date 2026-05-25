#include <stdint.h>

#include "../unicode.h"
#include "../unity/unity.h"
#include "array.h"

void setUp(void) {}

void tearDown(void) {}

void test_decode_utf8(void) {
  // Hello, world! in chinese.
  char str[] =
      "\xe4\xbd\xa0\xe5\xa5\xbd\xef\xbc\x8c\xe4\xb8\x96\xe7\x95\x8c\xef\xbc"
      "\x81";
  uint32_t actual[64];

  const char* c = str;
  size_t i = 0;
  while (*c) {
    c = decode_utf8(c, &actual[i]);
    ++i;
  }

  uint32_t expected[] = {0x4f60, 0x597d, 0xff0c, 0x4e16, 0x754c, 0xff01};
  for (uint32_t i = 0; i < 6; ++i) {
    TEST_ASSERT_EQUAL(expected[i], actual[i]);
  }
}

void test_encode_utf16(void) {
  // One code unit.
  {
    array arr;
    array_init(&arr, sizeof(uint16_t));
    // Euro sign.
    encode_utf16(0x20ac, &arr);

    TEST_ASSERT_EQUAL(1, arr.size);
    uint16_t* actual = array_at(&arr, 0);
    TEST_ASSERT_EQUAL(0x20ac, *actual);
    array_destroy(&arr);
  }

  // Two code units.
  {
    array arr;
    array_init(&arr, sizeof(uint16_t));
    // Letter "Yee" of Deseret alphabet.
    encode_utf16(0x10437, &arr);

    TEST_ASSERT_EQUAL(2, arr.size);
    uint16_t* actual = array_at(&arr, 0);
    TEST_ASSERT_EQUAL(0xd801, *actual);
    actual = array_at(&arr, 1);
    TEST_ASSERT_EQUAL(0xdc37, *actual);
    array_destroy(&arr);
  }
}

void test_encode_utf32(void) {
  array arr;
  array_init(&arr, sizeof(uint32_t));
  // Downward arrow.
  encode_utf32(0x1f813, &arr);

  TEST_ASSERT_EQUAL(1, arr.size);
  uint32_t* actual = array_at(&arr, 0);
  TEST_ASSERT_EQUAL(0x1f813, *actual);

  array_destroy(&arr);
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_decode_utf8);
  RUN_TEST(test_encode_utf16);
  RUN_TEST(test_encode_utf32);
  return UNITY_END();
}
