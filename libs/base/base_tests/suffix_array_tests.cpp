#include "testing/testing.hpp"

#include "base/suffix_array.hpp"

#include <cstring>
#include <string>
#include <vector>

using namespace base;
using namespace std;

#define TEST_STR_EQUAL(X, Y, msg) TEST_EQUAL(string(X), string(Y), msg)

namespace
{
UNIT_TEST(Skew_Smoke)
{
  Skew(0, nullptr /* s */, nullptr /* sa */);
}

UNIT_TEST(Skew_Simple)
{
  {
    string const s;
    vector<size_t> pos;
    Skew(s, pos);
    TEST_EQUAL(pos.size(), s.size(), ());
  }

  {
    string const s = "a";
    vector<size_t> pos;
    Skew(s, pos);
    TEST_EQUAL(pos.size(), s.size(), ());
    TEST_EQUAL(pos[0], 0, ());
  }

  {
    string const s = "aaaa";
    vector<size_t> pos;
    Skew(s, pos);
    TEST_EQUAL(pos.size(), s.size(), ());
    TEST_EQUAL(pos[0], 3, ());
    TEST_EQUAL(pos[1], 2, ());
    TEST_EQUAL(pos[2], 1, ());
    TEST_EQUAL(pos[3], 0, ());
  }

  for (size_t length = 0; length < 100; ++length)
  {
    string const s(length, 'a');
    vector<size_t> pos;
    Skew(s, pos);
    TEST_EQUAL(pos.size(), s.size(), ());
    for (size_t i = 0; i < pos.size(); ++i)
      TEST_EQUAL(pos[i], pos.size() - i - 1, ());
  }

  for (size_t length = 0; length < 100; ++length)
  {
    string const s(length, '\0');
    vector<size_t> pos;
    Skew(s, pos);
    TEST_EQUAL(pos.size(), s.size(), ());
    for (size_t i = 0; i < pos.size(); ++i)
      TEST_EQUAL(pos[i], pos.size() - i - 1, ());
  }
}

UNIT_TEST(Skew_Classic)
{
  char const * s = "mississippi";
  size_t const n = strlen(s);
  TEST_EQUAL(n, 11, ());

  vector<size_t> pos(n);
  Skew(n, reinterpret_cast<uint8_t const *>(s), pos.data());

  TEST_STR_EQUAL("i", s + pos[0], ());
  TEST_STR_EQUAL("ippi", s + pos[1], ());
  TEST_STR_EQUAL("issippi", s + pos[2], ());
  TEST_STR_EQUAL("ississippi", s + pos[3], ());
  TEST_STR_EQUAL("mississippi", s + pos[4], ());
  TEST_STR_EQUAL("pi", s + pos[5], ());
  TEST_STR_EQUAL("ppi", s + pos[6], ());
  TEST_STR_EQUAL("sippi", s + pos[7], ());
  TEST_STR_EQUAL("sissippi", s + pos[8], ());
  TEST_STR_EQUAL("ssippi", s + pos[9], ());
  TEST_STR_EQUAL("ssissippi", s + pos[10], ());
}
}  // namespace
