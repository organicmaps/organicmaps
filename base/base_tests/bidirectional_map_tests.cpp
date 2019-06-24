#include "testing/testing.hpp"

#include "base/bidirectional_map.hpp"
#include "base/macros.hpp"

#include <string>

using namespace base;
using namespace std;

UNIT_TEST(BidirectionalMap_Smoke)
{
  BidirectionalMap<int, string> m;
  m.Add(1, "a");
  TEST_EQUAL(m.MustGetValue(1), "a", ());
  TEST_EQUAL(m.MustGetKey("a"), 1, ());
}

UNIT_TEST(BidirectionalMap_Exceptions)
{
  // Tests that the exceptions are thrown as expected.
  // Note that it is discouraged to catch them when using Must* methods.
  BidirectionalMap<int, string> m;
  {
    bool caught = false;
    try
    {
      UNUSED_VALUE(m.MustGetValue(0));
    }
    catch (BidirectionalMap<int, string>::NoValueForKeyException const & e)
    {
      caught = true;
    }
    TEST(caught, ());
  }

  {
    bool caught = false;
    try
    {
      UNUSED_VALUE(m.MustGetKey(""));
    }
    catch (BidirectionalMap<int, string>::NoKeyForValueException const & e)
    {
      caught = true;
    }
    TEST(caught, ());
  }
}
