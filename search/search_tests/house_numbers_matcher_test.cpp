#include "testing/testing.hpp"

#include "search/house_numbers_matcher.hpp"

#include "std/vector.hpp"

#include "base/string_utils.hpp"

using namespace strings;
using namespace search;

namespace
{
void ParseHouseNumber(string const & s, vector<vector<string>> & ts)
{
  vector<Parse> parses;
  ParseQuery(MakeUniString(s), false /* queryIsPrefix */, parses);
  for (auto const & parse : parses)
  {
    ts.emplace_back();
    auto & tsb = ts.back();
    for (auto const & part : parse.m_parts)
      tsb.push_back(ToUtf8(part));
  }
}

bool HouseNumbersMatch(string const & houseNumber, string const & query, bool queryIsPrefix = false)
{
  vector<Parse> queryParses;
  ParseQuery(MakeUniString(query), queryIsPrefix, queryParses);
  return search::HouseNumbersMatch(MakeUniString(houseNumber), queryParses);
}

bool CheckTokenizer(string const & utf8s, vector<string> const & expected)
{
  UniString utf32s = MakeUniString(utf8s);
  vector<HouseNumberTokenizer::Token> tokens;
  HouseNumberTokenizer::Tokenize(utf32s, tokens);

  vector<string> actual;
  for (auto const & token : tokens)
    actual.push_back(ToUtf8(token.m_token));
  if (actual != expected)
  {
    LOG(LINFO, ("actual:", actual, "expected:", expected));
    return false;
  }
  return true;
}

bool CheckParser(string const & utf8s, string const & expected)
{
  vector<vector<string>> parses;
  ParseHouseNumber(utf8s, parses);

  for (auto const & parse : parses)
  {
    string actual;
    for (size_t i = 0; i < parse.size(); ++i)
    {
      actual.append(parse[i]);
      if (i + 1 != parse.size())
        actual.push_back(' ');
    }
    if (actual == expected)
      return true;
  }

  LOG(LINFO, ("actual:", parses, "expected:", expected));
  return false;
}
}  // namespace

UNIT_TEST(HouseNumberTokenizer_Smoke)
{
  TEST(CheckTokenizer("123Б", {"123", "Б"}), ());
  TEST(CheckTokenizer("123/Б", {"123", "Б"}), ());
  TEST(CheckTokenizer("123/34 корп. 4 стр1", {"123", "34", "корп", "4", "стр", "1"}), ());
}

UNIT_TEST(HouseNumberNormalizer_Smoke)
{
  TEST(CheckParser("123Б", "123б"), ());
  TEST(CheckParser("123/4 Литер А", "123 4 а"), ());
  TEST(CheckParser("123а корп. 2б", "123а 2б"), ());
  TEST(CheckParser("123к4", "123 4"), ());
  TEST(CheckParser("123к Корпус 2", "123к 2"), ());
  TEST(CheckParser("9 литер А корпус 2", "9 2 а"), ());
  TEST(CheckParser("39с79", "39 79"), ());
  TEST(CheckParser("9 литер аб1", "9 аб1"), ());
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
  TEST(HouseNumbersMatch("10 к2 с2", "10 корпус 2"), ());
  TEST(HouseNumbersMatch("10 к2 с2", "10 корпус 2 с 2"), ());
  TEST(HouseNumbersMatch("10 корпус 2 строение 2", "10 к2 с2"), ());
  TEST(HouseNumbersMatch("10 корпус 2 строение 2", "10к2с2"), ());
  TEST(HouseNumbersMatch("10к2а", "10 2а"), ());
  TEST(HouseNumbersMatch("10 к2с", "10 2с"), ());
  TEST(HouseNumbersMatch("22к", "22 к"), ());
  TEST(HouseNumbersMatch("22к корпус 2а строение 7", "22к к 2а стр 7"), ());
  TEST(HouseNumbersMatch("22к к 2а с 7", "22к корпус 2а"), ());
  TEST(HouseNumbersMatch("124к корпус к", "124к к"), ());

  TEST(!HouseNumbersMatch("39", "39 с 79"), ());
  TEST(!HouseNumbersMatch("127а корпус 2", "127"), ());
  TEST(!HouseNumbersMatch("6 корпус 2", "7"), ());
  TEST(!HouseNumbersMatch("10/42 корпус 2", "42"), ());
  TEST(!HouseNumbersMatch("--...--.-", "--.....-"), ());
  TEST(!HouseNumbersMatch("22к", "22 корпус"), ());
  TEST(!HouseNumbersMatch("22к", "22я"), ());
  TEST(!HouseNumbersMatch("22к", "22л"), ());

  TEST(HouseNumbersMatch("39 корпус 79", "39", true /* queryIsPrefix */), ());
  TEST(HouseNumbersMatch("39 корпус 79", "39 кор", true /* queryIsPrefix */), ());
  TEST(!HouseNumbersMatch("39", "39 корп", true /* queryIsPrefix */), ());
  TEST(HouseNumbersMatch("39 корпус 7", "39", true /* queryIsPrefix */), ());
  TEST(HouseNumbersMatch("39К корпус 7", "39 к", true /* queryIsPrefix */), ());
  TEST(HouseNumbersMatch("39К корпус 7", "39к", true /* queryIsPrefix */), ());
  TEST(HouseNumbersMatch("39 К корпус 7", "39 к", false /* queryIsPrefix */), ());
  TEST(!HouseNumbersMatch("39 К корпус 7", "39", false /* queryIsPrefix */), ());
}
