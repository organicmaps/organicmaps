#include "testing/testing.hpp"

#include "base/string_utils.hpp"
#include "base/logging.hpp"

#include <functional>
#include <fstream>
#include <iomanip>
#include <map>
#include <unordered_map>
#include <vector>

#include <sstream>

/// internal function in base
namespace strings { UniChar LowerUniChar(UniChar c); }

UNIT_TEST(LowerUniChar)
{
  // Load unicode case folding table.

  // To use Platform class here, we need to add many link stuff into .pro file ...
  //string const fName = GetPlatform().WritablePathForFile("CaseFolding.test");
  std::string const fName = "../../../omim/data/CaseFolding.test";

  std::ifstream file(fName.c_str());
  if (!file.good())
  {
    LOG(LWARNING, ("Can't open unicode test file", fName));
    return;
  }

  size_t fCount = 0, cCount = 0;
  typedef std::unordered_map<strings::UniChar, strings::UniString> mymap;
  mymap m;
  std::string line;
  while (file.good())
  {
    std::getline(file, line);
    // strip comments
    size_t const sharp = line.find('#');
    if (sharp != string::npos)
      line.erase(sharp);
    strings::SimpleTokenizer semicolon(line, ";");
    if (!semicolon)
      continue;
    std::string const capital = *semicolon;
    std::istringstream stream(capital);
    strings::UniChar uc;
    stream >> std::hex >> uc;
    ++semicolon;
    std::string const type = *semicolon;
    if (type == " S" || type == " T")
      continue;
    if (type != " C" && type != " F")
      continue;
    ++semicolon;
    std::string const outChars = *semicolon;
    strings::UniString us;
    strings::SimpleTokenizer spacer(outChars, " ");
    while (spacer)
    {
      stream.clear();
      stream.str(*spacer);
      strings::UniChar smallCode;
      stream >> std::hex >> smallCode;
      us.push_back(smallCode);
      ++spacer;
    }
    switch (us.size())
    {
      case 0: continue;
      case 1:
      {
        m[uc] = us;
        ++cCount;
        TEST_EQUAL(strings::LowerUniChar(uc), us[0], ());
        TEST_EQUAL(type, " C", ());
        break;
      }
      default: m[uc] = us; ++fCount; TEST_EQUAL(type, " F", ()); break;
    }
  }
  LOG(LINFO, ("Loaded", cCount, "common foldings and", fCount, "full foldings"));

  // full range unicode table test
  for (strings::UniChar c = 0; c < 0x11000; ++c)
  {
    mymap::iterator found = m.find(c);
    if (found == m.end())
    {
      TEST_EQUAL(c, strings::LowerUniChar(c), ());
    }
    else
    {
      strings::UniString const capitalChar(1, c);
      TEST_EQUAL(strings::MakeLowerCase(capitalChar), found->second, ());
    }
  }
}

UNIT_TEST(MakeLowerCase)
{
  std::string s;

  s = "THIS_IS_UPPER";
  strings::MakeLowerCaseInplace(s);
  TEST_EQUAL(s, "this_is_upper", ());

  s = "THIS_iS_MiXed";
  strings::MakeLowerCaseInplace(s);
  TEST_EQUAL(s, "this_is_mixed", ());

  s = "this_is_lower";
  strings::MakeLowerCaseInplace(s);
  TEST_EQUAL(s, "this_is_lower", ());

  std::string const utf8("Hola! 99-\xD0\xA3\xD0\x9F\xD0\xAF\xD0\xA7\xD0\x9A\xD0\x90");
  TEST_EQUAL(strings::MakeLowerCase(utf8),
    "hola! 99-\xD1\x83\xD0\xBF\xD1\x8F\xD1\x87\xD0\xBA\xD0\xB0", ());

  s = "\xc3\x9f"; // es-cet
  strings::MakeLowerCaseInplace(s);
  TEST_EQUAL(s, "ss", ());

  strings::UniChar const arr[] = {0x397, 0x10B4, 'Z'};
  strings::UniChar const carr[] = {0x3b7, 0x2d14, 'z'};
  strings::UniString const us(&arr[0], &arr[0] + ARRAY_SIZE(arr));
  strings::UniString const cus(&carr[0], &carr[0] + ARRAY_SIZE(carr));
  TEST_EQUAL(cus, strings::MakeLowerCase(us), ());
}

UNIT_TEST(EqualNoCase)
{
  TEST(strings::EqualNoCase("HaHaHa", "hahaha"), ());
}

