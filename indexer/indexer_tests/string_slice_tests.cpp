#include "../../testing/testing.hpp"

#include "indexer/string_slice.hpp"

#include "base/string_utils.hpp"

#include <iterator>
#include <vector>

using namespace search;
using namespace std;
using namespace strings;

namespace
{
UniString ToString(vector<UniString> const & v)
{
  StringSlice slice(v);
  UniString r;
  copy(JoinIterator::Begin(slice), JoinIterator::End(slice), back_inserter(r));
  return r;
}

UNIT_TEST(JoinIterator_Smoke)
{
  {
    vector<UniString> v;

    StringSlice slice1(v);
    auto begin1 = JoinIterator::Begin(slice1);
    auto end1 = JoinIterator::End(slice1);

    StringSlice slice2(v);
    auto begin2 = JoinIterator::Begin(slice2);
    auto end2 = JoinIterator::End(slice2);

    TEST(begin1 == end1, ());

    TEST(begin2 == end2, ());

    TEST(begin1 != begin2, ());
    TEST(end1 != end2, ());

    TEST(begin1 != end2, ());
    TEST(begin2 != end1, ());
  }

  {
    vector<UniString> const v;
    TEST_EQUAL(MakeUniString(""), ToString(v), ());
  }

  {
    vector<UniString> const v = {MakeUniString("")};
    TEST_EQUAL(MakeUniString(""), ToString(v), ());
  }

  {
    vector<UniString> const v = {MakeUniString(""), MakeUniString("")};
    TEST_EQUAL(MakeUniString(" "), ToString(v), ());
  }

  {
    vector<UniString> const v = {MakeUniString(""), MakeUniString("b"), MakeUniString("")};
    TEST_EQUAL(MakeUniString(" b "), ToString(v), ());
  }

  {
    vector<UniString> const v = {MakeUniString("Hello")};
    TEST_EQUAL(MakeUniString("Hello"), ToString(v), ());
  }

  {
    vector<UniString> const v = {MakeUniString("Hello"), MakeUniString("World!")};
    TEST_EQUAL(MakeUniString("Hello World!"), ToString(v), ());
  }
}
}  // namespace
