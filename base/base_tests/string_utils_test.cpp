#include "../../testing/testing.hpp"
#include "../string_utils.hpp"

#include "../logging.hpp"

#include "../../std/iomanip.hpp"
#include "../../std/fstream.hpp"
#include "../../std/bind.hpp"
#include "../../std/unordered_map.hpp"

#define UNICODE_TEST_FILE "../../data/CaseFolding.test"

/// internal function in base
namespace strings { UniChar LowerUniChar(UniChar c); }

UNIT_TEST(LowerUniChar)
{
  // load unicode case folding table
  ifstream file(UNICODE_TEST_FILE);
  if (!file.good())
  {
    LOG(LWARNING, ("Can't open unicode test file", UNICODE_TEST_FILE));
    return;
  }

  size_t fCount = 0, cCount = 0;
  typedef unordered_map<strings::UniChar, strings::UniString> mymap;
  mymap m;
  string line;
  while (file.good())
  {
    getline(file, line);
    // strip comments
    size_t const sharp = line.find('#');
    if (sharp != string::npos)
      line.erase(sharp);
    strings::SimpleTokenizer semicolon(line, ";");
    if (!semicolon)
      continue;
    string const capital = *semicolon;
    istringstream stream(capital);
    strings::UniChar uc;
    stream >> hex >> uc;
    ++semicolon;
    string const type = *semicolon;
    if (type == " S" || type == " T")
      continue;
    if (type != " C" && type != " F")
      continue;
    ++semicolon;
    string const outChars = *semicolon;
    strings::UniString us;
    strings::SimpleTokenizer spacer(outChars, " ");
    while (spacer)
    {
      stream.clear();
      stream.str(*spacer);
      strings::UniChar smallCode;
      stream >> hex >> smallCode;
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
  string s;

  s = "THIS_IS_UPPER";
  strings::MakeLowerCase(s);
  TEST_EQUAL(s, "this_is_upper", ());

  s = "THIS_iS_MiXed";
  strings::MakeLowerCase(s);
  TEST_EQUAL(s, "this_is_mixed", ());

  s = "this_is_lower";
  strings::MakeLowerCase(s);
  TEST_EQUAL(s, "this_is_lower", ());

  string const utf8("Hola! 99-\xD0\xA3\xD0\x9F\xD0\xAF\xD0\xA7\xD0\x9A\xD0\x90");
  TEST_EQUAL(strings::MakeLowerCase(utf8),
    "hola! 99-\xD1\x83\xD0\xBF\xD1\x8F\xD1\x87\xD0\xBA\xD0\xB0", ());

  s = "\xc3\x9f"; // es-cet
  strings::MakeLowerCase(s);
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

  s = "labuda";
  TEST(!strings::to_double(s, d), ());
}

UNIT_TEST(to_int)
{
  int i;
  string s;

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

UNIT_TEST(to_uint64)
{
  uint64_t i;
  string s;

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
  string s;

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

UNIT_TEST(GetUniString)
{
  string const s = "Hello, \xD0\x9C\xD0\xB8\xD0\xBD\xD1\x81\xD0\xBA!";
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
  strings::Normalize(us);
  TEST_EQUAL(us, result, ());
}

UNIT_TEST(UniString_Less)
{
  strings::UniString s0 = strings::MakeUniString("Test");
  TEST(!(s0 < s0), ());
  strings::UniString s1 = strings::MakeUniString("Test1");
  TEST(s0 < s1, ());
  strings::UniString s2 = strings::MakeUniString("Tast");
  TEST(s2 < s1, ());
  strings::UniString s3 = strings::MakeUniString("Tas");
  TEST(s3 < s0, ());
  strings::UniString s4 = strings::MakeUniString("Taste");
  TEST(!(s0 < s4), ());
  strings::UniString s5 = strings::MakeUniString("Tist");
  TEST(s0 < s5, ());
  strings::UniString s6 = strings::MakeUniString("Tis");
  TEST(s0 < s6, ());
  strings::UniString s7 = strings::MakeUniString("Tiste");
  TEST(s0 < s7, ());
}

UNIT_TEST(UniStringToUtf8)
{
  char const utf8Text[] = "У нас исходники хранятся в Utf8!";
  strings::UniString uniS = strings::MakeUniString(utf8Text);
  TEST_EQUAL(string(utf8Text), strings::ToUtf8(uniS), ());
}

UNIT_TEST(StartsWith)
{
  using namespace strings;

  TEST(StartsWith(string(), ""), ());

  string s("xyz");
  TEST(StartsWith(s, ""), ());
  TEST(StartsWith(s, "x"), ());
  TEST(StartsWith(s, "xyz"), ());
  TEST(!StartsWith(s, "xyzabc"), ());
  TEST(!StartsWith(s, "ayz"), ());
  TEST(!StartsWith(s, "axy"), ());
}
