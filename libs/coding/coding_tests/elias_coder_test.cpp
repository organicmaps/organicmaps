#include "testing/testing.hpp"

#include "coding/bit_streams.hpp"
#include "coding/elias_coder.hpp"
#include "coding/reader.hpp"
#include "coding/writer.hpp"

#include "base/bits.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace
{
template <typename TCoder>
void TestCoder(std::string const & name)
{
  using TBuffer = std::vector<uint8_t>;
  using TWriter = MemWriter<TBuffer>;

  uint64_t const kMask = 0xfedcba9876543210;

  TBuffer buf;
  {
    TWriter w(buf);
    BitWriter<TWriter> bits(w);
    for (int i = 0; i <= 64; ++i)
    {
      uint64_t const mask = bits::GetFullMask(i);
      uint64_t const value = kMask & mask;
      if (value == 0)
        TEST(!TCoder::Encode(bits, value), (name, i));
      else
        TEST(TCoder::Encode(bits, value), (name, i));
    }
  }

  {
    MemReader r(buf.data(), buf.size());
    ReaderSource<MemReader> src(r);
    BitReader<ReaderSource<MemReader>> bits(src);
    for (int i = 0; i <= 64; ++i)
    {
      uint64_t const mask = bits::GetFullMask(i);
      uint64_t const expected = kMask & mask;
      if (expected == 0)
        continue;
      TEST_EQUAL(expected, TCoder::Decode(bits), (name, i));
    }
  }
}

UNIT_TEST(EliasCoder_Gamma)
{
  TestCoder<coding::GammaCoder>("Gamma");
}
UNIT_TEST(EliasCoder_Delta)
{
  TestCoder<coding::DeltaCoder>("Delta");
}
}  // namespace
