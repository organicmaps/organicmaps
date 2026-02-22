#include "testing/testing.hpp"

#include "base/logging.hpp"
#include "base/string_utils.hpp"

#include <cfloat>
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

namespace string_utils_test
{
using strings::Normalize, strings::MakeUniString;
using strings::UniChar, strings::UniString, strings::MakeLowerCase, strings::LowerUniChar;

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

  // full range unicode table test (including SMP characters)
  for (strings::UniChar c = 0; c < 0x1F000; ++c)
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

  TEST_EQUAL(MakeLowerCase("Hola! 99-УПЯЧКА"), "hola! 99-упячка", ());

  // es-cet
  TEST_EQUAL(MakeLowerCase("ß"), "ss", ());

  UniString const us(U"ΗႴZ");
  UniString const cus(U"ηⴔz");
  TEST_EQUAL(cus, MakeLowerCase(us), ());

  // New BMP mappings added in CaseFolding 18.0
  // Greek Capital Letter Yot
  TEST_EQUAL(LowerUniChar(U'Ϳ'), U'ϳ', ());
  // Cyrillic Capital Letter En With Left Hook
  TEST_EQUAL(LowerUniChar(U'Ԩ'), U'ԩ', ());
  // Georgian Capital Letter Yn / Aen
  TEST_EQUAL(LowerUniChar(U'Ⴧ'), U'ⴧ', ());
  TEST_EQUAL(LowerUniChar(U'Ⴭ'), U'ⴭ', ());
  // Cherokee Small Letter YE (maps to uppercase Cherokee)
  TEST_EQUAL(LowerUniChar(U'ᏸ'), U'Ᏸ', ());
  // Cyrillic Small Letter Rounded VE -> common VE
  TEST_EQUAL(LowerUniChar(U'ᲀ'), U'в', ());
  // Cyrillic Small Letter Unblended UK
  TEST_EQUAL(LowerUniChar(U'ᲈ'), U'ꙋ', ());
  // Cyrillic Capital Letter TJE
  TEST_EQUAL(LowerUniChar(U'\x1C89'), U'\x1C8A', ());
  // Georgian Mtavruli Capital Letter AN
  TEST_EQUAL(LowerUniChar(U'Ა'), U'ა', ());
  // Glagolitic Capital Letter Caudate Chrivi
  TEST_EQUAL(LowerUniChar(U'Ⱟ'), U'ⱟ', ());
  // Coptic Capital Letter Bohairic Khei
  TEST_EQUAL(LowerUniChar(U'Ⳳ'), U'ⳳ', ());
  // Latin Capital Letter H With Hook
  TEST_EQUAL(LowerUniChar(U'Ɦ'), U'ɦ', ());
  // Latin Capital Letter Chi
  TEST_EQUAL(LowerUniChar(U'Ꭓ'), U'ꭓ', ());
  // Latin Capital Letter C With Palatal Hook
  TEST_EQUAL(LowerUniChar(U'Ꞔ'), U'ꞔ', ());
  // Latin Capital Letter Lambda With Stroke
  TEST_EQUAL(LowerUniChar(U'Ƛ'), U'ƛ', ());
  // Cherokee Small Letter A (AB range)
  TEST_EQUAL(LowerUniChar(U'ꭰ'), U'Ꭰ', ());
  TEST_EQUAL(LowerUniChar(U'ꮿ'), U'Ꮿ', ());

