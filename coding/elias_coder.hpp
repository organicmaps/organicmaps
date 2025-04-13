#pragma once

#include "coding/bit_streams.hpp"

#include "base/assert.hpp"
#include "base/bits.hpp"

#include <cstdint>

namespace coding
{
class GammaCoder
{
public:
  template <typename TWriter>
  static bool Encode(BitWriter<TWriter> & writer, uint64_t value)
  {
    if (value == 0)
      return false;

    uint8_t const n = bits::FloorLog(value);
    ASSERT_LESS_OR_EQUAL(n, 63, ());

    uint64_t const msb = static_cast<uint64_t>(1) << n;
    writer.WriteAtMost64Bits(msb, n + 1);
    writer.WriteAtMost64Bits(value, n);
    return true;
  }

  template <typename TReader>
  static uint64_t Decode(BitReader<TReader> & reader)
  {
    uint8_t n = 0;
    while (reader.Read(1) == 0)
      ++n;

    ASSERT_LESS_OR_EQUAL(n, 63, ());

    uint64_t const msb = static_cast<uint64_t>(1) << n;
    return msb | reader.ReadAtMost64Bits(n);
  }
};

class DeltaCoder
{
public:
  template <typename TWriter>
  static bool Encode(BitWriter<TWriter> & writer, uint64_t value)
  {
    if (value == 0)
      return false;

    uint8_t const n = bits::FloorLog(value);
    ASSERT_LESS_OR_EQUAL(n, 63, ());
    if (!GammaCoder::Encode(writer, n + 1))
      return false;

    writer.WriteAtMost64Bits(value, n);
    return true;
  }

  template <typename TReader>
  static uint64_t Decode(BitReader<TReader> & reader)
  {
    uint8_t n = GammaCoder::Decode(reader);

    ASSERT_GREATER(n, 0, ());
    --n;

    ASSERT_LESS_OR_EQUAL(n, 63, ());

    uint64_t const msb = static_cast<uint64_t>(1) << n;
    return msb | reader.ReadAtMost64Bits(n);
  }
};
}  // namespace coding
