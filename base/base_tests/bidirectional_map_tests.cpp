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

  {
    string value;
    TEST(m.GetValue(1, value), ());
    TEST_EQUAL(value, "a", ());
  }
  {
    int key;
    TEST(m.GetKey("a", key), ());
    TEST_EQUAL(key, 1, ());
  }

  TEST(!m.Add(1, "b"), ());
  TEST(!m.Add(2, "a"), ());
  TEST(m.Add(2, "b"), ());
  TEST(!m.Add(2, "b"), ());
}
