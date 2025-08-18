#include "testing/testing.hpp"

#include "coding/reader.hpp"
#include "coding/value_opt_string.hpp"
#include "coding/writer.hpp"

#include <algorithm>
#include <string>
#include <vector>

namespace
{

template <class T>
void TestStringCodingT(T const * arr, size_t count, size_t maxSize)
{
  for (size_t i = 0; i < count; ++i)
  {
    auto const ethalon = strings::to_string(arr[i]);

    StringNumericOptimal s;
    s.Set(ethalon);

    std::vector<char> buffer;
    MemWriter<std::vector<char>> w(buffer);

    s.Write(w);

    size_t const sz = buffer.size();
    TEST_GREATER(sz, 0, ());
    TEST_LESS_OR_EQUAL(sz, maxSize, ());

    MemReader r(&buffer[0], sz);
    ReaderSource<MemReader> src(r);
    s.Read(src);

    TEST_EQUAL(ethalon, s.Get(), ());
  }
}

}  // namespace

UNIT_TEST(StringNumericOptimal_Zero)
{
  int t1 = 0;
  TestStringCodingT(&t1, 1, 1);  // should be coded as VarUint

  std::string t2 = "01";
  TestStringCodingT(&t2, 1, 3);  // should be coded as String
}

UNIT_TEST(StringNumericOptimal_IntCoding1)
{
  int arr[] = {0, 1, 2, 666, 0x0FFFFFFF, 0x7FFFFFFF - 1, 0x7FFFFFFF};
  TestStringCodingT(arr, ARRAY_SIZE(arr), 5);  // should be coded as VarUint
}

UNIT_TEST(StringNumericOptimal_IntCoding2)
{
  int arr[] = {-1, -2, -666666, static_cast<int>(0xFFFFFFFE), static_cast<int>(0xFFFFFFFF)};
  TestStringCodingT(arr, ARRAY_SIZE(arr), 12);  // should be coded as String
}

UNIT_TEST(StringNumericOptimal_StringCoding)
{
  char const * arr[] = {"xxx", "yyy", "a", "0xFFFFFF", "123456UL"};
  TestStringCodingT(arr, ARRAY_SIZE(arr), 12);  // should be coded as String
}

UNIT_TEST(StringNumericOptimal_LargeStringCoding)
{
  std::string s;
  fill_n(back_inserter(s), 10000, 'x');

  TestStringCodingT(&s, 1, 10006);
}
