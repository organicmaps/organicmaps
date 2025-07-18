#include "testing/testing.hpp"

#include "base/bidirectional_map.hpp"
#include "base/macros.hpp"

#include <map>
#include <string>
#include <unordered_map>

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

UNIT_TEST(BidirectionalMap_Remove)
{
  {
    BidirectionalMap<int, string> m;
    TEST(m.Add(1, "a"), ());
    TEST(m.Add(2, "b"), ());

    TEST_EQUAL(m.Size(), 2, ());

    TEST(!m.RemoveKey(3), ());
    TEST_EQUAL(m.Size(), 2, ());

    TEST(m.RemoveKey(1), ());
    TEST_EQUAL(m.Size(), 1, ());

    TEST(!m.RemoveValue("a"), ());
    TEST_EQUAL(m.Size(), 1, ());

    TEST(m.RemoveValue("b"), ());
    TEST(m.IsEmpty(), ());
  }

  {
    BidirectionalMap<int, int> m;
    TEST(m.Add(1, 1), ());
    TEST(m.Add(2, 2), ());

    TEST_EQUAL(m.Size(), 2, ());

    TEST(!m.RemoveKey(3), ());
    TEST_EQUAL(m.Size(), 2, ());

    TEST(m.RemoveKey(1), ());
    TEST_EQUAL(m.Size(), 1, ());

    TEST(!m.RemoveValue(1), ());
    TEST_EQUAL(m.Size(), 1, ());

    TEST(m.RemoveValue(2), ());
    TEST(m.IsEmpty(), ());
  }

  {
    BidirectionalMap<int, int> m;
    TEST(m.Add(1, 2), ());
    TEST(m.Add(2, 1), ());

    TEST_EQUAL(m.Size(), 2, ());

    TEST(!m.RemoveKey(3), ());
    TEST_EQUAL(m.Size(), 2, ());

    TEST(m.RemoveKey(1), ());
    TEST_EQUAL(m.Size(), 1, ());

    TEST(!m.RemoveValue(2), ());
    TEST_EQUAL(m.Size(), 1, ());

    TEST(m.RemoveValue(1), ());
    TEST(m.IsEmpty(), ());
  }
}
