#include "../../testing/testing.hpp"

#include "../utf8_string.hpp"

using namespace utf8_string;

bool IsDelimeter(uint32_t symbol)
{
  switch (symbol)
  {
  case ' ':
  case '-':
  case '/':
  case ',':
  case '.':
  case 0x0336:
    return true;
  }
  return false;
}

UNIT_TEST(Utf8_Split)
{
  vector<string> result;
  TEST(!Split("", result, &IsDelimeter), ());
  TEST_EQUAL(result.size(), 0, ());

  TEST(!Split(" - ,. ", result, &IsDelimeter), ());
  TEST_EQUAL(result.size(), 0, ());

  TEST(Split("London - is the capital of babai-city.", result, &IsDelimeter), ());
  TEST_EQUAL(result.size(), 7, ());
  TEST_EQUAL(result[0], "London", ());
  TEST_EQUAL(result[6], "city", ());

  // Доллар подорожал на 500 рублей ̶копеек
  char const * s = "- \xD0\x94\xD0\xBE\xD0\xBB\xD0\xBB\xD0\xB0\xD1\x80\x20\xD0\xBF\xD0\xBE\xD0\xB4\xD0"
  "\xBE\xD1\x80\xD0\xBE\xD0\xB6\xD0\xB0\xD0\xBB\x20\xD0\xBD\xD0\xB0\x20\x35\x30\x30"
  "\x20\xD1\x80\xD1\x83\xD0\xB1\xD0\xBB\xD0\xB5\xD0\xB9\x20\xCC\xB6\xD0\xBA\xD0\xBE"
  "\xD0\xBF\xD0\xB5\xD0\xB5\xD0\xBA -";
  TEST(Split(s, result, &IsDelimeter), ());
  TEST_EQUAL(result.size(), 6, ());
  TEST_EQUAL(result[3], "500", ());
  TEST_EQUAL(result[4], "\xD1\x80\xD1\x83\xD0\xB1\xD0\xBB\xD0\xB5\xD0\xB9", ());
  TEST_EQUAL(result[5], "\xD0\xBA\xD0\xBE\xD0\xBF\xD0\xB5\xD0\xB5\xD0\xBA", ());
}
