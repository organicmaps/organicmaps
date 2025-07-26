#include "testing/testing.hpp"

#include "coding/hex.hpp"

#include <cstddef>
#include <cstdint>
#include <random>
#include <string>

using namespace std;

UNIT_TEST(GoldenRecode)
{
  string data("\x01\x23\x45\x67\x89\xAB\xCD\xEF");
  string hexData("0123456789ABCDEF");

  TEST_EQUAL(ToHex(data), hexData, ());
  TEST_EQUAL(data, FromHex(hexData), ());
}

UNIT_TEST(RandomRecode)
{
  mt19937 rng(0);
  for (size_t i = 0; i < 256; ++i)
  {
    string data(1 + (rng() % 20), 0);
    for (size_t j = 0; j < data.size(); ++j)
      data[j] = static_cast<char>(rng() % 26) + 'A';
    TEST_EQUAL(data, FromHex(ToHex(data)), ());
  }
}

UNIT_TEST(EncodeNumber)
{
  TEST_EQUAL(NumToHex(uint64_t(0x0123456789ABCDEFULL)), "0123456789ABCDEF", ());
}

UNIT_TEST(DecodeLowerCaseHex)
{
  TEST_EQUAL(FromHex("fe"), "\xfe", ());
}

UNIT_TEST(EncodeEmptyString)
{
  TEST_EQUAL(ToHex(string()), "", ());
}

UNIT_TEST(DecodeEmptyString)
{
  TEST_EQUAL(FromHex(""), "", ());
}
