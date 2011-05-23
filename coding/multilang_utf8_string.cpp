#include "multilang_utf8_string.hpp"

#include "../defines.hpp"

char StringUtf8Multilang::GetLangIndex(string const & lang)
{
  static char const * arr[] = { "default",
                         "en", "ja", "fr", "ko_rm", "ar", "de", "ru", "sv", "zh", "fi",
                         "ko", "ka", "he", "be", "nl", "ga", "ja_rm", "el", "it", "es",
                         "th", "zh_pinyin", "ca", "cy", "hu", "hsb", "sr", "fa", "eu", "pl",
                         "br", "uk", "sl", "ro", "sq", "am", "fy", "gd", "cs", "sk",
                         "af", "hr", "hy", "tr", "kn", "pt", "lt", "lb", "bg", "eo",
                         "kk", "la", "et", "vi", "mn", "mk", "lv", "fur", "gsw", "ja_kana",
                         "is", "hi", "ku" };

  STATIC_ASSERT(ARRAY_SIZE(arr) == MAX_SUPPORTED_LANGUAGES);

  for (size_t i = 0; i < ARRAY_SIZE(arr); ++i)
    if (lang == arr[i])
      return static_cast<char>(i);

  return -1;
}

size_t StringUtf8Multilang::GetNextIndex(size_t i) const
{
  ++i;
  size_t const sz = m_s.size();

  while (i < sz && (m_s[i] & 0xC0) != 0x80)
  {
    if ((m_s[i] & 0x80) == 0)
      i += 1;
    else if ((m_s[i] & 0xFE) == 0xFE)
      i += 7;
    else if ((m_s[i] & 0xFC) == 0xFC)
      i += 6;
    else if ((m_s[i] & 0xF8) == 0xF8)
      i += 5;
    else if ((m_s[i] & 0xF0) == 0xF0)
      i += 4;
    else if ((m_s[i] & 0xE0) == 0xE0)
      i += 3;
    else if ((m_s[i] & 0xC0) == 0xC0)
      i += 2;
  }

  return i;
}

void StringUtf8Multilang::AddString(char lang, string const & utf8s)
{
  m_s.push_back(lang | 0x80);
  m_s.insert(m_s.end(), utf8s.begin(), utf8s.end());
}

bool StringUtf8Multilang::GetString(char lang, string & utf8s) const
{
  size_t i = 0;
  size_t const sz = m_s.size();

  while (i < sz)
  {
    size_t const next = GetNextIndex(i);

    if ((m_s[i] & 0x3F) == lang)
    {
      ++i;
      utf8s.assign(m_s.c_str() + i, next - i);
      return true;
    }

    i = next;
  }

  return false;
}
