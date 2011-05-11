#include "multilang_utf8_string.hpp"

char StringUtf8Multilang::m_priorities[] =
{
  0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
  21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
  41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60,
  61, 62, 63
};

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

  STATIC_ASSERT(ARRAY_SIZE(arr) <= 64);

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

void StringUtf8Multilang::SetPreferableLanguages(vector<string> const & langCodes)
{
  CHECK_EQUAL(langCodes.size(), 64, ());
  for (size_t i = 0; i < langCodes.size(); ++i)
  {
    char index = GetLangIndex(langCodes[i]);
    if (index >= 0)
      m_priorities[static_cast<size_t>(index)] = i;
    else
    {
      ASSERT(false, ("Invalid language code"));
    }
    CHECK_GREATER_OR_EQUAL(m_priorities[i], 0, ("Unsupported language", langCodes[i]));
  }
}

void StringUtf8Multilang::GetPreferableString(string & utf8s) const
{
  size_t i = 0;
  size_t const sz = m_s.size();

  int currPriority = 256;
  while (i < sz)
  {
    size_t const next = GetNextIndex(i);

    int p = m_priorities[m_s[i] & 0x3F];
    if (p < currPriority)
    {
      ++i;

      currPriority = p;
      utf8s.assign(m_s.c_str() + i, next - i);

      if (p == 0)
        return;
    }

    i = next;
  }
}
