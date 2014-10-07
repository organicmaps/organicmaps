#include "../../testing/testing.hpp"

#include "../hex.hpp"

#include "../../base/pseudo_random.hpp"

#include "../../std/string.hpp"


UNIT_TEST(GoldenRecode)
{
  string data("\x01\x23\x45\x67\x89\xAB\xCD\xEF");
  string hexData("0123456789ABCDEF");

  TEST_EQUAL(ToHex(data), hexData, ());
  TEST_EQUAL(data, FromHex(hexData), ());
}

UNIT_TEST(RandomRecode)
{
  for (size_t i = 0; i < 256; ++i)
  {
    string const data = rnd::GenerateString();
    TEST_EQUAL(data, FromHex(ToHex(data)), ());
  }
}

UNIT_TEST(EncodeNumber)
{
  TEST_EQUAL(NumToHex(uint64_t(0x0123456789ABCDEFULL)),
                                "0123456789ABCDEF", ());
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
