#include "coding/multilang_utf8_string.hpp"

#include "defines.hpp"

// TODO(AlexZ): Review and replace invalid languages which does not map correctly to
// iOS/Android locales/UI by valid and more popular ones.
// Languages below were choosen after sorting name:<lang> tags in 2011.
// Note, that it's not feasible to increase languages number here due to
// our current encoding (6 bit to store language code).
static char const * gLangs[] = {
    "default",
    "en", "ja", "fr", "ko_rm", "ar", "de", "int_name", "ru", "sv", "zh", "fi", "be", "ka", "ko",
    "he", "nl", "ga", "ja_rm", "el", "it", "es", "zh_pinyin", "th", "cy", "sr", "uk", "ca", "hu",
    "hsb", "eu", "fa", "br", "pl", "hy", "kn", "sl", "ro", "sq", "am", "fy", "cs", "gd", "sk",
    "af", "ja_kana", "lb", "pt", "hr", "fur", "vi", "tr", "bg", "eo", "lt", "la", "kk", "gsw",
    "et", "ku", "mn", "mk", "lv", "hi" };

int8_t StringUtf8Multilang::GetLangIndex(string const & lang)
{
  static_assert(ARRAY_SIZE(gLangs) == MAX_SUPPORTED_LANGUAGES, "");

  for (size_t i = 0; i < ARRAY_SIZE(gLangs); ++i)
    if (lang == gLangs[i])
      return static_cast<int8_t>(i);

  return UNSUPPORTED_LANGUAGE_CODE;
}

char const * StringUtf8Multilang::GetLangByCode(int8_t langCode)
{
  if (langCode < 0 || langCode > ARRAY_SIZE(gLangs) - 1)
    return "";
  return gLangs[langCode];
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

void StringUtf8Multilang::AddString(int8_t lang, string const & utf8s)
{
  size_t i = 0;
  size_t const sz = m_s.size();

  while (i < sz)
  {
    size_t const next = GetNextIndex(i);

    if ((m_s[i] & 0x3F) == lang)
    {
      ++i;
      m_s.replace(i, next - i, utf8s);
      return;
    }

    i = next;
  }

  m_s.push_back(lang | 0x80);
  m_s.insert(m_s.end(), utf8s.begin(), utf8s.end());
}

bool StringUtf8Multilang::GetString(int8_t lang, string & utf8s) const
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

namespace
{

struct Printer
{
  string & m_out;
  Printer(string & out) : m_out(out) {}
  bool operator()(int8_t code, string const & name) const
  {
    m_out += string(StringUtf8Multilang::GetLangByCode(code)) + string(":") + name + " ";
    return true;
  }
};

struct Finder
{
  string const & m_s;
  int8_t m_res;
  Finder(string const & s) : m_s(s), m_res(-1) {}
  bool operator()(int8_t code, string const & name)
  {
    if (name == m_s)
    {
      m_res = code;
      return false;
    }
    return true;
  }
};

} // namespace

int8_t StringUtf8Multilang::FindString(string const & utf8s) const
{
  Finder finder(utf8s);
  ForEachRef(finder);
  return finder.m_res;
}

string DebugPrint(StringUtf8Multilang const & s)
{
  string out;
  Printer printer(out);
  s.ForEachRef(printer);
  return out;
}
