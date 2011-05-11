#include "../../testing/testing.hpp"

#include "../multilang_utf8_string.hpp"

#include "../../3party/utfcpp/source/utf8.h"


namespace
{
  struct lang_string
  {
    char const * m_lang;
    char const * m_str;
  };

  void TestMultilangString(lang_string const * arr, size_t count)
  {
    StringUtf8Multilang s;

    for (size_t i = 0; i < count; ++i)
    {
      string src(arr[i].m_str);
      TEST(utf8::is_valid(src.begin(), src.end()), ());

      s.AddString(arr[i].m_lang, src);

      string comp;
      TEST(s.GetString(arr[i].m_lang, comp), ());
      TEST_EQUAL(src, comp, ());
    }

    for (size_t i = 0; i < count; ++i)
    {
      string comp;
      TEST(s.GetString(arr[i].m_lang, comp), ());
      TEST_EQUAL(arr[i].m_str, comp, ());
    }

    string test;
    TEST(!s.GetString("xxx", test), ());
  }
}

UNIT_TEST(MultilangString_Smoke)
{
  StringUtf8Multilang s;

  lang_string arr[] = { {"default", "default"},
                        {"en", "abcd"},
                        {"ru", "\xD0\xA0\xD0\xB0\xD1\x88\xD0\xBA\xD0\xB0"},
                        {"be", "\xE2\x82\xAC\xF0\xA4\xAD\xA2"} };

  TestMultilangString(arr, ARRAY_SIZE(arr));
}
