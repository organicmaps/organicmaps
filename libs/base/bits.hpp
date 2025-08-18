#pragma once

#include "base/assert.hpp"

#include <bit>
#include <cstdint>
#include <limits>
#include <type_traits>

namespace bits
{
static constexpr int SELECT1_ERROR = -1;

template <typename T>
unsigned int select1(T x, unsigned int i)
{
  // TODO: Fast implementation of select1.
  ASSERT(i > 0 && i <= sizeof(T) * 8, (i));
  for (unsigned int j = 0; j < sizeof(T) * 8; x >>= 1, ++j)
    if (x & 1)
      if (--i == 0)
        return j;
  return static_cast<unsigned int>(SELECT1_ERROR);
}

constexpr uint8_t FloorLog(uint64_t x) noexcept
{
  return x == 0 ? 0 : std::bit_width(x) - 1;
}

template <typename T>
std::make_unsigned_t<T> ZigZagEncode(T x)
{
  static_assert(std::is_signed<T>::value, "Type should be signed");
  return (x << 1) ^ (x >> (sizeof(x) * 8 - 1));
}

template <typename T>
std::make_signed_t<T> ZigZagDecode(T x)
{
  static_assert(std::is_unsigned<T>::value, "Type should be unsigned.");
  return (x >> 1) ^ -static_cast<std::make_signed_t<T>>(x & 1);
}

// Perform a perfect shuffle of the bits of 64-bit integer, shuffling bits from
// 'abcd efgh ijkl mnop ABCD EFGH IJKL MNOP' to 'aAbB cCdD eEfF gGhH iIjJ kKlL mMnN oOpP'.
// See http://www.icodeguru.com/Embedded/Hacker's-Delight/047.htm
constexpr uint64_t PerfectShuffle(uint64_t x)
{
  x = ((x & 0x00000000FFFF0000ULL) << 16) | ((x >> 16) & 0x00000000FFFF0000ULL) | (x & 0xFFFF00000000FFFFULL);
  x = ((x & 0x0000FF000000FF00ULL) << 8) | ((x >> 8) & 0x0000FF000000FF00ULL) | (x & 0xFF0000FFFF0000FFULL);
  x = ((x & 0x00F000F000F000F0ULL) << 4) | ((x >> 4) & 0x00F000F000F000F0ULL) | (x & 0xF00FF00FF00FF00FULL);
  x = ((x & 0x0C0C0C0C0C0C0C0CULL) << 2) | ((x >> 2) & 0x0C0C0C0C0C0C0C0CULL) | (x & 0xC3C3C3C3C3C3C3C3ULL);
  x = ((x & 0x2222222222222222ULL) << 1) | ((x >> 1) & 0x2222222222222222ULL) | (x & 0x9999999999999999ULL);
  return x;
}

// Reverses the perfect shuffle of bits in a 64-bit integer
constexpr uint64_t PerfectUnshuffle(uint64_t x)
{
  x = ((x & 0x2222222222222222ULL) << 1) | ((x >> 1) & 0x2222222222222222ULL) | (x & 0x9999999999999999ULL);
  x = ((x & 0x0C0C0C0C0C0C0C0CULL) << 2) | ((x >> 2) & 0x0C0C0C0C0C0C0C0CULL) | (x & 0xC3C3C3C3C3C3C3C3ULL);
  x = ((x & 0x00F000F000F000F0ULL) << 4) | ((x >> 4) & 0x00F000F000F000F0ULL) | (x & 0xF00FF00FF00FF00FULL);
  x = ((x & 0x0000FF000000FF00ULL) << 8) | ((x >> 8) & 0x0000FF000000FF00ULL) | (x & 0xFF0000FFFF0000FFULL);
  x = ((x & 0x00000000FFFF0000ULL) << 16) | ((x >> 16) & 0x00000000FFFF0000ULL) | (x & 0xFFFF00000000FFFFULL);
  return x;
}

// Returns the integer that has the bits of |x| at even-numbered positions
// and the bits of |y| at odd-numbered positions without changing the
// relative order of bits coming from |x| and |y|.
// That is, if the bits of |x| are {x31, x30, ..., x0},
//         and the bits of |y| are {y31, y30, ..., y0},
// then the bits of the result are {y31, x31, y30, x30, ..., y0, x0}.
constexpr uint64_t BitwiseMerge(uint32_t x, uint32_t y)
{
  return PerfectShuffle((static_cast<uint64_t>(y) << 32) | x);
}

constexpr void BitwiseSplit(uint64_t v, uint32_t & x, uint32_t & y)
{
  uint64_t unshuffle = PerfectUnshuffle(v);
  x = static_cast<uint32_t>(unshuffle);
  y = static_cast<uint32_t>(unshuffle >> 32);
}

// Returns 1 if bit is set and 0 otherwise.
inline uint8_t GetBit(void const * p, uint32_t offset)
{
  uint8_t const * pData = static_cast<uint8_t const *>(p);
  return (pData[offset >> 3] >> (offset & 7)) & 1;
}

inline void SetBitTo0(void * p, uint32_t offset)
{
  uint8_t * pData = static_cast<uint8_t *>(p);
  pData[offset >> 3] &= ~(1 << (offset & 7));
}

inline void SetBitTo1(void * p, uint32_t offset)
{
  uint8_t * pData = static_cast<uint8_t *>(p);
  pData[offset >> 3] |= (1 << (offset & 7));
}

// Compute number of zero bits from the most significant bits side.
constexpr uint32_t NumHiZeroBits32(uint32_t n)
{
  if (n == 0)
    return 32;
  uint32_t result = 0;
  while ((n & (uint32_t{1} << 31)) == 0)
  {
    ++result;
    n <<= 1;
  }
  return result;
}

constexpr uint32_t NumHiZeroBits64(uint64_t n)
{
  if (n == 0)
    return 64;
  uint32_t result = 0;
  while ((n & (uint64_t{1} << 63)) == 0)
  {
    ++result;
    n <<= 1;
  }
  return result;
}

// Computes number of bits needed to store the number, it is not equal to number of ones.
// E.g. if we have a number (in bit representation) 00001000b then NumUsedBits is 4.
constexpr uint32_t NumUsedBits(uint64_t n)
{
  uint32_t result = 0;
  while (n != 0)
  {
    ++result;
    n >>= 1;
  }
  return result;
}

constexpr uint64_t GetFullMask(uint8_t numBits)
{
  ASSERT_LESS_OR_EQUAL(numBits, 64, ());
  return numBits == 64 ? std::numeric_limits<uint64_t>::max() : (static_cast<uint64_t>(1) << numBits) - 1;
}

constexpr bool IsPow2Minus1(uint64_t n)
{
  return (n & (n + 1)) == 0;
}
}  // namespace bits
