#include "testing/testing.hpp"

#include "search/house_numbers_matcher.hpp"

#include "std/vector.hpp"

#include "base/string_utils.hpp"

using namespace search::house_numbers;
using namespace search;
using namespace strings;

namespace
{
bool HouseNumbersMatch(string const & houseNumber, string const & query, bool queryIsPrefix = false)
{
  return search::house_numbers::HouseNumbersMatch(MakeUniString(houseNumber), MakeUniString(query),
                                                  queryIsPrefix);
}

bool CheckTokenizer(string const & utf8s, vector<string> const & expected)
{
  UniString utf32s = MakeUniString(utf8s);
  vector<Token> tokens;
  Tokenize(utf32s, false /* isPrefix */, tokens);

  vector<string> actual;
  for (auto const & token : tokens)
    actual.push_back(ToUtf8(token.m_value));
  if (actual != expected)
  {
    LOG(LINFO, ("actual:", actual, "expected:", expected));
    return false;
  }
  return true;
}

bool CheckParser(string const & utf8s, string const & expected)
{
  vector<vector<Token>> parses;
  ParseHouseNumber(MakeUniString(utf8s), parses);

  if (parses.size() != 1)
  {
    LOG(LINFO, ("Actual:", parses, "expected:", expected));
    return false;
  }

  auto const & parse = parses[0];
  string actual;
  for (size_t i = 0; i < parse.size(); ++i)
  {
    actual.append(ToUtf8(parse[i].m_value));
    if (i + 1 != parse.size())
      actual.push_back(' ');
  }

  if (actual != expected)
  {
    LOG(LINFO, ("Actual:", parses, "expected:", expected));
    return false;
  }

  return true;
}

bool LooksLikeHouseNumber(string const & s, bool isPrefix)
{
  return house_numbers::LooksLikeHouseNumber(MakeUniString(s), isPrefix);
}
}  // namespace

UNIT_TEST(HouseNumberTokenizer_Smoke)
{
  TEST(CheckTokenizer("123Б", {"123", "б"}), ());
  TEST(CheckTokenizer("123/Б", {"123", "/", "б"}), ());
  TEST(CheckTokenizer("123/34 корп. 4 стр1", {"123", "/", "34", "корп", "4", "стр", "1"}), ());
  TEST(CheckTokenizer("1-100", {"1", "-", "100"}), ());
  TEST(CheckTokenizer("19/1А литБ", {"19", "/", "1", "а", "лит", "б"}), ());
  TEST(CheckTokenizer("9 литер аб1", {"9", "литер", "аб", "1"}), ());
}

UNIT_TEST(HouseNumberParser_Smoke)
{
  TEST(CheckParser("123Б", "123 б"), ());
  TEST(CheckParser("123/4 Литер А", "123 4 а"), ());
  TEST(CheckParser("123а корп. 2б", "123 2 а б"), ());
  TEST(CheckParser("123к4", "123 4"), ());
  TEST(CheckParser("123к Корпус 2", "123 2 к"), ());
  TEST(CheckParser("9 литер А корпус 2", "9 2 а"), ());
  TEST(CheckParser("39с79", "39 79"), ());
  TEST(CheckParser("9 литер аб1", "9 1"), ());
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
  TEST(HouseNumbersMatch("127а корпус 2", "127"), ());
  TEST(HouseNumbersMatch("22к", "22 корпус"), ());

  TEST(HouseNumbersMatch("39 корпус 79", "39", true /* queryIsPrefix */), ());
  TEST(HouseNumbersMatch("39 корпус 79", "39 кор", true /* queryIsPrefix */), ());
  TEST(HouseNumbersMatch("39", "39 корп", true /* queryIsPrefix */), ());
  TEST(HouseNumbersMatch("39 корпус 7", "39", true /* queryIsPrefix */), ());
  TEST(HouseNumbersMatch("39К корпус 7", "39 к", true /* queryIsPrefix */), ());
  TEST(HouseNumbersMatch("39К корпус 7", "39к", true /* queryIsPrefix */), ());
  TEST(HouseNumbersMatch("39 К корпус 7", "39 к", false /* queryIsPrefix */), ());
  TEST(HouseNumbersMatch("39 К корпус 7", "39", false /* queryIsPrefix */), ());

  TEST(HouseNumbersMatch("3/7 с1Б", "3/7 строение 1 Б", true /* queryIsPrefix */), ());
  TEST(HouseNumbersMatch("3/7 с1Б", "3/7 строение 1 Б", false /* queryIsPrefix */), ());
  TEST(!HouseNumbersMatch("3/7 с1Б", "3/7 с 1Д", false /* queryIsPrefix */), ());

  TEST(!HouseNumbersMatch("39", "39 с 79"), ());
  TEST(!HouseNumbersMatch("6 корпус 2", "7"), ());
  TEST(!HouseNumbersMatch("10/42 корпус 2", "42"), ());
  TEST(!HouseNumbersMatch("22к", "22я"), ());
  TEST(!HouseNumbersMatch("22к", "22л"), ());

  TEST(HouseNumbersMatch("16 к1", "д 16 к 1"), ());
  TEST(HouseNumbersMatch("16 к1", "д 16 к1"), ());
  TEST(HouseNumbersMatch("16 к1", "16 к1"), ());
  TEST(HouseNumbersMatch("16 к1", "дом 16 к1"), ());
  TEST(HouseNumbersMatch("14 д 1", "дом 14 д1"), ());
}