UNIT_TEST(to_double)
{
  std::string s;
  double d;

  s = "";
  TEST(!strings::to_double(s, d), ());

  s = "0.123";
  TEST(strings::to_double(s, d), ());
  TEST_ALMOST_EQUAL_ULPS(0.123, d, ());

  s = "1.";
  TEST(strings::to_double(s, d), ());
  TEST_ALMOST_EQUAL_ULPS(1.0, d, ());

  s = "0";
  TEST(strings::to_double(s, d), ());
  TEST_ALMOST_EQUAL_ULPS(0., d, ());

  s = "5.6843418860808e-14";
  TEST(strings::to_double(s, d), ());
  TEST_ALMOST_EQUAL_ULPS(5.6843418860808e-14, d, ());

  s = "-2";
  TEST(strings::to_double(s, d), ());
  TEST_ALMOST_EQUAL_ULPS(-2.0, d, ());

  s = "labuda";
  TEST(!strings::to_double(s, d), ());

  s = "123.456 we don't parse it.";
  TEST(!strings::to_double(s, d), ());

  TEST(!strings::to_double("INF", d), ());
  TEST(!strings::to_double("NAN", d), ());
  TEST(!strings::to_double("1.18973e+4932", d), ());
}

UNIT_TEST(to_int)
{
  int i;
  std::string s;

  s = "AF";
  TEST(strings::to_int(s, i, 16), ());
  TEST_EQUAL(175, i, ());

  s = "-2";
  TEST(strings::to_int(s, i), ());
  TEST_EQUAL(-2, i, ());

  s = "0";
  TEST(strings::to_int(s, i), ());
  TEST_EQUAL(0, i, ());

  s = "123456789";
  TEST(strings::to_int(s, i), ());
  TEST_EQUAL(123456789, i, ());

  s = "labuda";
  TEST(!strings::to_int(s, i), ());
}

UNIT_TEST(to_uint)
{
  unsigned int i;
  std::string s;

  s = "";
  TEST(!strings::to_uint(s, i), ());

  s = "-2";
  TEST(!strings::to_uint(s, i), ());

  s = "0";
  TEST(strings::to_uint(s, i), ());
  TEST_EQUAL(0, i, ());

  s = "123456789123456789123456789";
  TEST(!strings::to_uint(s, i), ());

  s = "labuda";
  TEST(!strings::to_uint(s, i), ());

  s = "AF";
  TEST(strings::to_uint(s, i, 16), ());
  TEST_EQUAL(175, i, ());

  s = "100";
  TEST(strings::to_uint(s, i), ());
  TEST_EQUAL(100, i, ());

  s = "4294967295";
  TEST(strings::to_uint(s, i), ());
  TEST_EQUAL(0xFFFFFFFF, i, ());

  s = "4294967296";
  TEST(!strings::to_uint(s, i), ());
}

UNIT_TEST(to_uint64)
{
  uint64_t i;
  std::string s;

  s = "";
  TEST(!strings::to_uint64(s, i), ());

  s = "0";
  TEST(strings::to_uint64(s, i), ());
  TEST_EQUAL(0, i, ());

  s = "123456789101112";
  TEST(strings::to_uint64(s, i), ());
  TEST_EQUAL(123456789101112ULL, i, ());

  s = "labuda";
  TEST(!strings::to_uint64(s, i), ());
}

UNIT_TEST(to_int64)
{
  int64_t i;
  std::string s;

  s = "-24567";
  TEST(strings::to_int64(s, i), ());
  TEST_EQUAL(-24567, i, ());

  s = "0";
  TEST(strings::to_int64(s, i), ());
  TEST_EQUAL(0, i, ());

  s = "12345678911212";
  TEST(strings::to_int64(s, i), ());
  TEST_EQUAL(12345678911212LL, i, ());

  s = "labuda";
  TEST(!strings::to_int64(s, i), ());
}

