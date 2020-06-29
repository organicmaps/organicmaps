#include "coding/string_utf8_multilang.hpp"

#include <algorithm>
#include <array>

#include "defines.hpp"

using namespace std;

namespace
{
// Order is important. Any reordering breaks backward compatibility.
// Languages with code |StringUtf8Multilang::kReservedLang| may be used for another language after
// several data releases.
// Note that it's not feasible to increase languages number here due to current encoding (6 bit to
// store language code).
array<StringUtf8Multilang::Lang, StringUtf8Multilang::kMaxSupportedLanguages> const kLanguages = {
    {{"default", "Native for each country", {"Any-Latin"}},
     {"en", "English", {}},
     {"ja", "日本語", {}},
     {"fr", "Français", {}},
     {"ko_rm", "Korean (Romanized)", {"Korean-Latin/BGN"}},
     {"ar", "العربية", {"Any-Latin"}},
     {"de", "Deutsch", {}},
     {"int_name", "International (Latin)", {"Any-Latin"}},
     {"ru", "Русский", {"Russian-Latin/BGN"}},
     {"sv", "Svenska", {}},
     {"zh", "中文", {"Any-Latin"}},
     {"fi", "Suomi", {}},
     {"be", "Беларуская", {"Belarusian-Latin/BGN"}},
     {"ka", "ქართული", {"Georgian-Latin"}},
     {"ko", "한국어", {"Hangul-Latin/BGN"}},
     {"he", "עברית", {"Hebrew-Latin"}},
     {"nl", "Nederlands", {}},
     {"ga", "Gaeilge", {}},
     {"ja_rm", "Japanese (Romanized)", {"Any-Latin"}},
     {"el", "Ελληνικά", {"Greek-Latin"}},
     {"it", "Italiano", {}},
     {"es", "Español", {}},
     {"zh_pinyin", "Chinese (Pinyin)", {"Any-Latin"}},
     {"th", "ไทย", {}},  // Thai-Latin
     {"cy", "Cymraeg", {}},
     {"sr", "Српски", {"Serbian-Latin/BGN"}},
     {"uk", "Українська", {"Ukrainian-Latin/BGN"}},
     {"ca", "Català", {}},
     {"hu", "Magyar", {}},
     {StringUtf8Multilang::kReservedLang /* hsb */, "", {}},
     {"eu", "Euskara", {}},
     {"fa", "فارسی", {"Any-Latin"}},
     {StringUtf8Multilang::kReservedLang /* br */, "", {}},
     {"pl", "Polski", {}},
     {"hy", "Հայերէն", {"Armenian-Latin"}},
     {StringUtf8Multilang::kReservedLang /* kn */, "", {}},
     {"sl", "Slovenščina", {}},
     {"ro", "Română", {}},
     {"sq", "Shqip", {}},
     {"am", "አማርኛ", {"Amharic-Latin/BGN"}},
     {"no", "Norsk", {}},  // Was "fy" before December 2018.
     {"cs", "Čeština", {}},
     {"id", "Bahasa Indonesia", {}},  // Was "gd" before December 2018.
     {"sk", "Slovenčina", {}},
     {"af", "Afrikaans", {}},
     {"ja_kana", "日本語(カタカナ)", {"Katakana-Latin", "Hiragana-Latin"}},
     {StringUtf8Multilang::kReservedLang /* lb */, "", {}},
     {"pt", "Português", {}},
     {"hr", "Hrvatski", {}},
     {"da", "Dansk", {}},  // Was "fur" before December 2018.
     {"vi", "Tiếng Việt", {}},
     {"tr", "Türkçe", {}},
     {"bg", "Български", {"Bulgarian-Latin/BGN"}},
     {"alt_name", "Alternative name", {"Any-Latin"}},  // Was "eo" before December 2018.
     {"lt", "Lietuvių", {}},
     {"old_name", "Old/Previous name", {"Any-Latin"}},  // Was "la" before December 2018.
     {"kk", "Қазақ", {"Kazakh-Latin/BGN"}},
     {StringUtf8Multilang::kReservedLang /* gsw */, "", {}},
     {"et", "Eesti", {}},
     {"ku", "Kurdish", {"Any-Latin"}},
     {"mn", "Mongolian", {"Mongolian-Latin/BGN"}},
     {"mk", "Македонски", {"Macedonian-Latin/BGN"}},
     {"lv", "Latviešu", {}},
     {"hi", "हिन्दी", {"Any-Latin"}}}};

static_assert(
    kLanguages.size() == StringUtf8Multilang::kMaxSupportedLanguages,
    "With current encoding we are limited to 64 languages max. And we need kLanguages.size()"
    " to be exactly 64 for backward compatibility.");

bool IsSupportedLangCode(int8_t langCode)
{
  return langCode >= 0 && langCode < static_cast<int8_t>(kLanguages.size()) &&
         kLanguages[langCode].m_code != StringUtf8Multilang::kReservedLang;
}
}  // namespace