  // SMP (Supplementary Multilingual Plane) character tests
  {
    // Osage Capital Letter A
    TEST_EQUAL(MakeLowerCase(UniString(U"\U000104B0")), UniString(U"\U000104D8"), ());
  }
  {
    // Vithkuqi Capital Letter A
    TEST_EQUAL(MakeLowerCase(UniString(U"\U00010570")), UniString(U"\U00010597"), ());
    // Vithkuqi gap: U+1057B should be unchanged
    TEST_EQUAL(LowerUniChar(U'\U0001057B'), U'\U0001057B', ());
  }
  {
    // Old Hungarian Capital Letter A
    TEST_EQUAL(MakeLowerCase(UniString(U"\U00010C80")), UniString(U"\U00010CC0"), ());
  }
  {
    // Warang Citi Capital Letter NGAA
    TEST_EQUAL(MakeLowerCase(UniString(U"\U000118A0")), UniString(U"\U000118C0"), ());
  }
  {
    // Medefaidrin Capital Letter M
    TEST_EQUAL(MakeLowerCase(UniString(U"\U00016E40")), UniString(U"\U00016E60"), ());
  }
  {
    // Adlam Capital Letter ALIF
    TEST_EQUAL(MakeLowerCase(UniString(U"\U0001E900")), UniString(U"\U0001E922"), ());
  }
  {
    // Latin Small Ligature Long S With Descender S -> "ss" (full case folding)
    TEST_EQUAL(MakeLowerCase(UniString(U"\U0001DF95")), UniString(U"ss"), ());
  }
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
}  // namespace

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

  TEST(strings::to_uint(std::string_view("C8"), i, 16), ());
  TEST_EQUAL(200, i, ());

  TEST(!strings::to_uint("AXF", i, 16), ());
  TEST(!strings::to_uint(std::string_view("AXF"), i, 16), ());

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
  TEST(!strings::to_uint(s, i), ());

  s = "0";
  TEST(strings::to_uint(s, i), ());
  TEST_EQUAL(0, i, ());

  s = "123456789101112";
  TEST(!strings::to_uint(s, i), ());

  s = "AF";
  TEST(strings::to_uint(s, i, 16), ());
  TEST_EQUAL(175, i, ());

  s = "labuda";
  TEST(!strings::to_uint(s, i), ());

  s = "-1";
  TEST(!strings::to_uint(s, i), ());

  s = "4294967295";
  TEST(strings::to_uint(s, i), ());
  TEST_EQUAL(4294967295, i, ());

  s = "4294967296";
  TEST(!strings::to_uint(s, i), ());
}

UNIT_TEST(to_int64)
{
  int64_t i;
  std::string s;

  s = "-24567";
  TEST(strings::to_int(s, i), ());
  TEST_EQUAL(-24567, i, ());

  s = "0";
  TEST(strings::to_int(s, i), ());
  TEST_EQUAL(0, i, ());

  s = "12345678911212";
  TEST(strings::to_int(s, i), ());
  TEST_EQUAL(12345678911212LL, i, ());

  s = "labuda";
  TEST(!strings::to_int(s, i), ());
}

