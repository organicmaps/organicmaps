#include "base/SRC_FIRST.hpp"
#include "testing/testing.hpp"

#include "base/bits.hpp"
#include "std/cstdlib.hpp"

namespace
{
  template <typename T> uint32_t PopCountSimple(T x)
  {
    uint32_t res = 0;
    for (; x != 0; x >>= 1)
      if (x & 1)
        ++res;
    return res;
  }
}

UNIT_TEST(Popcount32)
{
  for (uint32_t i = 0; i < 10000; ++i)
  {
    TEST_EQUAL(bits::popcount(i), PopCountSimple(i), (i));
    TEST_EQUAL(bits::popcount(0xC2000000 | i), PopCountSimple(0xC2000000 | i), (0xC2000000 | i));
  }
}

UNIT_TEST(PopcountArray32)
{
  for (uint32_t j = 0; j < 2777; ++j)
  {
    vector<uint32_t> v(j / 10);
    for (size_t i = 0; i < v.size(); ++i)
      v[i] = ((uint32_t(rand()) & 255) << 24) + ((rand() & 255) << 16) +
             ((rand() & 255) << 8) + (rand() & 255);
    uint32_t expectedPopCount = 0;
    for (size_t i = 0; i < v.size(); ++i)
      expectedPopCount += PopCountSimple(v[i]);
    TEST_EQUAL(bits::popcount(v.empty() ? NULL : &v[0], v.size()), expectedPopCount,
               (j, v.size(), expectedPopCount));
  }
}

UNIT_TEST(Select1Test)
{
  TEST_EQUAL(0U, bits::select1(1, 1), ());
}

UNIT_TEST(ROL)
{
  TEST_EQUAL(bits::ROL<uint32_t>(0), 0, ());
  TEST_EQUAL(bits::ROL<uint32_t>(uint32_t(-1)), uint32_t(-1), ());
  TEST_EQUAL(bits::ROL<uint8_t>(128 | 32 | 4), uint8_t(64 | 8 | 1), ());
}

UNIT_TEST(PerfectShuffle)
{
  // 0010 0001 0100 0000
  // 0010 0001 1000 1110
  TEST_EQUAL(bits::PerfectShuffle(557851022), 201547860, ());
  TEST_EQUAL(bits::PerfectUnshuffle(201547860), 557851022, ());
}

UNIT_TEST(ZigZagEncode)
{
  TEST_EQUAL(bits::ZigZagEncode(0),  0, ());
  TEST_EQUAL(bits::ZigZagEncode(-1), 1, ());
  TEST_EQUAL(bits::ZigZagEncode(1),  2, ());
  TEST_EQUAL(bits::ZigZagEncode(-2), 3, ());
  TEST_EQUAL(bits::ZigZagEncode(2),  4, ());
  TEST_EQUAL(bits::ZigZagEncode(127),  254, ());
  TEST_EQUAL(bits::ZigZagEncode(-128), 255, ());
  TEST_EQUAL(bits::ZigZagEncode(128),  256, ());
}

UNIT_TEST(ZigZagDecode)
{
  TEST_EQUAL(bits::ZigZagDecode(0U), 0, ());
  TEST_EQUAL(bits::ZigZagDecode(1U), -1, ());
  TEST_EQUAL(bits::ZigZagDecode(2U),  1, ());
  TEST_EQUAL(bits::ZigZagDecode(3U), -2, ());
  TEST_EQUAL(bits::ZigZagDecode(4U),  2, ());
  TEST_EQUAL(bits::ZigZagDecode(254U),  127, ());
  TEST_EQUAL(bits::ZigZagDecode(255U), -128, ());
  TEST_EQUAL(bits::ZigZagDecode(256U),  128, ());
}

UNIT_TEST(NumHiZeroBits32)
{
  TEST_EQUAL(bits::NumHiZeroBits32(0), 32, ());
  TEST_EQUAL(bits::NumHiZeroBits32(0xFFFFFFFF), 0, ());
  TEST_EQUAL(bits::NumHiZeroBits32(0x0FABCDEF), 4, ());
  TEST_EQUAL(bits::NumHiZeroBits32(0x000000FF), 24, ());
}

UNIT_TEST(NumHiZeroBits64)
{
  TEST_EQUAL(bits::NumHiZeroBits64(0), 64, ());
  TEST_EQUAL(bits::NumHiZeroBits64(0xFFFFFFFFFFFFFFFFULL), 0, ());
  TEST_EQUAL(bits::NumHiZeroBits64(0x0FABCDEF0FABCDEFULL), 4, ());
  TEST_EQUAL(bits::NumHiZeroBits64(0x000000000000FDEFULL), 48, ());
}

UNIT_TEST(NumUsedBits)
{
  TEST_EQUAL(bits::NumUsedBits(0), 0, ());
  TEST_EQUAL(bits::NumUsedBits(0xFFFFFFFFFFFFFFFFULL), 64, ());
  TEST_EQUAL(bits::NumUsedBits(0x0FABCDEF0FABCDEFULL), 60, ());
  TEST_EQUAL(bits::NumUsedBits(0x000000000000FDEFULL), 16, ());
}
