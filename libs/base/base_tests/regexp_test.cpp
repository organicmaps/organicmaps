#include "testing/testing.hpp"

#include "base/stl_helpers.hpp"

#include <regex>

namespace regexp_test
{
template <typename Fn>
void ForEachMatched(std::string const & s, std::regex const & regex, Fn && fn)
{
  for (std::sregex_token_iterator cur(s.begin(), s.end(), regex), end; cur != end; ++cur)
    fn(*cur);
}

UNIT_TEST(RegExp_Or)
{
  std::regex exp("\\.mwm\\.(downloading2?$|resume2?$)");

  TEST(std::regex_search("Aruba.mwm.downloading", exp), ());
  TEST(!regex_search("Aruba.mwm.downloading1", exp), ());
  TEST(std::regex_search("Aruba.mwm.downloading2", exp), ());
  TEST(!regex_search("Aruba.mwm.downloading3", exp), ());
  TEST(!regex_search("Aruba.mwm.downloading.tmp", exp), ());

  TEST(std::regex_search("Aruba.mwm.resume", exp), ());
  TEST(!regex_search("Aruba.mwm.resume1", exp), ());
  TEST(std::regex_search("Aruba.mwm.resume2", exp), ());
  TEST(!regex_search("Aruba.mwm.resume3", exp), ());
  TEST(!regex_search("Aruba.mwm.resume.tmp", exp), ());
}

UNIT_TEST(RegExp_ForEachMatched)
{
  std::regex exp("-?\\d+\\.?\\d*, *-?\\d+\\.?\\d*");

  {
    std::string const s = "6.66, 9.99";
    std::vector<std::string> v;
    ForEachMatched(s, exp, base::MakeBackInsertFunctor(v));
    TEST_EQUAL(v.size(), 1, ());
    TEST_EQUAL(v[0], s, ());
  }

  {
    std::string const s1 = "6.66, 9.99";
    std::string const s2 = "-5.55, -7.77";
    std::vector<std::string> v;
    ForEachMatched(s1 + " 180 , bfuewib 365@" + s2, exp, base::MakeBackInsertFunctor(v));
    TEST_EQUAL(v.size(), 2, ());
    TEST_EQUAL(v[0], s1, ());
    TEST_EQUAL(v[1], s2, ());
  }

  {
    std::string const s = "X6.66, 9.99";
    std::vector<std::string> v;
    ForEachMatched(s, exp, base::MakeBackInsertFunctor(v));
    TEST_EQUAL(v.size(), 1, ());
    TEST_EQUAL(v[0], std::string(s.begin() + 1, s.end()), ());
  }

  {
    std::string const s = "6.66, 9.99X";
    std::vector<std::string> v;
    ForEachMatched(s, exp, base::MakeBackInsertFunctor(v));
    TEST_EQUAL(v.size(), 1, ());
    TEST_EQUAL(v[0], std::string(s.begin(), s.end() - 1), ());
  }

  {
    std::string const s = "6.66X, 9.99";
    std::vector<std::string> v;
    ForEachMatched(s, exp, base::MakeBackInsertFunctor(v));
    TEST_EQUAL(v.size(), 0, ());
  }

  {
    std::string const s = "6.66, X9.99";
    std::vector<std::string> v;
    ForEachMatched(s, exp, base::MakeBackInsertFunctor(v));
    TEST_EQUAL(v.size(), 0, ());
  }
}

}  // namespace regexp_test