UNIT_TEST(to_string)
{
  TEST_EQUAL(strings::to_string(0), "0", ());
  TEST_EQUAL(strings::to_string(-0), "0", ());
  TEST_EQUAL(strings::to_string(1), "1", ());
  TEST_EQUAL(strings::to_string(-1), "-1", ());
  TEST_EQUAL(strings::to_string(1234567890), "1234567890", ());
  TEST_EQUAL(strings::to_string(-987654321), "-987654321", ());
  TEST_EQUAL(strings::to_string(0.56), "0.56", ());
  TEST_EQUAL(strings::to_string(-100.2), "-100.2", ());

  // 6 digits after the comma with rounding - it's a default behavior
  TEST_EQUAL(strings::to_string(-0.66666666), "-0.666667", ());

  TEST_EQUAL(strings::to_string(-1.0E2), "-100", ());
  TEST_EQUAL(strings::to_string(1.0E-2), "0.01", ());

  TEST_EQUAL(strings::to_string(123456789123456789ULL), "123456789123456789", ());
  TEST_EQUAL(strings::to_string(-987654321987654321LL), "-987654321987654321", ());

  uint64_t const n = std::numeric_limits<uint64_t>::max();
  TEST_EQUAL(strings::to_string(n), "18446744073709551615", ());
}

UNIT_TEST(to_string_dac)
{
  TEST_EQUAL(strings::to_string_dac(99.9999, 3), "100", ());
  TEST_EQUAL(strings::to_string_dac(-99.9999, 3), "-100", ());
  TEST_EQUAL(strings::to_string_dac(-10.66666666, 7), "-10.6666667", ());
  TEST_EQUAL(strings::to_string_dac(10001.66666666, 8), "10001.66666666", ());
  TEST_EQUAL(strings::to_string_dac(99999.99999999, 8), "99999.99999999", ());
  TEST_EQUAL(strings::to_string_dac(0.7777, 3), "0.778", ());
  TEST_EQUAL(strings::to_string_dac(-0.333333, 4), "-0.3333", ());
  TEST_EQUAL(strings::to_string_dac(2.33, 2), "2.33", ());

  TEST_EQUAL(strings::to_string_dac(-0.0039, 2), "-0", ());
  TEST_EQUAL(strings::to_string_dac(0.0039, 2), "0", ());
  TEST_EQUAL(strings::to_string_dac(-1.0039, 2), "-1", ());
  TEST_EQUAL(strings::to_string_dac(1.0039, 2), "1", ());

  TEST_EQUAL(strings::to_string_dac(0., 5), "0", ());
  TEST_EQUAL(strings::to_string_dac(0., 0), "0", ());

  TEST_EQUAL(strings::to_string_dac(1.0, 6), "1", ());
  TEST_EQUAL(strings::to_string_dac(0.9, 6), "0.9", ());
  TEST_EQUAL(strings::to_string_dac(-1.0, 30), "-1", ());
  TEST_EQUAL(strings::to_string_dac(-0.99, 30), "-0.99", ());

  TEST_EQUAL(strings::to_string_dac(1.0E30, 6), "1e+30", ());
  TEST_EQUAL(strings::to_string_dac(1.0E-15, 15), "0.000000000000001", ());
  TEST_EQUAL(strings::to_string_dac(1.0 + 1.0E-14, 15), "1.00000000000001", ());
}

struct FunctorTester
{
  size_t & m_index;
  std::vector<std::string> const & m_tokens;

  FunctorTester(size_t & counter, std::vector<std::string> const & tokens)
    : m_index(counter), m_tokens(tokens)
  {
  }

  void operator()(std::string const & s)
  {
    TEST_EQUAL(s, m_tokens[m_index++], ());
  }
};

void TestIter(std::string const & s, char const * delims, std::vector<std::string> const & tokens)
{
  strings::SimpleTokenizer it(s, delims);
  for (size_t i = 0; i < tokens.size(); ++i)
  {
    TEST(it, (s, delims, i));
    TEST_EQUAL(*it, tokens[i], (s, delims, i));
    ++it;
  }
  TEST(!it, (s, delims));

  size_t counter = 0;
  FunctorTester f(counter, tokens);
  strings::Tokenize(s, delims, f);
  TEST_EQUAL(counter, tokens.size(), ());
}

void TestIterWithEmptyTokens(std::string const & s, char const * delims, std::vector<std::string> const & tokens)
{
  strings::SimpleTokenizerWithEmptyTokens it(s, delims);

  for (size_t i = 0; i < tokens.size(); ++i)
  {
    TEST(it, (s, delims, i));
    TEST_EQUAL(*it, tokens[i], (s, delims, i));
    ++it;
  }
  TEST(!it, (s, delims));
}

