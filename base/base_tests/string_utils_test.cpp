#include "../../testing/testing.hpp"
#include "../string_utils.hpp"

#include "../../std/bind.hpp"

UNIT_TEST(make_lower_case)
{
  string s;

  s = "THIS_IS_UPPER";
  strings::make_lower_case(s);
  TEST_EQUAL(s, "this_is_upper", ());

  s = "THIS_iS_MiXed";
  strings::make_lower_case(s);
  TEST_EQUAL(s, "this_is_mixed", ());

  s = "this_is_lower";
  strings::make_lower_case(s);
  TEST_EQUAL(s, "this_is_lower", ());
}

UNIT_TEST(to_double)
{
  string s;
  double d;

  s = "0.123";
  TEST(strings::to_double(s, d), ());
  TEST_ALMOST_EQUAL(0.123, d, ());

  s = "1.";
  TEST(strings::to_double(s, d), ());
  TEST_ALMOST_EQUAL(1.0, d, ());

  s = "0";
  TEST(strings::to_double(s, d), ());
  TEST_ALMOST_EQUAL(0., d, ());

  s = "5.6843418860808e-14";
  TEST(strings::to_double(s, d), ());
  TEST_ALMOST_EQUAL(5.6843418860808e-14, d, ());

  s = "-2";
  TEST(strings::to_double(s, d), ());
  TEST_ALMOST_EQUAL(-2.0, d, ());
}

UNIT_TEST(to_string)
{
  TEST_EQUAL(strings::to_string(-1), "-1", ());
  TEST_EQUAL(strings::to_string(1234567890), "1234567890", ());
  TEST_EQUAL(strings::to_string(0.56), "0.56", ());
  TEST_EQUAL(strings::to_string(-100.2), "-100.2", ());
}

struct FunctorTester
{
  size_t & m_index;
  vector<string> const & m_tokens;

  explicit FunctorTester(size_t & counter, vector<string> const & tokens)
    : m_index(counter), m_tokens(tokens) {}
  void operator()(string const & s)
  {
    TEST_EQUAL(s, m_tokens[m_index++], ());
  }
};

void TestIter(string const & str, char const * delims, vector<string> const & tokens)
{
  strings::SimpleTokenizer it(str, delims);
  for (size_t i = 0; i < tokens.size(); ++i)
  {
    TEST_EQUAL(true, it, (str, delims, i));
    TEST_EQUAL(i == tokens.size() - 1, it.IsLast(), ());
    TEST_EQUAL(*it, tokens[i], (str, delims, i));
    ++it;
  }
  TEST_EQUAL(false, it, (str, delims));

  size_t counter = 0;
  FunctorTester f = FunctorTester(counter, tokens);
  strings::Tokenize(str, delims, f);
  TEST_EQUAL(counter, tokens.size(), ());
}

UNIT_TEST(SimpleTokenizer)
{
  vector<string> tokens;
  TestIter("", "", tokens);
  TestIter("", "; ", tokens);
  TestIter("  : ;  , ;", "; :,", tokens);

  {
    char const * s[] = {"hello"};
    tokens.assign(&s[0], &s[0] + ARRAY_SIZE(s));
    TestIter("hello", ";", tokens);
  }

  {
    char const * s[] = {"hello", "world"};
    tokens.assign(&s[0], &s[0] + ARRAY_SIZE(s));
    TestIter(" hello, world!", ", !", tokens);
  }

  {
    char const * s[] = {"\xD9\x80", "\xD8\xA7\xD9\x84\xD9\x85\xD9\x88\xD8\xA7\xD9\x81\xD9\x82",
                       "\xD8\xAC"};
    tokens.assign(&s[0], &s[0] + ARRAY_SIZE(s));
    TestIter("\xD9\x87\xD9\x80 - \xD8\xA7\xD9\x84\xD9\x85\xD9\x88\xD8\xA7\xD9\x81\xD9\x82 \xD9\x87\xD8\xAC",
             " -\xD9\x87", tokens);
  }

}

UNIT_TEST(LastUniChar)
{
  TEST_EQUAL(strings::LastUniChar(""), 0, ());
  TEST_EQUAL(strings::LastUniChar("Hello"), 0x6f, ());
  TEST_EQUAL(strings::LastUniChar(" \xD0\x90"), 0x0410, ());

}