UNIT_TEST(to_int32)
{
  int32_t i;
  std::string s;

  s = "-24567";
  TEST(strings::to_int(s, i), ());
  TEST_EQUAL(-24567, i, ());

  s = "0";
  TEST(strings::to_int(s, i), ());
  TEST_EQUAL(0, i, ());

  s = "12345678911212";
  TEST(!strings::to_int(s, i), ());

  s = "labuda";
  TEST(!strings::to_int(s, i), ());

  s = "-1";
  TEST(strings::to_int(s, i), ());
  TEST_EQUAL(-1, i, ());

  s = "4294967295";
  TEST(!strings::to_int(s, i), ());

  s = "2147483647";
  TEST(strings::to_int(s, i), ());
  TEST_EQUAL(2147483647, i, ());

  s = "2147483648";
  TEST(!strings::to_int(s, i), ());

  s = "-2147483648";
  TEST(strings::to_int(s, i), ());
  TEST_EQUAL(-2147483648, i, ());

  s = "-2147483649";
  TEST(!strings::to_int(s, i), ());
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

  FunctorTester(size_t & counter, std::vector<std::string> const & tokens) : m_index(counter), m_tokens(tokens) {}

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

void TestIterWithEmptyTokens(std::string const & s, char const * delims, std::vector<std::string> const & tokens)
{
  using namespace strings;
  TokenizeIterator<SimpleDelimiter, std::string::const_iterator, true /* KeepEmptyTokens */> it(s.begin(), s.end(),
                                                                                                delims);

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
    char const * s[] = {"ـ", "الموافق", "ج"};
    tokens.assign(&s[0], &s[0] + ARRAY_SIZE(s));
    TestIter("هـ - الموافق هج", " -ه", tokens);
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
  TEST_EQUAL(strings::LastUniChar("Hello"), U'o', ());
  TEST_EQUAL(strings::LastUniChar(" А"), U'А', ());
}

UNIT_TEST(GetUniString)
{
  std::string const s = "Hello, Минск!";
  strings::SimpleTokenizer iter(s, ", !");
  {
    UniString us(U"Hello");
    TEST_EQUAL(iter.GetUniString(), us, ());
  }
  ++iter;
  {
    UniString us(U"Минск");
    TEST_EQUAL(iter.GetUniString(), us, ());
  }
}

UNIT_TEST(MakeUniString_Smoke)
{
  char const s[] = "Hello!";
  TEST_EQUAL(strings::UniString(&s[0], &s[0] + ARRAY_SIZE(s) - 1), MakeUniString(s), ());
}

UNIT_TEST(Normalize)
{
  UniString us(U"\U0001F101Hﻰ﷼⅐");
  UniString result(U"0,Hىریال1⁄7");
  strings::NormalizeInplace(us);
  TEST_EQUAL(us, result, ());
}

UNIT_TEST(Normalize_Special)
{
  TEST_EQUAL(ToUtf8(Normalize(MakeUniString("ąĄćłŁÓŻźŃĘęĆ"))), "aAclLOZzNEeC", ());
  TEST_EQUAL(ToUtf8(Normalize(MakeUniString("əüöğ"))), "əuog", ());
}

UNIT_TEST(Normalize_LowerCase_Danish)
{
  UniString const us(U"Æ, æ");
  TEST_EQUAL(ToUtf8(strings::Normalize(us)), "Æ, æ", ("Not normalized by NKFD as expected"));
  TEST_EQUAL(ToUtf8(strings::MakeLowerCase(us)), "æ, æ", ());
}

UNIT_TEST(Normalize_French)
{
  UniString const us(U"Œ, œ");
  TEST_EQUAL(ToUtf8(strings::Normalize(us)), "Œ, œ", ("Not normalized by NKFD as expected"));
  TEST_EQUAL(ToUtf8(strings::MakeLowerCase(us)), "œ, œ", ());
}

UNIT_TEST(Normalize_Turkish)
{
  UniString const us(U"Iiİı");
  TEST_EQUAL(ToUtf8(strings::Normalize(us)), "IiIı", ());
  TEST_EQUAL(ToUtf8(strings::MakeLowerCase(us)), "iii̇ı", ());
}

UNIT_TEST(Normalize_Arabic)
{
  // Test Arabic-Indic digits normalization
  TEST_EQUAL(ToUtf8(Normalize(MakeUniString("٠١٢٣٤٥٦٧٨٩"))), "0123456789", ());

  // Test Extended Arabic-Indic digits normalization
  TEST_EQUAL(ToUtf8(Normalize(MakeUniString("۰۱۲۳۴۵۶۷۸۹"))), "0123456789", ());

  // All Arabic Letters (all of these are standalone unicode characters)
  TEST_EQUAL(ToUtf8(Normalize(MakeUniString(
                 "ء أ إ ا آ ٱ ٲ ٳ ٵ ب ت ة ۃ ث ج ح خ د ذ ر ز س ش ص ط ظ ع غ ف ق ك ل م ن ه ۀ ۂ ؤ ٶ ٷ و ي ى ئ ٸ ے ۓ"))),
             "ء ا ا ا ا ا ا ا ا ب ت ه ه ث ج ح خ د ذ ر ز س ش ص ط ظ ع غ ف ق ك ل م ن ه ه ه و و و و ي ي ي ي ے ے", ());

  // Test Removing Arabic Diacritics (Tashkeel), we can add multiple diacritics to the same letter
  // Each diacritic is a standalone unicode character
  TEST_EQUAL(ToUtf8(Normalize(MakeUniString("هَذِهْٜ تَّجُّرًّبهٌ عَلَىٰ إِزَالْهٍ ألتَشٗكُيٓلُ وَّ ليٙسٝت دقٞيٛقٚه لُغَويًّاً"))),
             "هذه تجربه علي ازاله التشكيل و ليست دقيقه لغويا", ());

  // Test Removing Arabic Islamic Honorifics
  // These are standalone unicode characters that can be applied to a letter
  TEST_EQUAL(ToUtf8(Normalize(MakeUniString("صؐلي  عنؑه رحؒمه رضؓي سؔ طؕ الىؖ زؗ اؘ اؙ ؚا"))), "صلي  عنه رحمه رضي س ط الي ز ا ا ا",
             ());

  // Test Removing Arabic Quranic Annotations
  // These are standalone unicode characters that can be applied to a letter
  TEST_EQUAL(ToUtf8(Normalize(MakeUniString("نۖ بۗ مۘ لۙا جۛ جۚ سۜ ا۟ ا۠ اۡ مۢ  ۣس  نۤ  ك۪  ك۫  ك۬ مۭ"))),
             "ن ب م لا ج ج س ا ا ا م  س  ن  ك  ك  ك م", ());

  // Tests Arabic Tatweel (Kashida) normalization
  // This character is used to elongate text in Arabic script, (used in justifing/aligning text)
  TEST_EQUAL(ToUtf8(Normalize(MakeUniString("اميـــــن"))), "امين", ());

  // Tests Arabic Comma normalization
  TEST_EQUAL(ToUtf8(Normalize(MakeUniString("،"))), ",", ());
}

UNIT_TEST(Normalize_NewerDigits)
{
  {
    // Adlam digits (Unicode 9.0, U+1E950-1E959)
    // strings::UniChar const s[] = {0x1E950, 0x1E955, 0x1E959};
    // strings::UniString us(std::begin(s), std::end(s));
    UniString us(U"\U0001E950\U0001E955\U0001E959");
    strings::NormalizeInplace(us);
    TEST_EQUAL(ToUtf8(us), "059", ());
  }

  {
    // Hanifi Rohingya digits (Unicode 11.0, U+10D30-10D39)
    UniString us(U"\U00010D31\U00010D32\U00010D33");
    strings::NormalizeInplace(us);
    TEST_EQUAL(ToUtf8(us), "123", ());
  }

  {
    // Kawi digits (Unicode 15.0, U+11F50-11F59)
    UniString us(U"\U00011F54\U00011F52\U00011F50");
    strings::NormalizeInplace(us);
    TEST_EQUAL(ToUtf8(us), "420", ());
  }

  {
    // Nag Mundari digits (Unicode 15.0, U+1E4F0-1E4F9)
    UniString us(U"\U0001E4F7\U0001E4F8\U0001E4F9");
    strings::NormalizeInplace(us);
    TEST_EQUAL(ToUtf8(us), "789", ());
  }
}

UNIT_TEST(Normalize_MathAlphanumeric)
{
  // Mathematical Alphanumeric Symbols (Unicode 3.1, Plane 1)
  // Bold A -> A, Bold a -> a, Bold 0 -> 0
  UniString us(U"𝐀𝐚𝟎");
  strings::NormalizeInplace(us);
  TEST_EQUAL(ToUtf8(us), "Aa0", ());
}

UNIT_TEST(Normalize_EnclosedAndCompat)
{
  {
    // SQUARE DJ (Unicode 6.0, U+1F190) -> "DJ"
    UniString us(U"\U0001F190");
    strings::NormalizeInplace(us);
    TEST_EQUAL(ToUtf8(us), "DJ", ());
  }

  {
    // SQUARE ERA NAME REIWA (Unicode 12.1, U+32FF) -> CJK 令和
    UniString us(U"㋿");
    strings::NormalizeInplace(us);
    UniString exp(U"令和");
    TEST_EQUAL(us, exp, ());
  }

  {
    // VULGAR FRACTION ZERO THIRDS (Unicode 6.1, U+2189) -> "0⁄3"
    UniString us(U"↉");
    strings::NormalizeInplace(us);
    UniString exp(U"0⁄3");
    TEST_EQUAL(us, exp, ());
  }

  {
    // CJK COMPATIBILITY IDEOGRAPH-2F800 丽 (Plane 2, U+2F800) -> U+4E3D 丽
    UniString us(U"\U0002F800");
    strings::NormalizeInplace(us);
    TEST_EQUAL(us[0], U'\U00004E3D', ());
  }
}

UNIT_TEST(Normalize_SuperscriptSubscript)
{
  // Superscript and subscript characters -> ASCII equivalents
  // ⁰ⁱ⁴₀ₐ -> "0i40a" (superscript 0, superscript i, superscript 4, subscript 0, subscript a)
  UniString us(U"⁰ⁱ⁴₀ₐ");
  strings::NormalizeInplace(us);
  TEST_EQUAL(ToUtf8(us), "0i40a", ());
}

UNIT_TEST(NormalizeChar)
{
  {
    // ASCII passthrough
    strings::UniChar buf[strings::kMaxNormalizedLen];
    size_t const n = strings::NormalizeChar(U'A', buf);
    TEST_EQUAL(n, 1, ());
    TEST_EQUAL(buf[0], U'A', ());
  }

  {
    // Single-char mapping: Polish ł (U+0142) -> l
    strings::UniChar buf[strings::kMaxNormalizedLen];
    size_t const n = strings::NormalizeChar(U'ł', buf);
    TEST_EQUAL(n, 1, ());
    TEST_EQUAL(buf[0], U'l', ());
  }

  {
    // Strip: Arabic Tatweel (U+0640) -> removed
    strings::UniChar buf[strings::kMaxNormalizedLen];
    size_t const n = strings::NormalizeChar(U'ـ', buf);
    TEST_EQUAL(n, 0, ());
  }

  {
    // Multi-char: DOUBLE QUESTION MARK (U+2047) -> "??"
    strings::UniChar buf[strings::kMaxNormalizedLen];
    size_t const n = strings::NormalizeChar(U'⁇', buf);
    TEST_EQUAL(n, 2, ());
    TEST_EQUAL(buf[0], U'?', ());
    TEST_EQUAL(buf[1], U'?', ());
  }

  {
    // Identity: character with no mapping (e.g. U+4E00, CJK ideograph 一)
    strings::UniChar buf[strings::kMaxNormalizedLen];
    size_t const n = strings::NormalizeChar(U'一', buf);
    TEST_EQUAL(n, 1, ());
    TEST_EQUAL(buf[0], U'一', ());
  }
}

UNIT_TEST(UniStringToUtf8)
{
  char constexpr utf8Text[] = "У нас исходники хранятся в Utf8!";
  auto const uniS = MakeUniString(utf8Text);
  TEST_EQUAL(std::string(utf8Text), ToUtf8(uniS), ());
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
      {"abc", "a", true, "bc"}, {"abc", "b", false, "abc"},     {"abc", "", true, "abc"},
      {"abc", "abc", true, ""}, {"abc", "abc00", false, "abc"}, {"", "", true, ""},
      {"абв", "а", true, "бв"}, {"абв", "б", false, "абв"},     {"字符串", "字", true, "符串"},
  };

  std::vector<std::tuple<std::string, std::string, bool, std::string>> kSuffixTestcases = {
      {"abc", "c", true, "ab"}, {"abc", "b", false, "abc"},     {"abc", "", true, "abc"},
      {"abc", "abc", true, ""}, {"abc", "00abc", false, "abc"}, {"", "", true, ""},
      {"абв", "в", true, "аб"}, {"абв", "б", false, "абв"},     {"字符串", "串", true, "字符"},
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
  v.emplace_back(U"");
  v.emplace_back(U"Tes");
  v.emplace_back(U"Test");
  v.emplace_back(U"TestT");
  v.emplace_back(U"TestTestTest");
  v.emplace_back(U"To");
  v.emplace_back(U"To!");
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
    strings::UniString source = MakeUniString(strs[i]);
    strings::UniString result = MakeUniString(low_strs[i]);

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
  { TEST_EQUAL(strings::EditDistance(s1.begin(), s1.end(), s2.begin(), s2.end()), expected, ()); };

  testEditDistance("", "wwwww", 5);
  testEditDistance("", "", 0);
  testEditDistance("abc", "def", 3);
  testEditDistance("zzzvvv", "zzzvvv", 0);
  testEditDistance("a", "A", 1);
  testEditDistance("bbbbb", "qbbbbb", 1);
  testEditDistance("aaaaaa", "aaabaaa", 1);
  testEditDistance("aaaab", "aaaac", 1);
  testEditDistance("a spaces test", "aspacestest", 2);

  auto testUniStringEditDistance = [](std::string const & utf1, std::string const & utf2, uint32_t expected)
  {
    auto s1 = MakeUniString(utf1);
    auto s2 = MakeUniString(utf2);
    TEST_EQUAL(strings::EditDistance(s1.begin(), s1.end(), s2.begin(), s2.end()), expected, ());
  };

  testUniStringEditDistance("ll", "l1", 1);
  testUniStringEditDistance("Ĳij", "ĳIJ", 3);
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
    strings::UniString us = MakeUniString(utf8);
    strings::NormalizeDigits(us);
    return ToUtf8(us);
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
  std::vector<std::string> const testStrings = {"longlong",
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
      {"longlong", "ll"},        {"ss", "shortshort"},       {"samesize", "sizesame"},
      {"длинная строка", "д с"}, {"к с", "короткая строка"}, {"такая же строка", "строка такая же"}};

  for (auto testString : testStrings)
  {
    auto uniStr = MakeUniString(testString);
    for (auto const & r : replacements)
    {
      {
        auto const toReplace = MakeUniString(r.first);
        auto const replacement = MakeUniString(r.second);
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
}  // namespace

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

}  // namespace string_utils_test
