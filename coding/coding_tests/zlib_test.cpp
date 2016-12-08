#include "testing/testing.hpp"

#include "coding/zlib.hpp"

#include "std/iterator.hpp"
#include "std/sstream.hpp"
#include "std/string.hpp"

using namespace coding;

namespace
{
void TestInflateDeflate(string const & original)
{
  string compressed;
  TEST(ZLib::Deflate(original, ZLib::Level::BestCompression, back_inserter(compressed)), ());

  string decompressed;
  TEST(ZLib::Inflate(compressed, back_inserter(decompressed)), ());

  TEST_EQUAL(original, decompressed, ());
}

UNIT_TEST(ZLib_Smoke)
{
  {
    string s;
    TEST(!ZLib::Deflate(nullptr, 0, ZLib::Level::BestCompression, back_inserter(s)), ());
    TEST(!ZLib::Deflate(nullptr, 4, ZLib::Level::BestCompression, back_inserter(s)), ());
    TEST(!ZLib::Inflate(nullptr, 0, back_inserter(s)), ());
    TEST(!ZLib::Inflate(nullptr, 4, back_inserter(s)), ());
  }

  TestInflateDeflate("");
  TestInflateDeflate("Hello, World!");
}

UNIT_TEST(ZLib_Large)
{
  string original;
  {
    ostringstream os;
    for (size_t i = 0; i < 1000; ++i)
      os << i;
    original = os.str();
  }

  TestInflateDeflate(original);
}
}  // namespace
