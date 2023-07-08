#include "testing/testing.hpp"

#include "coding/succinct_mapper.hpp"
#include "coding/writer.hpp"

#include <cstdint>
#include <vector>

#include "3party/succinct/mapper.hpp"

using namespace coding;

UNIT_TEST(ReverseMapper_Smoke)
{
  uint64_t data = 0x0123456789abcdef;
  uint64_t rdata = 0x0;
  TEST_EQUAL(8, ReverseMap(rdata, reinterpret_cast<uint8_t *>(&data), "uint64_t"), ());

  // Test that reversed uint64_t was read.
  TEST_EQUAL(0xefcdab8967452301, rdata, ());

  // Test that underlying buffer was modified.
  TEST_EQUAL(0xefcdab8967452301, data, ());
}

UNIT_TEST(Freeze_Smoke)
{
  std::vector<uint8_t> data;
  {
    MemWriter<decltype(data)> writer(data);
    uint64_t const data = 0x0123456789abcdef;
    Freeze(data, writer, "uint64_t");
  }
  TEST_EQUAL(8, data.size(), ());

  uint64_t value = 0x0;
  TEST_EQUAL(8, Map(value, reinterpret_cast<uint8_t const *>(data.data()), "uint64_t"), ());
  TEST_EQUAL(0x0123456789abcdef, value, ());
}

UNIT_TEST(ReverseFreeze_Smoke)
{
  std::vector<uint8_t> data;
  {
    MemWriter<decltype(data)> writer(data);
    uint64_t const data = 0x0123456789abcdef;
    ReverseFreeze(data, writer, "uint64_t");
  }

  TEST_EQUAL(8, data.size(), ());

  uint64_t value = 0x0;
  TEST_EQUAL(8, Map(value, reinterpret_cast<uint8_t const *>(data.data()), "uint64_t"), ());
  TEST_EQUAL(0xefcdab8967452301, value, ());
}
