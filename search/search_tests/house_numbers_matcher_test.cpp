#include "testing/testing.hpp"

#include "search/v2/house_numbers_matcher.hpp"

#include "std/vector.hpp"

#include "base/string_utils.hpp"

using namespace strings;
using namespace search::v2;

namespace
{
void NormalizeHouseNumber(string const & s, vector<string> & ts)
{
  vector<strings::UniString> tokens;
  search::v2::NormalizeHouseNumber(strings::MakeUniString(s), tokens);
  for (auto const & token : tokens)
    ts.push_back(strings::ToUtf8(token));
}

bool HouseNumbersMatch(string const & houseNumber, string const & query)
{
  return search::v2::HouseNumbersMatch(strings::MakeUniString(houseNumber),
                                       strings::MakeUniString(query));
}

void CheckTokenizer(string const & utf8s, vector<string> const & expected)
{
  UniString utf32s = MakeUniString(utf8s);
  vector<HouseNumberTokenizer::Token> tokens;
  HouseNumberTokenizer::Tokenize(utf32s, tokens);

  vector<string> actual;
  for (auto const & token : tokens)
    actual.push_back(ToUtf8(token.m_token));
  TEST_EQUAL(actual, expected, ());
}

void CheckNormalizer(string const & utf8s, string const & expected)
{
  vector<string> tokens;
  NormalizeHouseNumber(utf8s, tokens);

  string actual;
  for (size_t i = 0; i < tokens.size(); ++i)
  {
    actual.append(tokens[i]);
    if (i + 1 != tokens.size())
      actual.push_back(' ');
  }
  TEST_EQUAL(actual, expected, ());
}
}  // namespace

UNIT_TEST(HouseNumberTokenizer_Smoke)
{
  CheckTokenizer("123Б", {"123", "Б"});
  CheckTokenizer("123/Б", {"123", "Б"});
  CheckTokenizer("123/34 корп. 4 стр1", {"123", "34", "корп", "4", "стр", "1"});
}

UNIT_TEST(HouseNumberNormalizer_Smoke)
{
  CheckNormalizer("123Б", "123б");
  CheckNormalizer("123/4 Литер А", "123 4 а");
  CheckNormalizer("123а корп. 2б", "123а 2б");
  CheckNormalizer("123к4", "123 4");
  CheckNormalizer("123к Корпус 2", "123к 2");
  CheckNormalizer("9 литер А корпус 2", "9 а 2");
  CheckNormalizer("39с79", "39 79");
  CheckNormalizer("9 литер аб1", "9 аб1");
}

UNIT_TEST(HouseNumbersMatcher_Smoke)
{
  TEST(HouseNumbersMatch("39с79", "39"), ());
  TEST(HouseNumbersMatch("39с79", "39 Строение 79"), ());
  TEST(HouseNumbersMatch("39с79", "39 к. 79"), ());
  TEST(HouseNumbersMatch("39 - 79", "39 строение 79"), ());
  TEST(HouseNumbersMatch("39/79", "39 строение 79"), ());
  TEST(HouseNumbersMatch("127а корпус 2", "127а"), ());
  TEST(HouseNumbersMatch("127а корпус 2", "127а кор. 2"), ());
  TEST(HouseNumbersMatch("1234abcdef", "1234  abcdef"), ());
  TEST(HouseNumbersMatch("10/42 корпус 2", "10"), ());

  TEST(!HouseNumbersMatch("39", "39 с 79"), ());
  TEST(!HouseNumbersMatch("127а корпус 2", "127"), ());
  TEST(!HouseNumbersMatch("6 корпус 2", "7"), ());
  TEST(!HouseNumbersMatch("10/42 корпус 2", "42"), ());
  TEST(!HouseNumbersMatch("--...--.-", "--.....-"), ());
}
