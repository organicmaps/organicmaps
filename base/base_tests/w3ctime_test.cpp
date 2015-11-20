#include "base/w3ctime.hpp"
#include "testing/testing.hpp"

using namespace w3ctime;

UNIT_TEST(ParseTime)
{
  TEST_EQUAL(ParseTime(""), kNotATime, ());
  TEST_EQUAL(ParseTime("2015-10-11 23:21"), kNotATime, ());
  TEST_EQUAL(ParseTime("2015-10-11T23:21Z"), 1444605660, ());
}

UNIT_TEST(TimeToString)
{
  TEST_EQUAL(TimeToString(0), "1970-01-01T00:00Z", ());
  TEST_EQUAL(TimeToString(1444605660), "2015-10-11T23:21Z", ());
}