int8_t constexpr StringUtf8Multilang::kUnsupportedLanguageCode;
int8_t constexpr StringUtf8Multilang::kDefaultCode;
int8_t constexpr StringUtf8Multilang::kEnglishCode;
int8_t constexpr StringUtf8Multilang::kInternationalCode;
char constexpr StringUtf8Multilang::kReservedLang[9 /* strlen("reserved") + 1 */];

// static
StringUtf8Multilang::Languages const & StringUtf8Multilang::GetSupportedLanguages()
{
  // Asserts for generic class constants.
  ASSERT_EQUAL(kLanguages[kDefaultCode].m_code, string("default"), ());
  ASSERT_EQUAL(kLanguages[kInternationalCode].m_code, string("int_name"), ());
  ASSERT_EQUAL(kLanguages[kAltNameCode].m_code, string("alt_name"), ());
  ASSERT_EQUAL(kLanguages[kOldNameCode].m_code, string("old_name"), ());
  ASSERT_EQUAL(kLanguages[kEnglishCode].m_code, string("en"), ());
  static StringUtf8Multilang::Languages languages;
  if (languages.empty())
  {
    copy_if(kLanguages.cbegin(), kLanguages.cend(), back_inserter(languages),
            [](Lang const & lang) { return lang.m_code != kReservedLang; });
  }

  return languages;
}

// static
int8_t StringUtf8Multilang::GetLangIndex(string const & lang)
{
  if (lang == kReservedLang)
    return kUnsupportedLanguageCode;

  for (size_t i = 0; i < kLanguages.size(); ++i)

    if (lang == kLanguages[i].m_code)
      return static_cast<int8_t>(i);

  return kUnsupportedLanguageCode;
}

// static
char const * StringUtf8Multilang::GetLangByCode(int8_t langCode)
{
  if (!IsSupportedLangCode(langCode))
    return "";

  return kLanguages[langCode].m_code.c_str();
}

// static
char const * StringUtf8Multilang::GetLangNameByCode(int8_t langCode)
{
  if (!IsSupportedLangCode(langCode))
    return "";

  return kLanguages[langCode].m_name.c_str();
}

// static
vector<string> const & StringUtf8Multilang::GetTransliteratorsIdsByCode(int8_t langCode)
{
  static const vector<string> empty;
  if (!IsSupportedLangCode(langCode))
    return empty;

  return kLanguages[langCode].m_transliteratorsIds;
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

    if ((m_s[i] & kLangCodeMask) == lang)
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

void StringUtf8Multilang::RemoveString(int8_t lang)
{
  size_t i = 0;
  size_t const sz = m_s.size();

  while (i < sz)
  {
    size_t const next = GetNextIndex(i);

    if ((m_s[i] & kLangCodeMask) == lang)
    {
      m_s.erase(i, next - i);
      return;
    }

    i = next;
  }
}

bool StringUtf8Multilang::GetString(int8_t lang, string & utf8s) const
{
  if (!IsSupportedLangCode(lang))
    return false;

  size_t i = 0;
  size_t const sz = m_s.size();

  while (i < sz)
  {
    size_t const next = GetNextIndex(i);

    if ((m_s[i] & kLangCodeMask) == lang)
    {
      ++i;
      utf8s.assign(m_s.c_str() + i, next - i);
      return true;
    }

    i = next;
  }

  return false;
}

StringUtf8Multilang::TranslationPositions StringUtf8Multilang::GenerateTranslationPositions() const
{
  TranslationPositions result;
  size_t i = 0;
  size_t const sz = m_s.size();
  while (i < sz)
  {
    size_t const next = GetNextIndex(i);
    int8_t const code = m_s[i] & kLangCodeMask;
    if (GetLangByCode(code) != kReservedLang)
      result[code] = Position{i + 1, next - i - 1};

    i = next;
  }

  return result;
}

std::string StringUtf8Multilang::GetTranslation(
    StringUtf8Multilang::Position const & position) const
{
  return m_s.substr(position.m_begin, position.m_length);
}

bool StringUtf8Multilang::HasString(int8_t lang) const
{
  if (!IsSupportedLangCode(lang))
    return false;

  for (size_t i = 0; i < m_s.size(); i = GetNextIndex(i))
  {
    if ((m_s[i] & kLangCodeMask) == lang)
      return true;
  }

  return false;
}

int8_t StringUtf8Multilang::FindString(string const & utf8s) const
{
  int8_t result = kUnsupportedLanguageCode;

  ForEach([&utf8s, &result](int8_t code, string const & name) {
    if (name == utf8s)
    {
      result = code;
      return base::ControlFlow::Break;
    }
    return base::ControlFlow::Continue;
  });

  return result;
}

size_t StringUtf8Multilang::CountLangs() const
{
  size_t count = 0;
  for (size_t i = 0; i < m_s.size(); i = GetNextIndex(i))
    ++count;

  return count;
}

string DebugPrint(StringUtf8Multilang const & s)
{
  string result;

  s.ForEach([&result](int8_t code, string const & name) {
    result += string(StringUtf8Multilang::GetLangByCode(code)) + string(":") + name + " ";
  });

  return result;
}