UNIT_TEST(SimpleTokenizer)
{
  std::vector<std::string> tokens;
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

  {
    char const * s[] = {"27.535536", "53.884926" , "189"};
    tokens.assign(&s[0], &s[0] + ARRAY_SIZE(s));
    TestIter("27.535536,53.884926,189", ",", tokens);
  }

  {
    char const * s[] = {"1", "2"};
    tokens.assign(&s[0], &s[0] + ARRAY_SIZE(s));
    TestIter("/1/2/", "/", tokens);
  }

  {
    std::string const s = "";
    std::vector<std::string> const tokens = {""};
    TestIterWithEmptyTokens(s, ",", tokens);
  }

  {
    std::string const s = ";";
    std::vector<std::string> const tokens = {"", ""};
    TestIterWithEmptyTokens(s, ";", tokens);
  }

  {
    std::string const s = ";;";
    std::vector<std::string> const tokens = {"", "", ""};
    TestIterWithEmptyTokens(s, ";", tokens);
  }

  {
    std::string const s = "Hello, World!";
    std::vector<std::string> const tokens = {s};
    TestIterWithEmptyTokens(s, "", tokens);
  }

  {
    std::string const s = "Hello, World!";
    std::vector<std::string> const tokens = {"Hello", " World", ""};
    TestIterWithEmptyTokens(s, ",!", tokens);
  }

  {
    std::string const s = ";a;b;;c;d;";
    std::vector<std::string> const tokens = {"", "a", "b", "", "c", "d", ""};
    TestIterWithEmptyTokens(s, ";", tokens);
  }
}

UNIT_TEST(Tokenize)
{
  {
    std::initializer_list<std::string> expected{"acb", "def", "ghi"};
    TEST_EQUAL(strings::Tokenize<std::vector>("acb def ghi", " " /* delims */), std::vector<std::string>(expected), ());
    TEST_EQUAL(strings::Tokenize<std::set>("acb def ghi", " " /* delims */), std::set<std::string>(expected), ());
  }
}

UNIT_TEST(LastUniChar)
{
  TEST_EQUAL(strings::LastUniChar(""), 0, ());
  TEST_EQUAL(strings::LastUniChar("Hello"), 0x6f, ());
  TEST_EQUAL(strings::LastUniChar(" \xD0\x90"), 0x0410, ());
}

UNIT_TEST(GetUniString)
{
  std::string const s = "Hello, \xD0\x9C\xD0\xB8\xD0\xBD\xD1\x81\xD0\xBA!";
  strings::SimpleTokenizer iter(s, ", !");
  {
    strings::UniChar const s[] = { 'H', 'e', 'l', 'l', 'o' };
    strings::UniString us(&s[0], &s[0] + ARRAY_SIZE(s));
    TEST_EQUAL(iter.GetUniString(), us, ());
  }
  ++iter;
  {
    strings::UniChar const s[] = { 0x041C, 0x0438, 0x043D, 0x0441, 0x043A };
    strings::UniString us(&s[0], &s[0] + ARRAY_SIZE(s));
    TEST_EQUAL(iter.GetUniString(), us, ());
  }
}

UNIT_TEST(MakeUniString_Smoke)
{
  char const s [] = "Hello!";
  TEST_EQUAL(strings::UniString(&s[0], &s[0] + ARRAY_SIZE(s) - 1), strings::MakeUniString(s), ());
}

UNIT_TEST(Normalize)
{
  strings::UniChar const s[] = { 0x1f101, 'H', 0xfef0, 0xfdfc, 0x2150 };
  strings::UniString us(&s[0], &s[0] + ARRAY_SIZE(s));
  strings::UniChar const r[] = { 0x30, 0x2c, 'H', 0x649, 0x631, 0x6cc, 0x627, 0x644,
                                      0x31, 0x2044, 0x37 };
  strings::UniString result(&r[0], &r[0] + ARRAY_SIZE(r));
  strings::NormalizeInplace(us);
  TEST_EQUAL(us, result, ());
}

UNIT_TEST(Normalize_Special)
{
  {
    std::string const utf8 = "ąĄćłŁÓŻźŃĘęĆ";
    TEST_EQUAL(strings::ToUtf8(strings::Normalize(strings::MakeUniString(utf8))), "aAclLOZzNEeC", ());
  }

  {
    std::string const utf8 = "əüöğ";
    TEST_EQUAL(strings::ToUtf8(strings::Normalize(strings::MakeUniString(utf8))), "əuog", ());
  }
}

UNIT_TEST(UniStringToUtf8)
{
  char const utf8Text[] = "У нас исходники хранятся в Utf8!";
  strings::UniString uniS = strings::MakeUniString(utf8Text);
  TEST_EQUAL(std::string(utf8Text), strings::ToUtf8(uniS), ());
}

