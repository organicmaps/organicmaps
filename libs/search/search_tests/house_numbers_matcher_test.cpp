#include "testing/testing.hpp"

#include "search/house_numbers_matcher.hpp"

#include "base/string_utils.hpp"

#include <string>
#include <vector>

namespace house_number_matcher_test
{
using namespace search::house_numbers;
using namespace strings;
using namespace std;

bool HouseNumbersMatch(string const & houseNumber, string const & query, bool queryIsPrefix = false)
{
  vector<Token> queryParse;
  ParseQuery(MakeUniString(query), queryIsPrefix, queryParse);
  return search::house_numbers::HouseNumbersMatch(MakeUniString(houseNumber), queryParse);
}

bool HouseNumbersMatchConscription(string const & houseNumber, string const & query, bool queryIsPrefix = false)
{
  vector<Token> queryParse;
  ParseQuery(MakeUniString(query), queryIsPrefix, queryParse);
  return search::house_numbers::HouseNumbersMatchConscription(MakeUniString(houseNumber), queryParse);
}

bool HouseNumbersMatchRange(string_view const & hnRange, string const & query, feature::InterpolType interpol)
{
  vector<Token> queryParse;
  ParseQuery(MakeUniString(query), false /* isPrefix */, queryParse);
  return search::house_numbers::HouseNumbersMatchRange(hnRange, queryParse, interpol);
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

UNIT_TEST(HouseNumber_ToUInt)
{
  TEST_EQUAL(ToUInt(MakeUniString("1234987650")), 1234987650ULL, ());
}

UNIT_TEST(HouseNumber_Tokenizer)
{
  TEST(CheckTokenizer("123Б", {"123", "б"}), ());
  TEST(CheckTokenizer("123/Б", {"123", "/", "б"}), ());
  TEST(CheckTokenizer("123/34 корп. 4 стр1", {"123", "/", "34", "корп", "4", "стр", "1"}), ());
  TEST(CheckTokenizer("1-100", {"1", "-", "100"}), ());
  TEST(CheckTokenizer("19/1А литБ", {"19", "/", "1", "а", "лит", "б"}), ());
  TEST(CheckTokenizer("9 литер аб1", {"9", "литер", "аб", "1"}), ());
}

UNIT_TEST(HouseNumber_Parser)
{
  TEST(CheckParser("123Б", "123 б"), ());
  TEST(CheckParser("123/4 Литер А", "123 4 а"), ());
  TEST(CheckParser("123а корп. 2б", "123 2 а б"), ());
  TEST(CheckParser("123к4", "123 4"), ());
  TEST(CheckParser("123к Корпус 2", "123 2 к"), ());
  TEST(CheckParser("9 литер А корпус 2", "9 2 а"), ());
  TEST(CheckParser("39с79", "39 79"), ());
  TEST(CheckParser("9 литер аб1", "9 1"), ());
  TEST(CheckParser("1-100", "1 100"), ());
}

UNIT_TEST(HouseNumber_Matcher)
{
  TEST(HouseNumbersMatch("39с79", "39"), ());
  TEST(HouseNumbersMatch("39с79", "39 Строение 79"), ());
  TEST(HouseNumbersMatch("39с79", "39 к. 79"), ());
  TEST(HouseNumbersMatch("39 - 79", "39 строение 79"), ());
  TEST(!HouseNumbersMatch("39-79", "49"), ());
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

  TEST(HouseNumbersMatch("39с80", "39"), ());
  TEST(HouseNumbersMatch("39", "39 с 80"), ());
  TEST(!HouseNumbersMatch("39 c 80", "39 с 79"), ());
  TEST(!HouseNumbersMatch("39 c 79", "39 с 80"), ());

  TEST(!HouseNumbersMatch("6 корпус 2", "7"), ());
  TEST(!HouseNumbersMatch("10/42 корпус 2", "42"), ());

  TEST(HouseNumbersMatch("22", "22к"), ());
  TEST(HouseNumbersMatch("22к", "22"), ());
  TEST(!HouseNumbersMatch("22к", "22я"), ());
  TEST(!HouseNumbersMatch("22к", "22л"), ());

  TEST(HouseNumbersMatch("16 к1", "д 16 к 1"), ());
  TEST(HouseNumbersMatch("16 к1", "д 16 к1"), ());
  TEST(HouseNumbersMatch("16 к1", "16 к1"), ());
  TEST(HouseNumbersMatch("16 к1", "дом 16 к1"), ());
  TEST(HouseNumbersMatch("14 д 1", "дом 14 д1"), ());

  TEST(HouseNumbersMatch("12;14", "12"), ());
  TEST(HouseNumbersMatch("12;14", "14"), ());
  TEST(!HouseNumbersMatch("12;14", "13"), ());
  TEST(HouseNumbersMatch("12,14", "12"), ());
  TEST(HouseNumbersMatch("12,14", "14"), ());
  TEST(!HouseNumbersMatch("12,14", "13"), ());
}

UNIT_TEST(HouseNumber_Matcher_Conscription)
{
  TEST(HouseNumbersMatchConscription("77/21", "77"), ());
  TEST(HouseNumbersMatchConscription("77 b/21", "77b"), ());

  TEST(HouseNumbersMatchConscription("77/21", "21"), ());
  TEST(HouseNumbersMatchConscription("77/21 a", "21a"), ());

  TEST(HouseNumbersMatchConscription("77/21", "77/21"), ());
  TEST(HouseNumbersMatchConscription("77/21", "21/77"), ());

  TEST(HouseNumbersMatchConscription("77x/21y", "77"), ());
  TEST(HouseNumbersMatchConscription("77x/21y", "21"), ());

  /// @todo Controversial, but need skip ParseQuery and treat query as 2 separate inputs.
  /// @{
  TEST(HouseNumbersMatchConscription("77/21", "77x/21y"), ());
  TEST(!HouseNumbersMatchConscription("77x/21y", "77/21"), ());
  TEST(!HouseNumbersMatchConscription("78/21", "77/21"), ());
  /// @}
}

UNIT_TEST(HouseNumber_Matcher_Range)
{
  using IType = feature::InterpolType;

  TEST(HouseNumbersMatchRange("3000:3100", "3088", IType::Even), ());
  TEST(!HouseNumbersMatchRange("3000:3100", "3088", IType::Odd), ());
  TEST(!HouseNumbersMatchRange("3000:3100", "3000", IType::Any), ());
  TEST(!HouseNumbersMatchRange("3000:3100", "3100", IType::Any), ());
  TEST(HouseNumbersMatchRange("2:40", "32A", IType::Even), ());
  TEST(!HouseNumbersMatchRange("2:40", "33B", IType::Even), ());

  /// @todo Maybe some day ..
  // TEST(HouseNumbersMatchRange("30A:30D", "30B", IType::Any), ());
}

UNIT_TEST(HouseNumber_LooksLike)
{
  TEST(LooksLikeHouseNumber("1", false /* isPrefix */), ());
  TEST(LooksLikeHouseNumber("ev 10", false /* isPrefix */), ());
  TEST(LooksLikeHouseNumber("ev.1", false /* isPrefix */), ());

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
  TEST(LooksLikeHouseNumber("house", true /* isPrefix */), ());
  TEST(LooksLikeHouseNumber("house ", false /* isPrefix */), ());

  TEST(LooksLikeHouseNumber("дом 39 строение 79", false /* isPrefix */), ());

  TEST(LooksLikeHouseNumber("3/7 с1Б", false /* isPrefix */), ());
  TEST(LooksLikeHouseNumber("3/7 с1Б", true /* isPrefix */), ());

  TEST(LooksLikeHouseNumber("16 к 1", false /* isPrefix */), ());
  TEST(LooksLikeHouseNumber("д 16 к 1", false /* isPrefix */), ());
  TEST(LooksLikeHouseNumber("дом 16 к 1", false /* isPrefix */), ());
  TEST(LooksLikeHouseNumber("д 16", false /* isPrefix */), ());
  TEST(LooksLikeHouseNumber("дом 16", false /* isPrefix */), ());
  TEST(LooksLikeHouseNumber("дом 14 д 1", false /* isPrefix */), ());

  TEST(LooksLikeHouseNumber("12;14", false /* isPrefix */), ());
  TEST(LooksLikeHouseNumber("12,14", true /* isPrefix */), ());

  TEST(!LooksLikeHouseNumber("улица", false /* isPrefix */), ());
  TEST(!LooksLikeHouseNumber("avenida", false /* isPrefix */), ());
  TEST(!LooksLikeHouseNumber("street", false /* isPrefix */), ());

  TEST(LooksLikeHouseNumber("2:40", false /* isPrefix */), ());

  TEST(LooksLikeHouseNumber("к424", false /* isPrefix */), ());
  TEST(LooksLikeHouseNumber("к12", true /* isPrefix */), ());
}
}  // namespace house_number_matcher_test
