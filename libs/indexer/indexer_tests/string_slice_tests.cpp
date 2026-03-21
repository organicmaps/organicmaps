#include "../../testing/testing.hpp"

#include "indexer/string_slice.hpp"

#include "base/string_utils.hpp"

#include <iterator>
#include <vector>

namespace
{
strings::UniString ToString(std::vector<strings::UniString> const & v)
{
  search::StringSlice slice(v);
  strings::UniString r;
  std::copy(search::JoinIterator::Begin(slice), search::JoinIterator::End(slice), std::back_inserter(r));
  return r;
}

UNIT_TEST(JoinIterator_Smoke)
{
  {
    std::vector<strings::UniString> v;

    search::StringSlice slice1(v);
    auto begin1 = search::JoinIterator::Begin(slice1);
    auto end1 = search::JoinIterator::End(slice1);

    search::StringSlice slice2(v);
    auto begin2 = search::JoinIterator::Begin(slice2);
    auto end2 = search::JoinIterator::End(slice2);

    TEST(begin1 == end1, ());

    TEST(begin2 == end2, ());

    TEST(begin1 != begin2, ());
    TEST(end1 != end2, ());

    TEST(begin1 != end2, ());
    TEST(begin2 != end1, ());
  }

  {
    std::vector<strings::UniString> const v;
    TEST_EQUAL(strings::MakeUniString(""), ToString(v), ());
  }

  {
    std::vector<strings::UniString> const v = {strings::MakeUniString("")};
    TEST_EQUAL(strings::MakeUniString(""), ToString(v), ());
  }

  {
    std::vector<strings::UniString> const v = {strings::MakeUniString(""), strings::MakeUniString("")};
    TEST_EQUAL(strings::MakeUniString(" "), ToString(v), ());
  }

  {
    std::vector<strings::UniString> const v = {strings::MakeUniString(""), strings::MakeUniString("b"),
                                               strings::MakeUniString("")};
    TEST_EQUAL(strings::MakeUniString(" b "), ToString(v), ());
  }

  {
    std::vector<strings::UniString> const v = {strings::MakeUniString("Hello")};
    TEST_EQUAL(strings::MakeUniString("Hello"), ToString(v), ());
  }

  {
    std::vector<strings::UniString> const v = {strings::MakeUniString("Hello"), strings::MakeUniString("World!")};
    TEST_EQUAL(strings::MakeUniString("Hello World!"), ToString(v), ());
  }
}
}  // namespace
