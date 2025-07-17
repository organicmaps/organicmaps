#include "testing/testing.hpp"

#include "base/limited_priority_queue.hpp"

using namespace base;

UNIT_TEST(LPQueue_Smoke)
{
  limited_priority_queue<int> q(3);
  TEST(q.empty(), ());

  q.push(5);
  q.push(-1);
  q.push(3);
  TEST_EQUAL(q.top(), 5, ());

  q.push(2);
  TEST_EQUAL(q.top(), 3, ());
  TEST_EQUAL(q.size(), 3, ());

  q.push(0);
  q.push(0);
  q.push(0);

  TEST_EQUAL(q.top(), 0, ());
  q.pop();

  TEST_EQUAL(q.top(), 0, ());
  q.pop();

  TEST_EQUAL(q.top(), -1, ());
  q.pop();

  TEST(q.empty(), ());
}
