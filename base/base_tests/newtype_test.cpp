#include "testing/testing.hpp"

#include "base/newtype.hpp"

#include <sstream>
#include <string>
#include <type_traits>

namespace newtype_test
{
NEWTYPE(int, Int);

std::string DebugPrint(Int const & i)
{
  std::stringstream sstr;
  sstr << "Int(" << i.Get() << ')';
  return sstr.str();
}

UNIT_TEST(NewType_TypeChecks)
{
  TEST((std::is_constructible<Int, int>::value), ());
  TEST((std::is_constructible<Int, char>::value), ());
  TEST(!(std::is_convertible<int, Int>::value), ());
  TEST(!(std::is_convertible<Int, int>::value), ());
}

UNIT_TEST(NewType_Base)
{
  Int a{10};
  TEST_EQUAL(a.Get(), 10, ());

  a.Set(100);
  TEST_EQUAL(a.Get(), 100, ());
}

UNIT_TEST(NewType_Operations)
{
  TEST(Int(10) == Int(10), ());
  TEST(Int(20) != Int(30), ());
  TEST(Int(10) < Int(20), ());
  TEST(Int(10) <= Int(20), ());
  TEST(Int(20) > Int(10), ());
  TEST(Int(20) >= Int(10), ());

  TEST_EQUAL(Int(10) + Int(20), Int(30), ());
  TEST_EQUAL(Int(10) - Int(20), Int(-10), ());
  TEST_EQUAL(Int(10) / Int(2), Int(5), ());
  TEST_EQUAL(Int(10) * Int(2), Int(20), ());
  TEST_EQUAL(Int(10) % Int(3), Int(1), ());

  TEST_EQUAL(Int(10) | Int(7), Int(10 | 7), ());
  TEST_EQUAL(Int(10) & Int(7), Int(10 & 7), ());
  TEST_EQUAL(Int(10) ^ Int(7), Int(10 ^ 7), ());
}

UNIT_TEST(NewTypeMember_Operations)
{
  Int a(10);
  auto b = a--;
  TEST_EQUAL(a, Int(9), ());
  TEST_EQUAL(b, Int(10), ());

  b = --a;
  TEST_EQUAL(a, b, ());
  TEST_EQUAL(a, Int(8), ());

  b = ++a;
  TEST_EQUAL(a, b, ());
  TEST_EQUAL(a, Int(9), ());

  b = a++;
  TEST_EQUAL(a, Int(10), ());
  TEST_EQUAL(b, Int(9), ());

  a.Set(100);
  b = Int(2);
  a *= b;
  TEST_EQUAL(a, Int(200), ());

  a /= b;
  TEST_EQUAL(a, Int(100), ());

  b.Set(3);
  a %= b;
  TEST_EQUAL(a, Int(1), ());

  a.Set(10);
  a |= Int(7);
  TEST_EQUAL(a, Int(10 | 7), ());

  a.Set(10);
  a &= Int(7);
  TEST_EQUAL(a, Int(10 & 7), ());

  a.Set(10);
  a ^= Int(7);
  TEST_EQUAL(a, Int(10 ^ 7), ());
}

namespace test_output
{
NEWTYPE_SIMPLE_OUTPUT(Int);
}  // namespace test_output

UNIT_TEST(NewType_SimpleOutPut)
{
  using namespace test_output;
  TEST_EQUAL(test_output::DebugPrint(Int(10)), "10", ());

  std::ostringstream sstr;
  sstr << Int(20);
  TEST_EQUAL(sstr.str(), "20", ());
}
}  // namespace newtype_test