UNIT_TEST(StartsWith)
{
  using namespace strings;

  TEST(StartsWith(std::string(), ""), ());

  std::string s("xyz");
  TEST(StartsWith(s, ""), ());
  TEST(StartsWith(s, "x"), ());
  TEST(StartsWith(s, "xyz"), ());
  TEST(!StartsWith(s, "xyzabc"), ());
  TEST(!StartsWith(s, "ayz"), ());
  TEST(!StartsWith(s, "axy"), ());

  UniString const us = MakeUniString(s);
  TEST(StartsWith(us, UniString()), ());
  TEST(StartsWith(us, MakeUniString("x")), ());
  TEST(StartsWith(us, MakeUniString("xyz")), ());
  TEST(!StartsWith(us, MakeUniString("xyzabc")), ());
  TEST(!StartsWith(us, MakeUniString("ayz")), ());
  TEST(!StartsWith(us, MakeUniString("axy")), ());
}

UNIT_TEST(EndsWith)
{
  using namespace strings;
  TEST(EndsWith(std::string(), ""), ());

  std::string s("xyz");
  TEST(EndsWith(s, ""), ());
  TEST(EndsWith(s, "z"), ());
  TEST(EndsWith(s, "yz"), ());
  TEST(EndsWith(s, "xyz"), ());
  TEST(!EndsWith(s, "abcxyz"), ());
  TEST(!EndsWith(s, "ayz"), ());
  TEST(!EndsWith(s, "axyz"), ());
}

UNIT_TEST(UniString_LessAndEqualsAndNotEquals)
{
  std::vector<strings::UniString> v;
  v.push_back(strings::MakeUniString(""));
  v.push_back(strings::MakeUniString("Tes"));
  v.push_back(strings::MakeUniString("Test"));
  v.push_back(strings::MakeUniString("TestT"));
  v.push_back(strings::MakeUniString("TestTestTest"));
  v.push_back(strings::MakeUniString("To"));
  v.push_back(strings::MakeUniString("To!"));
  for (size_t i = 0; i < v.size(); ++i)
  {
    TEST(v[i] == v[i], ());
    TEST(!(v[i] < v[i]), ());
    for (size_t j = i + 1; j < v.size(); ++j)
    {
      TEST(v[i] < v[j], ());
      TEST(!(v[j] < v[i]), ());
      TEST(v[i] != v[j], ());
      TEST(v[j] != v[i], ());
    }
  }
}

UNIT_TEST(IsUtf8Test)
{
  TEST(!strings::IsASCIIString("Нет"), ());
  TEST(!strings::IsASCIIString("Классненькие места в Жодино.kml"), ());
  TEST(!strings::IsASCIIString("在Zhodino陰涼處.kml"), ());
  TEST(!strings::IsASCIIString("מקום קריר בZhodino.kml"), ());

  TEST(strings::IsASCIIString("YES"), ());
  TEST(strings::IsASCIIString("Nice places in Zhodino.kml"), ());
}

UNIT_TEST(CountNormLowerSymbols)
{
  char const * strs[] = {
    "æüßs",
    "üßü",
    "İŉẖtestὒ",
    "İŉẖ",
    "İŉẖtestὒ",
    "HelloWorld",
    "üßü",
    "",
    "",
    "Тест на не корректную русскую строку",
    "В ответе пустая строка",
    "Überstraße"
  };

  char const * low_strs[] = {
    "æusss",
    "ussu",
    "i\u0307\u02bcnh\u0331testυ\u0313\u0300",
    "i\u0307\u02bcnh\u0331testυ\u0313\u0300",
    "i\u0307\u02bcnh\u0331",
    "helloworld",
    "usu",
    "",
    "empty",
    "Тест на не корректную строку",
    "",
    "uberstras"
  };

  size_t const results [] = {
    4,
    3,
    8,
    0,
    3,
    10,
    0,
    0,
    0,
    0,
    0,
    9
  };


  size_t const test_count = ARRAY_SIZE(strs);

  for (size_t i = 0; i < test_count; ++i)
  {
    strings::UniString source = strings::MakeUniString(strs[i]);
    strings::UniString result = strings::MakeUniString(low_strs[i]);

    size_t res = strings::CountNormLowerSymbols(source, result);
    TEST_EQUAL(res, results[i], ());
  }
}

