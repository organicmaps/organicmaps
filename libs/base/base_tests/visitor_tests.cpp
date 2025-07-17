#include "testing/testing.hpp"

#include "base/visitor.hpp"

#include <string>

using namespace std;

namespace
{
struct Foo
{
  DECLARE_VISITOR()
  DECLARE_DEBUG_PRINT(Foo)
};

struct Bar
{
  Bar() = default;
  Bar(int d, string const & s) : m_d(d), m_s(s) {}

  DECLARE_VISITOR(visitor(m_d), visitor(m_s, "string"))
  DECLARE_DEBUG_PRINT(Bar)

  int m_d = 0;
  string m_s;
};

UNIT_TEST(DebugPrintVisitor_Smoke)
{
  {
    Foo foo;
    TEST_EQUAL(DebugPrint(foo), "Foo []", ());
  }

  {
    Bar bar;
    TEST_EQUAL(DebugPrint(bar), "Bar [0, string: ]", ());

    bar.m_d = 7;
    bar.m_s = "Hello, World!";
    TEST_EQUAL(DebugPrint(bar), "Bar [7, string: Hello, World!]", ());
  }
}
}  // namespace
