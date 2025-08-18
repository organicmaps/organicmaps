#include "testing/testing.hpp"

#include "coding/endianness.hpp"

UNIT_TEST(Endianness1Byte)
{
  TEST_EQUAL(uint8_t(0), ReverseByteOrder<uint8_t>(0), ());
  TEST_EQUAL(uint8_t(17), ReverseByteOrder<uint8_t>(17), ());
  TEST_EQUAL(uint8_t(255), ReverseByteOrder<uint8_t>(255), ());

  TEST_EQUAL(uint8_t(0), ReverseByteOrder<uint8_t>(0), ());
  TEST_EQUAL(uint8_t(17), ReverseByteOrder<uint8_t>(17), ());
  TEST_EQUAL(uint8_t(255), ReverseByteOrder<uint8_t>(255), ());
}

UNIT_TEST(Endianness12Bytes)
{
  TEST_EQUAL(uint16_t(0), ReverseByteOrder<uint16_t>(0), ());
  TEST_EQUAL(uint16_t(256), ReverseByteOrder<uint16_t>(1), ());
  TEST_EQUAL(uint16_t(0xE8FD), ReverseByteOrder<uint16_t>(0xFDE8), ());
  TEST_EQUAL(uint16_t(0xFFFF), ReverseByteOrder<uint16_t>(0xFFFF), ());

  TEST_EQUAL(uint16_t(0), ReverseByteOrder<uint16_t>(0), ());
  TEST_EQUAL(uint16_t(256), ReverseByteOrder<uint16_t>(1), ());
  TEST_EQUAL(uint16_t(0xE8FD), ReverseByteOrder<uint16_t>(0xFDE8), ());
  TEST_EQUAL(uint16_t(0xFFFF), ReverseByteOrder<uint16_t>(0xFFFF), ());
}

UNIT_TEST(Endianness18Bytes)
{
  TEST_EQUAL(0ULL, ReverseByteOrder(0ULL), ());
  TEST_EQUAL(1ULL, ReverseByteOrder(1ULL << 56), ());
  TEST_EQUAL(0xE2E4D7D5B1C3B8C6ULL, ReverseByteOrder(0xC6B8C3B1D5D7E4E2ULL), ());
  TEST_EQUAL(0xFFFFFFFFFFFFFFFFULL, ReverseByteOrder(0xFFFFFFFFFFFFFFFFULL), ());

  TEST_EQUAL(0ULL, ReverseByteOrder(0ULL), ());
  TEST_EQUAL(1ULL, ReverseByteOrder(1ULL << 56), ());
  TEST_EQUAL(0xE2E4D7D5B1C3B8C6ULL, ReverseByteOrder(0xC6B8C3B1D5D7E4E2ULL), ());
  TEST_EQUAL(0xFFFFFFFFFFFFFFFFULL, ReverseByteOrder(0xFFFFFFFFFFFFFFFFULL), ());
}