UNIT_TEST(IsHTML)
{
  using namespace strings;

  TEST(IsHTML("<a href=\"link\">some link</a>"), ());
  TEST(IsHTML("This is: ---> a <b>broken</b> html"), ());
  TEST(!IsHTML("This is not html"), ());
  TEST(!IsHTML("This is not html < too!"), ());
  TEST(!IsHTML("I am > not html"), ());
}

UNIT_TEST(AlmostEqual)
{
  using namespace strings;

  TEST(AlmostEqual("МКАД, 70-й километр", "МКАД, 79-й километр", 2), ());
  TEST(AlmostEqual("MKAD, 60 km", "MKAD, 59 km", 2), ());
  TEST(AlmostEqual("KAD, 5-y kilometre", "KAD, 7-y kilometre", 1), ());
  TEST(AlmostEqual("", "", 2), ());
  TEST(AlmostEqual("The Vista", "The Vista", 2), ());
  TEST(!AlmostEqual("Glasbrook Road", "ул. Петрова", 2), ());
  TEST(!AlmostEqual("MKAD, 600 km", "MKAD, 599 km", 2), ());
  TEST(!AlmostEqual("MKAD, 45-y kilometre", "MKAD, 46", 2), ());
  TEST(!AlmostEqual("ул. Героев Панфиловцев", "ул. Планерная", 2), ());

  std::string small(10, '\0');
  std::string large(1000, '\0');
  TEST(AlmostEqual(small, large, large.length()), ());
  TEST(AlmostEqual(large, small, large.length()), ());
}

UNIT_TEST(EditDistance)
{
  auto testEditDistance = [](std::string const & s1, std::string const & s2, uint32_t expected)
  {
    TEST_EQUAL(strings::EditDistance(s1.begin(), s1.end(), s2.begin(), s2.end()), expected, ());
  };

  testEditDistance("", "wwwww", 5);
  testEditDistance("", "", 0);
  testEditDistance("abc", "def", 3);
  testEditDistance("zzzvvv", "zzzvvv", 0);
  testEditDistance("a", "A", 1);
  testEditDistance("bbbbb", "qbbbbb", 1);
  testEditDistance("aaaaaa", "aaabaaa", 1);
  testEditDistance("aaaab", "aaaac", 1);
  testEditDistance("a spaces test", "aspacestest", 2);

  auto testUniStringEditDistance =
      [](std::string const & utf1, std::string const & utf2, uint32_t expected)
  {
    auto s1 = strings::MakeUniString(utf1);
    auto s2 = strings::MakeUniString(utf2);
    TEST_EQUAL(strings::EditDistance(s1.begin(), s1.end(), s2.begin(), s2.end()), expected, ());
  };

  testUniStringEditDistance("ll", "l1", 1);
  testUniStringEditDistance("\u0132ij", "\u0133IJ", 3);
}

UNIT_TEST(NormalizeDigits)
{
  auto const nd = [](std::string str) -> std::string
  {
    strings::NormalizeDigits(str);
    return str;
  };
  TEST_EQUAL(nd(""), "", ());
  TEST_EQUAL(nd("z12345／／"), "z12345／／", ());
  TEST_EQUAL(nd("a０１9２ "), "a0192 ", ());
  TEST_EQUAL(nd("３４５６７８９"), "3456789", ());
}

UNIT_TEST(NormalizeDigits_UniString)
{
  auto const nd = [](std::string const & utf8) -> std::string
  {
    strings::UniString us = strings::MakeUniString(utf8);
    strings::NormalizeDigits(us);
    return strings::ToUtf8(us);
  };
  TEST_EQUAL(nd(""), "", ());
  TEST_EQUAL(nd("z12345／／"), "z12345／／", ());
  TEST_EQUAL(nd("a０１9２ "), "a0192 ", ());
  TEST_EQUAL(nd("３４５６７８９"), "3456789", ());
}

UNIT_TEST(CSV)
{
  std::vector<std::string> target;
  strings::ParseCSVRow(",Test\\,проверка,0,", ',', target);
  std::vector<std::string> expected({"", "Test\\", "проверка", "0", ""});
  TEST_EQUAL(target, expected, ());
  strings::ParseCSVRow("and there  was none", ' ', target);
  std::vector<std::string> expected2({"and", "there", "", "was", "none"});
  TEST_EQUAL(target, expected2, ());
  strings::ParseCSVRow("", 'q', target);
  std::vector<std::string> expected3;
  TEST_EQUAL(target, expected3, ());
}
