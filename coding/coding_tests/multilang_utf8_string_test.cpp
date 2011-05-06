#include "../../testing/testing.hpp"

#include "../multilang_utf8_string.hpp"
#include "../strutil.hpp"


namespace
{
  struct lang_string
  {
    char const * m_lang;
    wchar_t const * m_str;
  };

  void TestMultilangString(lang_string const * arr, size_t count)
  {
    StringUtf8Multilang s;

    for (size_t i = 0; i < count; ++i)
    {
      string const utf8 = ToUtf8(arr[i].m_str);
      s.AddString(arr[i].m_lang, utf8);

      string comp;
      TEST(s.GetString(arr[i].m_lang, comp), ());
      TEST_EQUAL(utf8, comp, ());
    }

    for (size_t i = 0; i < count; ++i)
    {
      string const utf8 = ToUtf8(arr[i].m_str);

      string comp;
      TEST(s.GetString(arr[i].m_lang, comp), ());
      TEST_EQUAL(utf8, comp, ());
    }

    string test;
    TEST(!s.GetString("xxx", test), ());
  }
}

UNIT_TEST(MultilangString_Smoke)
{
  StringUtf8Multilang s;

  lang_string arr[] = { {"en", L"abcd"}, {"ru", L"éóõ¸"}, {"be", L"öìîê"} };

  TestMultilangString(arr, ARRAY_SIZE(arr));
}
