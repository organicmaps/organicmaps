#include "base/SRC_FIRST.hpp"
#include "testing/testing.hpp"

#include "std/limits.hpp"

#include "coding/bit_shift.hpp"


UNIT_TEST(BitShift)
{
  TEST_EQUAL(INT_MIN, bits::ror(1, 1), ());
  uint8_t ui8 = 1;
  TEST_EQUAL(0x80U, bits::ror(ui8, 1), ());
  uint16_t ui16 = 1;
  TEST_EQUAL(0x8000U, bits::ror(ui16, 1), ());
  uint32_t ui32 = 1;
  TEST_EQUAL(0x80000000U, bits::ror(ui32, 1), ());
  uint64_t ui64 = 1;
  TEST_EQUAL(0x8000000000000000LL, bits::ror(ui64, 1), ());

  uint16_t v = 0x58b1;
  TEST_EQUAL(0x2b16, bits::ror(v, 3), ());
  TEST_EQUAL(v, bits::ror(v, 32), ());
  TEST_EQUAL(v, bits::ror(v, 0), ());
}