UNIT_TEST(LooksLikeHouseNumber_Smoke)
{
  TEST(LooksLikeHouseNumber("1", false /* isPrefix */), ());
  TEST(LooksLikeHouseNumber("ev 10", false /* isPrefix */), ());

  TEST(LooksLikeHouseNumber("14 к", true /* isPrefix */), ());
  TEST(LooksLikeHouseNumber("14 кор", true /* isPrefix */), ());
  TEST(LooksLikeHouseNumber("14 корпус", true /*isPrefix */), ());
  TEST(LooksLikeHouseNumber("14 корпус 1", true /* isPrefix */), ());
  TEST(LooksLikeHouseNumber("14 корпус 1", false /* isPrefix */), ());

  TEST(LooksLikeHouseNumber("39 c 79", false /* isPrefix */), ());
  TEST(LooksLikeHouseNumber("владение 14", false /* isPrefix */), ());

  TEST(LooksLikeHouseNumber("4", false /* isPrefix */), ());
  TEST(LooksLikeHouseNumber("4 2", false /* isPrefix */), ());
  TEST(!LooksLikeHouseNumber("4 2 останкинская", false /* isPrefix */), ());

  TEST(!LooksLikeHouseNumber("39 c 79 ленинградский", false /* isPrefix */), ());
  TEST(!LooksLikeHouseNumber("каптерка 1", false /* isPrefix */), ());
  TEST(!LooksLikeHouseNumber("1 канал", false /* isPrefix */), ());
  TEST(!LooksLikeHouseNumber("2 останкинская", false /* isPrefix */), ());

  TEST(LooksLikeHouseNumber("39 строе", true /* isPrefix */), ());
  TEST(!LooksLikeHouseNumber("39 строе", false /* isPrefix */), ());
  TEST(LooksLikeHouseNumber("39", true /* isPrefix */), ());
  TEST(LooksLikeHouseNumber("39", false /* isPrefix */), ());
  TEST(LooksLikeHouseNumber("строе", true /* isPrefix */), ());
  TEST(!LooksLikeHouseNumber("строе", false /* isPrefix */), ());

  TEST(LooksLikeHouseNumber("дом ", true /* isPrefix */), ());
  TEST(LooksLikeHouseNumber("дом ", false /* isPrefix */), ());

  TEST(LooksLikeHouseNumber("дом 39 строение 79", false /* isPrefix */), ());

  TEST(LooksLikeHouseNumber("3/7 с1Б", false /* isPrefix */), ());
  TEST(LooksLikeHouseNumber("3/7 с1Б", true /* isPrefix */), ());

  TEST(LooksLikeHouseNumber("16 к 1", false /* isPrefix */), ());
  TEST(LooksLikeHouseNumber("д 16 к 1", false /* isPrefix */), ());
  TEST(LooksLikeHouseNumber("дом 16 к 1", false /* isPrefix */), ());
  TEST(LooksLikeHouseNumber("д 16", false /* isPrefix */), ());
  TEST(LooksLikeHouseNumber("дом 16", false /* isPrefix */), ());
  TEST(LooksLikeHouseNumber("дом 14 д 1", false /* isPrefix */), ());
}
