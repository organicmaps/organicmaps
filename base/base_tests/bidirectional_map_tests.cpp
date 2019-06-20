#include "testing/testing.hpp"

#include "base/bidirectional_map.hpp"

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
