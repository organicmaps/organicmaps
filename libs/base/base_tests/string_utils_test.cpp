#include "testing/testing.hpp"

#include "base/logging.hpp"
#include "base/string_utils.hpp"

#include <cfloat>
#include <cmath>
#include <fstream>
#include <limits>
#include <string>
#include <unordered_map>
#include <vector>

#include <sstream>

/// internal function in base
namespace strings
{
UniChar LowerUniChar(UniChar c);
}

UNIT_TEST(LowerUniChar)
{
  // Load unicode case folding table.

  static char constexpr kFile[] = "./data/test_data/CaseFolding.test";
  std::ifstream file(kFile);
  TEST(file.is_open(), (kFile));

  size_t fCount = 0, cCount = 0;
  std::unordered_map<strings::UniChar, strings::UniString> m;
  std::string line;
  while (file.good())
  {
    std::getline(file, line);

    // strip comments
    size_t const sharp = line.find('#');
    if (sharp != std::string::npos)
      line.erase(sharp);
    strings::SimpleTokenizer semicolon(line, ";");
    if (!semicolon)
      continue;

    std::istringstream stream{std::string{*semicolon}};
    uint32_t uc;
    stream >> std::hex >> uc;
    ASSERT(stream, ("Overflow"));
    ++semicolon;

    auto const type = *semicolon;
    if (type == " S" || type == " T")
      continue;
    if (type != " C" && type != " F")
      continue;
    ++semicolon;

    strings::UniString us;
    strings::SimpleTokenizer spacer(*semicolon, " ");
    while (spacer)
    {
      stream.clear();
      stream.str(std::string(*spacer));
      uint32_t smallCode;
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
    default:
    {
      m[uc] = us;
      ++fCount;
      TEST_EQUAL(type, " F", ());
      break;
    }
    }
  }
  LOG(LINFO, ("Loaded", cCount, "common foldings and", fCount, "full foldings"));

  // full range unicode table test
  for (strings::UniChar c = 0; c < 0x11000; ++c)
  {
    auto const found = m.find(c);
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
  using namespace strings;

  TEST_EQUAL(MakeLowerCase("THIS_IS_UPPER"), "this_is_upper", ());
  TEST_EQUAL(MakeLowerCase("THIS_iS_MiXed"), "this_is_mixed", ());
  TEST_EQUAL(MakeLowerCase("this_is_lower"), "this_is_lower", ());

  TEST_EQUAL(MakeLowerCase("Hola! 99-\xD0\xA3\xD0\x9F\xD0\xAF\xD0\xA7\xD0\x9A\xD0\x90"),
                           "hola! 99-\xD1\x83\xD0\xBF\xD1\x8F\xD1\x87\xD0\xBA\xD0\xB0", ());

  // es-cet
  TEST_EQUAL(MakeLowerCase("\xc3\x9f"), "ss", ());

  UniChar const arr[] = {0x397, 0x10B4, 'Z'};
  UniChar const carr[] = {0x3b7, 0x2d14, 'z'};
  UniString const us(&arr[0], &arr[0] + ARRAY_SIZE(arr));
  UniString const cus(&carr[0], &carr[0] + ARRAY_SIZE(carr));
  TEST_EQUAL(cus, MakeLowerCase(us), ());
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

  TEST(!strings::to_double("INF", d), (d));
  TEST(!strings::to_double("NAN", d), (d));
  TEST(!strings::to_double("1.18973e+4932", d), (d));
}

UNIT_TEST(to_float)
{
  float kEps = 1E-30f;
  std::string s;
  float f;

  s = "";
  TEST(!strings::to_float(s, f), ());

  s = "0.123";
  TEST(strings::to_float(s, f), ());
  TEST_ALMOST_EQUAL_ABS(0.123f, f, kEps, ());

  s = "1.";
  TEST(strings::to_float(s, f), ());
  TEST_ALMOST_EQUAL_ABS(1.0f, f, kEps, ());

  s = "0";
  TEST(strings::to_float(s, f), ());
  TEST_ALMOST_EQUAL_ABS(0.f, f, kEps, ());

  s = "5.6843418860808e-14";
  TEST(strings::to_float(s, f), ());
  TEST_ALMOST_EQUAL_ABS(5.6843418860808e-14f, f, kEps, ());

  s = "-2";
  TEST(strings::to_float(s, f), ());
  TEST_ALMOST_EQUAL_ABS(-2.0f, f, kEps, ());

  s = "labuda";
  TEST(!strings::to_float(s, f), ());

  s = "123.456 we don't parse it.";
  TEST(!strings::to_float(s, f), ());

  TEST(!strings::to_float("INF", f), (f));
  TEST(!strings::to_float("NAN", f), (f));
  TEST(!strings::to_float("1.18973e+4932", f), (f));
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

namespace
{
void ToUIntTest(char const * p, bool good = false, uint32_t expected = 0)
{
  std::string s(p);
  std::string_view v(s);

  uint32_t i1, i2, i3;
  TEST(good == strings::to_uint(p, i1), (s));
  TEST(good == strings::to_uint(s, i2), (s));
  TEST(good == strings::to_uint(v, i3), (s));
  if (good)
    TEST(expected == i1 && expected == i2 && expected == i3, (s, i1, i2, i3));
}
} // namespace

UNIT_TEST(to_uint)
{
  ToUIntTest("");
  ToUIntTest("-2");
  ToUIntTest("0", true, 0);
  ToUIntTest("123456789123456789123456789");
  ToUIntTest("labuda");
  ToUIntTest("100", true, 100);
  ToUIntTest("4294967295", true, 0xFFFFFFFF);
  ToUIntTest("4294967296");
  ToUIntTest("2U");
  ToUIntTest("0.");
  ToUIntTest("99,");

  uint32_t i;
  TEST(strings::to_uint("AF", i, 16), ());
  TEST_EQUAL(175, i, ());

  TEST(!strings::to_uint("AXF", i, 16), ());

  uint8_t i8;
  TEST(!strings::to_uint(std::string_view("256"), i8), ());

  uint64_t i64;
  TEST(strings::to_uint(std::string_view("123456789000"), i64), ());
  TEST_EQUAL(i64, 123456789000ULL, ());
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

  s = "AF";
  TEST(strings::to_uint64(s, i, 16), ());
  TEST_EQUAL(175, i, ());

  s = "labuda";
  TEST(!strings::to_uint64(s, i), ());

  s = "-1";
  TEST(strings::to_uint64(s, i), ());
  TEST_EQUAL(18446744073709551615ULL, i, ());
}

UNIT_TEST(to_uint32)
{
  uint32_t i;
  std::string s;

  s = "";
  TEST(!strings::to_uint32(s, i), ());

  s = "0";
  TEST(strings::to_uint32(s, i), ());
  TEST_EQUAL(0, i, ());

  s = "123456789101112";
  TEST(!strings::to_uint32(s, i), ());

  s = "AF";
  TEST(strings::to_uint32(s, i, 16), ());
  TEST_EQUAL(175, i, ());

  s = "labuda";
  TEST(!strings::to_uint32(s, i), ());

  s = "-1";
  TEST(!strings::to_uint32(s, i), ());

  s = "4294967295";
  TEST(strings::to_uint32(s, i), ());
  TEST_EQUAL(4294967295, i, ());

  s = "4294967296";
  TEST(!strings::to_uint32(s, i), ());
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

UNIT_TEST(to_int32)
{
  int32_t i;
  std::string s;

  s = "-24567";
  TEST(strings::to_int32(s, i), ());
  TEST_EQUAL(-24567, i, ());

  s = "0";
  TEST(strings::to_int32(s, i), ());
  TEST_EQUAL(0, i, ());

  s = "12345678911212";
  TEST(!strings::to_int32(s, i), ());

  s = "labuda";
  TEST(!strings::to_int32(s, i), ());

  s = "-1";
  TEST(strings::to_int32(s, i), ());
  TEST_EQUAL(-1, i, ());

  s = "4294967295";
  TEST(!strings::to_int32(s, i), ());

  s = "2147483647";
  TEST(strings::to_int32(s, i), ());
  TEST_EQUAL(2147483647, i, ());

  s = "2147483648";
  TEST(!strings::to_int32(s, i), ());

  s = "-2147483648";
  TEST(strings::to_int32(s, i), ());
  TEST_EQUAL(-2147483648, i, ());

  s = "-2147483649";
  TEST(!strings::to_int32(s, i), ());
}

UNIT_TEST(to_any)
{
  {
    int8_t i;
    std::string s;

    s = "";
    TEST(!strings::to_any(s, i), ());

    s = "1oo";
    TEST(!strings::to_any(s, i), ());

    s = "-129";
    TEST(!strings::to_any(s, i), ());

    s = "128";
    TEST(!strings::to_any(s, i), ());

    s = "-128";
    TEST(strings::to_any(s, i), ());
    TEST_EQUAL(i, -128, ());

    s = "127";
    TEST(strings::to_any(s, i), ());
    TEST_EQUAL(i, 127, ());
  }
  {
    uint8_t i;
    std::string s;

    s = "";
    TEST(!strings::to_any(s, i), ());

    s = "1oo";
    TEST(!strings::to_any(s, i), ());

    s = "-1";
    TEST(!strings::to_any(s, i), ());

    s = "256";
    TEST(!strings::to_any(s, i), ());

    s = "0";
    TEST(strings::to_any(s, i), ());
    TEST_EQUAL(i, 0, ());

    s = "255";
    TEST(strings::to_any(s, i), ());
    TEST_EQUAL(i, 255, ());
  }
  {
    int16_t i;
    std::string s;

    s = "";
    TEST(!strings::to_any(s, i), ());

    s = "1oo";
    TEST(!strings::to_any(s, i), ());

    s = "-32769";
    TEST(!strings::to_any(s, i), ());

    s = "32768";
    TEST(!strings::to_any(s, i), ());

    s = "-32768";
    TEST(strings::to_any(s, i), ());
    TEST_EQUAL(i, -32768, ());

    s = "32767";
    TEST(strings::to_any(s, i), ());
    TEST_EQUAL(i, 32767, ());
  }
  {
    uint16_t i;
    std::string s;

    s = "";
    TEST(!strings::to_any(s, i), ());

    s = "1oo";
    TEST(!strings::to_any(s, i), ());

    s = "-1";
    TEST(!strings::to_any(s, i), ());

    s = "65536";
    TEST(!strings::to_any(s, i), ());

    s = "0";
    TEST(strings::to_any(s, i), ());
    TEST_EQUAL(i, 0, ());

    s = "65535";
    TEST(strings::to_any(s, i), ());
    TEST_EQUAL(i, 65535, ());
  }
  {
    int32_t i;
    std::string s;

    s = "";
    TEST(!strings::to_any(s, i), ());

    s = "1oo";
    TEST(!strings::to_any(s, i), ());

    s = "-2147483649";
    TEST(!strings::to_any(s, i), ());

    s = "2147483648";
    TEST(!strings::to_any(s, i), ());

    s = "-2147483648";
    TEST(strings::to_any(s, i), ());
    TEST_EQUAL(i, -2147483648, ());

    s = "2147483647";
    TEST(strings::to_any(s, i), ());
    TEST_EQUAL(i, 2147483647, ());
  }
  {
    uint32_t i;
    std::string s;

    s = "";
    TEST(!strings::to_any(s, i), ());

    s = "1oo";
    TEST(!strings::to_any(s, i), ());

    s = "-1";
    TEST(!strings::to_any(s, i), ());

    s = "4294967296";
    TEST(!strings::to_any(s, i), ());

    s = "0";
    TEST(strings::to_any(s, i), ());
    TEST_EQUAL(i, 0, ());

    s = "4294967295";
    TEST(strings::to_any(s, i), ());
    TEST_EQUAL(i, 4294967295, ());
  }
  {
    int64_t i;
    std::string s;

    s = "";
    TEST(!strings::to_any(s, i), ());

    s = "1oo";
    TEST(!strings::to_any(s, i), ());

    s = "-9223372036854775809";
    TEST(!strings::to_any(s, i), ());

    s = "9223372036854775808";
    TEST(!strings::to_any(s, i), ());

    s = "-9223372036854775808";
    TEST(strings::to_any(s, i), ());
    TEST_EQUAL(i, std::numeric_limits<int64_t>::min(), ());

    s = "9223372036854775807";
    TEST(strings::to_any(s, i), ());
    TEST_EQUAL(i, 9223372036854775807, ());
  }
  {
    uint64_t i;
    std::string s;

    s = "";
    TEST(!strings::to_any(s, i), ());

    s = "1oo";
    TEST(!strings::to_any(s, i), ());

    s = "-18446744073709551616";
    TEST(!strings::to_any(s, i), ());

    s = "18446744073709551616";
    TEST(!strings::to_any(s, i), ());

    s = "0";
    TEST(strings::to_any(s, i), ());
    TEST_EQUAL(i, 0, ());

    s = "18446744073709551615";
    TEST(strings::to_any(s, i), ());
    TEST_EQUAL(i, 18446744073709551615ULL, ());
  }
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

UNIT_TEST(to_string_width)
{
  TEST_EQUAL(strings::to_string_width(123, 5), "00123", ());
  TEST_EQUAL(strings::to_string_width(99, 3), "099", ());
  TEST_EQUAL(strings::to_string_width(0, 4), "0000", ());
  TEST_EQUAL(strings::to_string_width(-10, 4), "-0010", ());
  TEST_EQUAL(strings::to_string_width(545, 1), "545", ());
  TEST_EQUAL(strings::to_string_width(1073741824, 0), "1073741824", ());
}


struct FunctorTester
{
  size_t & m_index;
  std::vector<std::string> const & m_tokens;

  FunctorTester(size_t & counter, std::vector<std::string> const & tokens)
    : m_index(counter), m_tokens(tokens)
  {
  }

  void operator()(std::string_view s) { TEST_EQUAL(s, m_tokens[m_index++], ()); }
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

void TestIterWithEmptyTokens(std::string const & s, char const * delims,
                             std::vector<std::string> const & tokens)
{
  using namespace strings;
  TokenizeIterator<SimpleDelimiter, std::string::const_iterator, true /* KeepEmptyTokens */>
      it(s.begin(), s.end(), delims);

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
    TestIter(
        "\xD9\x87\xD9\x80 - \xD8\xA7\xD9\x84\xD9\x85\xD9\x88\xD8\xA7\xD9\x81\xD9\x82 "
        "\xD9\x87\xD8\xAC",
        " -\xD9\x87", tokens);
  }

  {
    char const * s[] = {"27.535536", "53.884926", "189"};
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
  TEST_EQUAL(strings::Tokenize<std::string>("acb def ghi", " "), std::vector<std::string>({"acb", "def", "ghi"}), ());
  TEST_EQUAL(strings::Tokenize("  xxx yyy  ", " "), std::vector<std::string_view>({"xxx", "yyy"}), ());
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
    strings::UniChar const s[] = {'H', 'e', 'l', 'l', 'o'};
    strings::UniString us(&s[0], &s[0] + ARRAY_SIZE(s));
    TEST_EQUAL(iter.GetUniString(), us, ());
  }
  ++iter;
  {
    strings::UniChar const s[] = {0x041C, 0x0438, 0x043D, 0x0441, 0x043A};
    strings::UniString us(&s[0], &s[0] + ARRAY_SIZE(s));
    TEST_EQUAL(iter.GetUniString(), us, ());
  }
}

UNIT_TEST(MakeUniString_Smoke)
{
  char const s[] = "Hello!";
  TEST_EQUAL(strings::UniString(&s[0], &s[0] + ARRAY_SIZE(s) - 1), strings::MakeUniString(s), ());
}

UNIT_TEST(Normalize)
{
  strings::UniChar const s[] = {0x1f101, 'H', 0xfef0, 0xfdfc, 0x2150};
  strings::UniString us(&s[0], &s[0] + ARRAY_SIZE(s));
  strings::UniChar const r[] = {0x30,  0x2c,  'H',  0x649,  0x631, 0x6cc,
                                0x627, 0x644, 0x31, 0x2044, 0x37};
  strings::UniString result(&r[0], &r[0] + ARRAY_SIZE(r));
  strings::NormalizeInplace(us);
  TEST_EQUAL(us, result, ());
}

UNIT_TEST(Normalize_Special)
{
  {
    std::string const utf8 = "ąĄćłŁÓŻźŃĘęĆ";
    TEST_EQUAL(strings::ToUtf8(strings::Normalize(strings::MakeUniString(utf8))), "aAclLOZzNEeC",
               ());
  }

  {
    std::string const utf8 = "əüöğ";
    TEST_EQUAL(strings::ToUtf8(strings::Normalize(strings::MakeUniString(utf8))), "əuog", ());
  }
}


UNIT_TEST(Normalize_Arabic)
{
  {
    // Test Arabic-Indic digits normalization
    std::string const utf8       = "٠١٢٣٤٥٦٧٨٩";
    std::string const normalized = "0123456789";
    TEST_EQUAL(strings::ToUtf8(strings::Normalize(strings::MakeUniString(utf8))), normalized, ());
  }

  {
    // Test Extended Arabic-Indic digits normalization
    std::string const utf8       = "۰۱۲۳۴۵۶۷۸۹";
    std::string const normalized = "0123456789";
    TEST_EQUAL(strings::ToUtf8(strings::Normalize(strings::MakeUniString(utf8))), normalized, ());
  }

  {
    // All Arabic Letters (all of these are standalone unicode characters)
    std::string const utf8       = "ء أ إ ا آ ٱ ٲ ٳ ٵ ب ت ة ۃ ث ج ح خ د ذ ر ز س ش ص ط ظ ع غ ف ق ك ل م ن ه ۀ ۂ ؤ ٶ ٷ و ي ى ئ ٸ ے ۓ";
    std::string const normalized = "ء ا ا ا ا ا ا ا ا ب ت ه ه ث ج ح خ د ذ ر ز س ش ص ط ظ ع غ ف ق ك ل م ن ه ه ه و و و و ي ي ي ي ے ے";
    TEST_EQUAL(strings::ToUtf8(strings::Normalize(strings::MakeUniString(utf8))), normalized , ());
  }

  {
    // Test Removing Arabic Diacritics (Tashkeel), we can add multiple diacritics to the same letter
    // Each diacritic is a standalone unicode character
    std::string const utf8       = "هَذِهْٜ تَّجُّرًّبهٌ عَلَىٰ إِزَالْهٍ ألتَشٗكُيٓلُ وَّ ليٙسٝت دقٞيٛقٚه لُغَويًّاً";
    std::string const normalized = "هذه تجربه علي ازاله التشكيل و ليست دقيقه لغويا";
    TEST_EQUAL(strings::ToUtf8(strings::Normalize(strings::MakeUniString(utf8))), normalized , ());
  }

  {
    // Test Removing Arabic Islamic Honorifics
    // These are standalone unicode characters that can be applied to a letter
    std::string const utf8       = "صؐلي  عنؑه رحؒمه رضؓي سؔ طؕ الىؖ زؗ اؘ اؙ ؚا";
    std::string const normalized = "صلي  عنه رحمه رضي س ط الي ز ا ا ا";
    TEST_EQUAL(strings::ToUtf8(strings::Normalize(strings::MakeUniString(utf8))), normalized , ());
  }

  {
    // Test Removing Arabic Quranic Annotations
    // These are standalone unicode characters that can be applied to a letter
    std::string const utf8       = "نۖ بۗ مۘ لۙا جۛ جۚ سۜ ا۟ ا۠ اۡ مۢ  ۣس  نۤ  ك۪  ك۫  ك۬ مۭ";
    std::string const normalized = "ن ب م لا ج ج س ا ا ا م  س  ن  ك  ك  ك م";
    TEST_EQUAL(strings::ToUtf8(strings::Normalize(strings::MakeUniString(utf8))), normalized , ());
  }

  {
    // Tests Arabic Tatweel (Kashida) normalization
    // This character is used to elongate text in Arabic script, (used in justifing/aligning text)
    std::string const utf8       = "اميـــــن";
    std::string const normalized =      "امين";
    TEST_EQUAL(strings::ToUtf8(strings::Normalize(strings::MakeUniString(utf8))), normalized , ());
  }

  {
    // Tests Arabic Comma normalization
    std::string const utf8       = "،";
    std::string const normalized = ",";
    TEST_EQUAL(strings::ToUtf8(strings::Normalize(strings::MakeUniString(utf8))), normalized , ());
  }

}

UNIT_TEST(UniStringToUtf8)
{
  char constexpr utf8Text[] = "У нас исходники хранятся в Utf8!";
  auto const uniS = strings::MakeUniString(utf8Text);
  TEST_EQUAL(std::string(utf8Text), strings::ToUtf8(uniS), ());
}

UNIT_TEST(UniStringToUtf16)
{
  std::string_view constexpr utf8sv = "Текст";
  static std::u16string_view constexpr utf16sv = u"Текст";
  TEST_EQUAL(utf16sv, strings::ToUtf16(utf8sv), ());
}

UNIT_TEST(StartsWith)
{
  using namespace strings;

  UniString const us = MakeUniString("xyz");
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

  auto const s = MakeUniString("zюя");
  TEST(EndsWith(s, MakeUniString("")), ());
  TEST(EndsWith(s, MakeUniString("я")), ());
  TEST(EndsWith(s, MakeUniString("юя")), ());
  TEST(EndsWith(s, MakeUniString("zюя")), ());
  TEST(!EndsWith(s, MakeUniString("абвгдzюя")), ());
  TEST(!EndsWith(s, MakeUniString("aюя")), ());
  TEST(!EndsWith(s, MakeUniString("1zюя")), ());
}

UNIT_TEST(EatPrefix_EatSuffix)
{
  // <original string, prefix/suffix to cut, success, string after cutting>
  std::vector<std::tuple<std::string, std::string, bool, std::string>> kPrefixTestcases = {
    {"abc", "a", true, "bc"},
    {"abc", "b", false, "abc"},
    {"abc", "", true, "abc"},
    {"abc", "abc", true, ""},
    {"abc", "abc00", false, "abc"},
    {"", "", true, ""},
    {"абв", "а", true, "бв"},
    {"абв", "б", false, "абв"},
    {"字符串", "字", true, "符串"},
  };

  std::vector<std::tuple<std::string, std::string, bool, std::string>> kSuffixTestcases = {
    {"abc", "c", true, "ab"},
    {"abc", "b", false, "abc"},
    {"abc", "", true, "abc"},
    {"abc", "abc", true, ""},
    {"abc", "00abc", false, "abc"},
    {"", "", true, ""},
    {"абв", "в", true, "аб"},
    {"абв", "б", false, "абв"},
    {"字符串", "串", true, "字符"},
  };

  for (auto const & [original, toCut, success, afterCutting] : kPrefixTestcases)
  {
    auto s = original;
    TEST_EQUAL(strings::EatPrefix(s, toCut), success, (original, toCut));
    TEST_EQUAL(s, afterCutting, ());
  }

  for (auto const & [original, toCut, success, afterCutting] : kSuffixTestcases)
  {
    auto s = original;
    TEST_EQUAL(strings::EatSuffix(s, toCut), success, (original, toCut));
    TEST_EQUAL(s, afterCutting, ());
  }
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

UNIT_TEST(IsASCIINumericTest)
{
  TEST(strings::IsASCIINumeric("0"), ());
  TEST(strings::IsASCIINumeric("1"), ());
  TEST(strings::IsASCIINumeric("10"), ());
  TEST(strings::IsASCIINumeric("01"), ());
  TEST(strings::IsASCIINumeric("00"), ());

  TEST(!strings::IsASCIINumeric(""), ());
  TEST(!strings::IsASCIINumeric(" "), ());
  TEST(!strings::IsASCIINumeric(" 9"), ());
  TEST(!strings::IsASCIINumeric("9 "), ());
  TEST(!strings::IsASCIINumeric("+3"), ());
  TEST(!strings::IsASCIINumeric("-2"), ());
  TEST(!strings::IsASCIINumeric("0x09"), ());
  TEST(!strings::IsASCIINumeric("0.1"), ());
}

UNIT_TEST(CountNormLowerSymbols)
{
  char const * strs[] = {"æüßs",
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
                         "Überstraße"};

  char const * low_strs[] = {"æusss",
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
                             "uberstras"};

  size_t const results[] = {4, 3, 8, 0, 3, 10, 0, 0, 0, 0, 0, 9};

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
  auto testEditDistance = [](std::string const & s1, std::string const & s2, uint32_t expected) {
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

  auto testUniStringEditDistance = [](std::string const & utf1, std::string const & utf2,
                                      uint32_t expected) {
    auto s1 = strings::MakeUniString(utf1);
    auto s2 = strings::MakeUniString(utf2);
    TEST_EQUAL(strings::EditDistance(s1.begin(), s1.end(), s2.begin(), s2.end()), expected, ());
  };

  testUniStringEditDistance("ll", "l1", 1);
  testUniStringEditDistance("\u0132ij", "\u0133IJ", 3);
}

UNIT_TEST(NormalizeDigits)
{
  auto const nd = [](std::string str) -> std::string {
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
  auto const nd = [](std::string const & utf8) -> std::string {
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

UNIT_TEST(UniString_Replace)
{
  std::vector<std::string> const testStrings = {
      "longlong",
      "ss",
      "samesize",
      "sometext longlong",
      "sometext ss",
      "sometext samesize",
      "longlong sometext",
      "ss sometext",
      "samesize sometext",
      "longlong ss samesize",
      "sometext longlong sometext ss samesize sometext",
      "длинная строка",
      "к с",
      "такая же строка",
      "sometext длинная строка",
      "sometext к с",
      "sometext такая же строка",
      "длинная строка sometext",
      "к с sometext",
      "samesize sometext",
      "длинная строка к с samesize",
      "sometext длинная строка sometext к с такая же строка sometext"};

  std::vector<std::pair<std::string, std::string>> const replacements = {
      {"longlong", "ll"},         {"ss", "shortshort"},
      {"samesize", "sizesame"},   {"длинная строка", "д с"},
      {"к с", "короткая строка"}, {"такая же строка", "строка такая же"}};

  for (auto testString : testStrings)
  {
    auto uniStr = strings::MakeUniString(testString);
    for (auto const & r : replacements)
    {
      {
        auto const toReplace = strings::MakeUniString(r.first);
        auto const replacement = strings::MakeUniString(r.second);
        auto & str = uniStr;
        auto start = std::search(str.begin(), str.end(), toReplace.begin(), toReplace.end());
        if (start != str.end())
        {
          auto end = start + toReplace.size();
          str.Replace(start, end, replacement.begin(), replacement.end());
        }
      }
      {
        auto const toReplace = r.first;
        auto const replacement = r.second;
        auto & str = testString;
        auto start = std::search(str.begin(), str.end(), toReplace.begin(), toReplace.end());
        if (start != str.end())
        {
          auto end = start + toReplace.size();
          str.replace(start, end, replacement.begin(), replacement.end());
        }
      }
    }
    TEST_EQUAL(testString, ToUtf8(uniStr), ());
  }
}

namespace
{
void TestTrim(std::string const & str, std::string const & expected)
{
  std::string copy = str;
  std::string_view sv = str;

  strings::Trim(copy);
  strings::Trim(sv);

  TEST(expected == copy && expected == sv, (str));
}
} // namespace

UNIT_TEST(Trim)
{
  std::string str = "string";

  TestTrim("", "");
  TestTrim("  ", "");
  TestTrim(str, str);
  TestTrim("  " + str, str);
  TestTrim(str + "  ", str);
  TestTrim("  " + str + "  ", str);

  strings::Trim(str, "tsgn");
  TEST_EQUAL(str, "ri", ());

  std::string_view v = "\"abc ";
  strings::Trim(v, "\" ");
  TEST_EQUAL(v, "abc", ());

  v = "aaa";
  strings::Trim(v, "a");
  TEST(v.empty(), ());
}

UNIT_TEST(ToLower_ToUpper)
{
  std::string s = "AbC0;9z";

  strings::AsciiToLower(s);
  TEST_EQUAL(s, "abc0;9z", ());

  strings::AsciiToUpper(s);
  TEST_EQUAL(s, "ABC0;9Z", ());
}
