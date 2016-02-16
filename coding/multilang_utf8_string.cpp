#include "coding/multilang_utf8_string.hpp"

#include "defines.hpp"

namespace
{
// TODO(AlexZ): Review and replace invalid languages which does not map correctly to
// iOS/Android locales/UI by valid and more popular ones.
// Languages below were choosen after sorting name:<lang> tags in 2011.
// Note, that it's not feasible to increase languages number here due to
// our current encoding (6 bit to store language code).
StringUtf8Multilang::Languages const g_languages = {{ {"default", "Native for each country"},
    {"en", "English"}, {"ja", "日本語"}, {"fr", "Français"}, {"ko_rm", "Korean (Romanized)"},
    {"ar", "العربية"}, {"de", "Deutsch"}, {"int_name", "International (Latin)"}, {"ru", "Русский"},
    {"sv", "Svenska"}, {"zh", "中文"}, {"fi", "Suomi"}, {"be", "Беларуская"}, {"ka", "ქართული"},
    {"ko", "한국어"}, {"he", "עברית"}, {"nl", "Nederlands"}, {"ga", "Gaeilge"},
    {"ja_rm", "Japanese (Romanized)"}, {"el", "Ελληνικά"}, {"it", "Italiano"}, {"es", "Español"},
    {"zh_pinyin", "Chinese (Pinyin)"}, {"th", "ไทย"}, {"cy", "Cymraeg"}, {"sr", "Српски"},
    {"uk", "Українська"}, {"ca", "Català"}, {"hu", "Magyar"}, {"hsb", "Hornjoserbšćina"}, {"eu", "Euskara"},
    {"fa", "فارسی"}, {"br", "Breton"}, {"pl", "Polski"}, {"hy", "Հայերէն"}, {"kn", "ಕನ್ನಡ"},
    {"sl", "Slovenščina"}, {"ro", "Română"}, {"sq", "Shqipe"}, {"am", "አማርኛ"}, {"fy", "Frysk"},
    {"cs", "Čeština"}, {"gd", "Gàidhlig"}, {"sk", "Slovenčina"}, {"af", "Afrikaans"},
    {"ja_kana", "日本語(カタカナ)"}, {"lb", "Luxembourgish"}, {"pt", "Português"}, {"hr", "Hrvatski"},
    {"fur", "Friulian"}, {"vi", "Tiếng Việt"}, {"tr", "Türkçe"}, {"bg", "Български"},
    {"eo", "Esperanto"}, {"lt", "Lietuvių"}, {"la", "Latin"}, {"kk", "Қазақ"},
    {"gsw", "Schwiizertüütsch"}, {"et", "Eesti"}, {"ku", "Kurdish"}, {"mn", "Mongolian"},
    {"mk", "Македонски"}, {"lv", "Latviešu"}, {"hi", "हिन्दी"}
}};
}  // namespace

// static
StringUtf8Multilang::Languages const & StringUtf8Multilang::GetSupportedLanguages()
{
  // Asserts for generic class constants.
  ASSERT_EQUAL(g_languages[kDefaultCode].m_code, string("default"), ());
  ASSERT_EQUAL(g_languages[kInternationalCode].m_code, string("int_name"), ());
  return g_languages;
}
// static
int8_t StringUtf8Multilang::GetLangIndex(string const & lang)
{
  static_assert(g_languages.size() == kMaxSupportedLanguages,
                "With current encoding we are limited to 64 languages max.");

  for (size_t i = 0; i < g_languages.size(); ++i)
    if (lang == g_languages[i].m_code)
      return static_cast<int8_t>(i);

  return kUnsupportedLanguageCode;
}
// static
char const * StringUtf8Multilang::GetLangByCode(int8_t langCode)
{
  if (langCode < 0 || langCode > g_languages.size() - 1)
    return "";
  return g_languages[langCode].m_code;
}
// static
char const * StringUtf8Multilang::GetLangNameByCode(int8_t langCode)
{
  if (langCode < 0 || langCode > g_languages.size() - 1)
    return "";
  return g_languages[langCode].m_name;
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
  ForEach(finder);
  return finder.m_res;
}

string DebugPrint(StringUtf8Multilang const & s)
{
  string out;
  s.ForEach(Printer(out));
  return out;
}
