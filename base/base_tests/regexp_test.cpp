#include "testing/testing.hpp"

#include "base/stl_add.hpp"
#include "base/string_utils.hpp"

#include "std/regex.hpp"

UNIT_TEST(RegExp_Or)
{
  regex exp("\\.mwm\\.(downloading2?$|resume2?$)");

  TEST(regex_search("Aruba.mwm.downloading", exp), ());
  TEST(!regex_search("Aruba.mwm.downloading1", exp), ());
  TEST(regex_search("Aruba.mwm.downloading2", exp), ());
  TEST(!regex_search("Aruba.mwm.downloading3", exp), ());
  TEST(!regex_search("Aruba.mwm.downloading.tmp", exp), ());

  TEST(regex_search("Aruba.mwm.resume", exp), ());
  TEST(!regex_search("Aruba.mwm.resume1", exp), ());
  TEST(regex_search("Aruba.mwm.resume2", exp), ());
  TEST(!regex_search("Aruba.mwm.resume3", exp), ());
  TEST(!regex_search("Aruba.mwm.resume.tmp", exp), ());
}

UNIT_TEST(RegExp_ForEachMatched)
{
  regex exp("-?\\d+\\.?\\d*, *-?\\d+\\.?\\d*");

  {
    string const s = "6.66, 9.99";
    std::vector<string> v;
    strings::ForEachMatched(s, exp, MakeBackInsertFunctor(v));
    TEST_EQUAL(v.size(), 1, ());
    TEST_EQUAL(v[0], s, ());
  }

  {
    string const s1 = "6.66, 9.99";
    string const s2 = "-5.55, -7.77";
    std::vector<string> v;
    strings::ForEachMatched(s1 + " 180 , bfuewib 365@" + s2, exp, MakeBackInsertFunctor(v));
    TEST_EQUAL(v.size(), 2, ());
    TEST_EQUAL(v[0], s1, ());
    TEST_EQUAL(v[1], s2, ());
  }

  {
    string const s = "X6.66, 9.99";
    std::vector<string> v;
    strings::ForEachMatched(s, exp, MakeBackInsertFunctor(v));
    TEST_EQUAL(v.size(), 1, ());
    TEST_EQUAL(v[0], string(s.begin() + 1, s.end()), ());
  }

  {
    string const s = "6.66, 9.99X";
    std::vector<string> v;
    strings::ForEachMatched(s, exp, MakeBackInsertFunctor(v));
    TEST_EQUAL(v.size(), 1, ());
    TEST_EQUAL(v[0], string(s.begin(), s.end() - 1), ());
  }

  {
    string const s = "6.66X, 9.99";
    std::vector<string> v;
    strings::ForEachMatched(s, exp, MakeBackInsertFunctor(v));
    TEST_EQUAL(v.size(), 0, ());
  }

  {
    string const s = "6.66, X9.99";
    std::vector<string> v;
    strings::ForEachMatched(s, exp, MakeBackInsertFunctor(v));
    TEST_EQUAL(v.size(), 0, ());
  }
}
