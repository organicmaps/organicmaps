#include "testing/testing.hpp"

#include "base/regexp.hpp"

#include "base/stl_add.hpp"


UNIT_TEST(RegExp_Or)
{
  regexp::RegExpT exp;
  regexp::Create("\\.mwm\\.(downloading2?$|resume2?$)", exp);

  TEST(regexp::IsExist("Aruba.mwm.downloading", exp), ());
  TEST(!regexp::IsExist("Aruba.mwm.downloading1", exp), ());
  TEST(regexp::IsExist("Aruba.mwm.downloading2", exp), ());
  TEST(!regexp::IsExist("Aruba.mwm.downloading3", exp), ());
  TEST(!regexp::IsExist("Aruba.mwm.downloading.tmp", exp), ());

  TEST(regexp::IsExist("Aruba.mwm.resume", exp), ());
  TEST(!regexp::IsExist("Aruba.mwm.resume1", exp), ());
  TEST(regexp::IsExist("Aruba.mwm.resume2", exp), ());
  TEST(!regexp::IsExist("Aruba.mwm.resume3", exp), ());
  TEST(!regexp::IsExist("Aruba.mwm.resume.tmp", exp), ());
}

UNIT_TEST(RegExp_ForEachMatched)
{
  regexp::RegExpT exp;
  regexp::Create("-?\\d+\\.?\\d*, *-?\\d+\\.?\\d*", exp);

  {
    string const s = "6.66, 9.99";
    vector<string> v;
    regexp::ForEachMatched(s, exp, MakeBackInsertFunctor(v));
    TEST_EQUAL(v.size(), 1, ());
    TEST_EQUAL(v[0], s, ());
  }

  {
    string const s1 = "6.66, 9.99";
    string const s2 = "-5.55, -7.77";
    vector<string> v;
    regexp::ForEachMatched(s1 + " 180 , bfuewib 365@" + s2, exp, MakeBackInsertFunctor(v));
    TEST_EQUAL(v.size(), 2, ());
    TEST_EQUAL(v[0], s1, ());
    TEST_EQUAL(v[1], s2, ());
  }

  {
    string const s = "X6.66, 9.99";
    vector<string> v;
    regexp::ForEachMatched(s, exp, MakeBackInsertFunctor(v));
    TEST_EQUAL(v.size(), 1, ());
    TEST_EQUAL(v[0], string(s.begin() + 1, s.end()), ());
  }

  {
    string const s = "6.66, 9.99X";
    vector<string> v;
    regexp::ForEachMatched(s, exp, MakeBackInsertFunctor(v));
    TEST_EQUAL(v.size(), 1, ());
    TEST_EQUAL(v[0], string(s.begin(), s.end() - 1), ());
  }

  {
    string const s = "6.66X, 9.99";
    vector<string> v;
    regexp::ForEachMatched(s, exp, MakeBackInsertFunctor(v));
    TEST_EQUAL(v.size(), 0, ());
  }

  {
    string const s = "6.66, X9.99";
    vector<string> v;
    regexp::ForEachMatched(s, exp, MakeBackInsertFunctor(v));
    TEST_EQUAL(v.size(), 0, ());
  }
}
