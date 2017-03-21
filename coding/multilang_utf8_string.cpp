#include "coding/multilang_utf8_string.hpp"

#include "defines.hpp"

namespace
{
// TODO(AlexZ): Review and replace invalid languages which does not map correctly to
// iOS/Android locales/UI by valid and more popular ones.
// Languages below were choosen after sorting name:<lang> tags in 2011.
// Note, that it's not feasible to increase languages number here due to
// our current encoding (6 bit to store language code).
StringUtf8Multilang::Languages const g_languages = {{ {"default", "Native for each country", "Any"},
    {"en", "English", "English"}, {"ja", "日本語", "Japanese"}, {"fr", "Français", "French"}, {"ko_rm", "Korean (Romanized)", "Korean"},
    {"ar", "العربية", "Arabic"}, {"de", "Deutsch", "German"}, {"int_name", "International (Latin)", "Latin"}, {"ru", "Русский", "Russian"},
    {"sv", "Svenska", "Swedish"}, {"zh", "中文", "Chinese"}, {"fi", "Suomi", "Finnish"}, {"be", "Беларуская", "Belarusian"}, {"ka", "ქართული", "Georgian"},
    {"ko", "한국어", "Korean"}, {"he", "עברית", "Hebrew"}, {"nl", "Nederlands", "Dutch"}, {"ga", "Gaeilge", "Irish"},
    {"ja_rm", "Japanese (Romanized)", "Japanese"}, {"el", "Ελληνικά", "Greek"}, {"it", "Italiano", "Italian"}, {"es", "Español", "Spanish"},
    {"zh_pinyin", "Chinese (Pinyin)", "Chinese"}, {"th", "ไทย", "Thailand"}, {"cy", "Cymraeg", "Welsh"}, {"sr", "Српски", "Serbian"},
    {"uk", "Українська", "Ukrainian"}, {"ca", "Català", "Catalan"}, {"hu", "Magyar", "Hungarian"}, {"hsb", "Hornjoserbšćina", "Upper Sorbian"}, {"eu", "Euskara", "Basque"},
    {"fa", "فارسی", "Farsi"}, {"br", "Breton", "Breton"}, {"pl", "Polski", "Polish"}, {"hy", "Հայերէն", "Armenian"}, {"kn", "ಕನ್ನಡ", "Kannada"},
    {"sl", "Slovenščina", "Slovene"}, {"ro", "Română", "Romanian"}, {"sq", "Shqipe", "Shqipe"}, {"am", "አማርኛ", "Amharic"}, {"fy", "Frysk", "Frisian"},
    {"cs", "Čeština", "Czech"}, {"gd", "Gàidhlig", "Scots Gaelic"}, {"sk", "Slovenčina", "Slovak"}, {"af", "Afrikaans", "Afrikaans"},
    {"ja_kana", "日本語(カタカナ)", "Japanese (Katakana)"}, {"lb", "Luxembourgish", "Luxembourgish"}, {"pt", "Português", "Portuguese"}, {"hr", "Hrvatski", "Croatian"},
    {"fur", "Friulian", "Friulian"}, {"vi", "Tiếng Việt", "Vietnamese"}, {"tr", "Türkçe", "Turkish"}, {"bg", "Български", "Bulgarian"},
    {"eo", "Esperanto", "Esperanto"}, {"lt", "Lietuvių", "Lithuanian"}, {"la", "Latin", "Latin"}, {"kk", "Қазақ", "Kazakh"},
    {"gsw", "Schwiizertüütsch", "Swiss German"}, {"et", "Eesti", "Estonian"}, {"ku", "Kurdish", "Kurdish"}, {"mn", "Mongolian", "Mongolian"},
    {"mk", "Македонски", "Macedonian"}, {"lv", "Latviešu", "Latvian"}, {"hi", "हिन्दी", "Hindi"}
}};

static_assert(g_languages.size() == StringUtf8Multilang::kMaxSupportedLanguages,
              "With current encoding we are limited to 64 languages max.");
}  // namespace

int8_t constexpr StringUtf8Multilang::kUnsupportedLanguageCode;
int8_t constexpr StringUtf8Multilang::kDefaultCode;
int8_t constexpr StringUtf8Multilang::kEnglishCode;
int8_t constexpr StringUtf8Multilang::kInternationalCode;

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
  for (size_t i = 0; i < g_languages.size(); ++i)
    if (lang == g_languages[i].m_code)
      return static_cast<int8_t>(i);

  return kUnsupportedLanguageCode;
}

// static
char const * StringUtf8Multilang::GetLangByCode(int8_t langCode)
{
  if (langCode < 0 || langCode >= static_cast<int8_t>(g_languages.size()))
    return "";
  return g_languages[langCode].m_code;
}

// static
char const * StringUtf8Multilang::GetLangNameByCode(int8_t langCode)
{
  if (langCode < 0 || langCode >= static_cast<int8_t>(g_languages.size()))
    return "";
  return g_languages[langCode].m_name;
}

// static
char const * StringUtf8Multilang::GetLangEnNameByCode(int8_t langCode)
{
  if (langCode < 0 || langCode >= static_cast<int8_t>(g_languages.size()))
    return "";
  return g_languages[langCode].m_enName;
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

bool StringUtf8Multilang::HasString(int8_t lang) const
{
  for(size_t i = 0; i < m_s.size(); i = GetNextIndex(i))
  {
    if ((m_s[i] & 0x3F) == lang)
      return true;
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
