#include "testing/testing.hpp"

#include "base/newtype.hpp"

#include "std/sstream.hpp"
#include "std/type_traits.hpp"

namespace
{
NEWTYPE(int, Int);

string DebugPrint(Int const & i)
{
  stringstream sstr;
  sstr << "Int(" << i.Get() << ')';
  return sstr.str();
}

UNIT_TEST(NewType_TypeChecks)
{
  TEST((is_constructible<Int, int>::value), ());
  TEST((is_constructible<Int, char>::value), ());
  TEST(!(is_convertible<int, Int>::value), ());
  TEST(!(is_convertible<Int, int>::value), ());
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
}  // namespace
