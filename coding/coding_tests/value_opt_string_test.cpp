#include "testing/testing.hpp"

#include "coding/value_opt_string.hpp"

#include "coding/reader.hpp"
#include "coding/writer.hpp"


namespace
{
  template <class T>
  void TestStringCodingT(T const * arr, size_t count, size_t maxSize)
  {
    for (size_t i = 0; i < count; ++i)
    {
      StringNumericOptimal s;
      s.Set(arr[i]);

      vector<char> buffer;
      MemWriter<vector<char> > w(buffer);

      s.Write(w);

      size_t const sz = buffer.size();
      TEST_GREATER(sz, 0, ());
      TEST_LESS_OR_EQUAL(sz, maxSize, ());

      MemReader r(&buffer[0], sz);
      ReaderSource<MemReader> src(r);
      s.Read(src);

      TEST_EQUAL(strings::to_string(arr[i]), s.Get(), ());
    }
  }
}

UNIT_TEST(StringNumericOptimal_IntCoding1)
{
  int arr[] = { 0, 1, 2, 666, 0x0FFFFFFF, 0x7FFFFFFF-1, 0x7FFFFFFF };
  TestStringCodingT(arr, ARRAY_SIZE(arr), 5);  // should be coded as VarUint
}

UNIT_TEST(StringNumericOptimal_IntCoding2)
{
  int arr[] = { -1, -2, -666666, static_cast<int>(0xFFFFFFFE), static_cast<int>(0xFFFFFFFF) };
  TestStringCodingT(arr, ARRAY_SIZE(arr), 12);  // should be coded as String
}

UNIT_TEST(StringNumericOptimal_StringCoding)
{
  char const * arr[] = { "xxx", "yyy", "a", "0xFFFFFF", "123456UL" };
  TestStringCodingT(arr, ARRAY_SIZE(arr), 12);  // should be coded as String
}

UNIT_TEST(StringNumericOptimal_LargeStringCoding)
{
  string s;
  std::fill_n(back_inserter(s), 10000, 'x');

  TestStringCodingT(&s, 1, 10006);
}
