#include "coding/string_utf8_multilang.hpp"

#include <algorithm>
#include <array>
#include <cstdint>

namespace
{
// Order is important. Any reordering breaks backward compatibility.
// Languages with code |StringUtf8Multilang::kReservedLang| may be used for another language after
// several data releases.
// Note that it's not feasible to increase the number of languages here due to current encoding (6 bit to
// store language code).
std::array<StringUtf8Multilang::Lang, StringUtf8Multilang::kMaxSupportedLanguages> constexpr kLanguages = {
    {{"default", "Name in local language", {"Any-Latin"}},
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
     {"mr", "मराठी", {"Any-Latin"}},  // Was kReservedLang "gsw" before March 2022
     {"et", "Eesti", {}},
     {"ku", "Kurdish", {"Any-Latin"}},
     {"mn", "Mongolian", {"Mongolian-Latin/BGN"}},
     {"mk", "Македонски", {"Macedonian-Latin/BGN"}},
     {"lv", "Latviešu", {}},
     {"hi", "हिन्दी", {"Any-Latin"}}}};

static_assert(kLanguages.size() == StringUtf8Multilang::kMaxSupportedLanguages,
              "With current encoding we are limited to 64 languages max. And we need kLanguages.size()"
              " to be exactly 64 for backward compatibility.");
static_assert(kLanguages[StringUtf8Multilang::kDefaultCode].m_code == std::string_view{"default"});
static_assert(kLanguages[StringUtf8Multilang::kInternationalCode].m_code == std::string_view{"int_name"});
static_assert(kLanguages[StringUtf8Multilang::kAltNameCode].m_code == std::string_view{"alt_name"});
static_assert(kLanguages[StringUtf8Multilang::kOldNameCode].m_code == std::string_view{"old_name"});
static_assert(kLanguages[StringUtf8Multilang::kEnglishCode].m_code == std::string_view{"en"});

constexpr bool IsSupportedLangCode(int8_t langCode)
{
  return langCode >= 0 && langCode < static_cast<int8_t>(kLanguages.size()) &&
         kLanguages[langCode].m_code != StringUtf8Multilang::kReservedLang;
}

constexpr bool IsServiceLang(std::string_view const lang)
{
  return lang == kLanguages[StringUtf8Multilang::kDefaultCode].m_code ||
         lang == kLanguages[StringUtf8Multilang::kInternationalCode].m_code ||
         lang == kLanguages[StringUtf8Multilang::kAltNameCode].m_code ||
         lang == kLanguages[StringUtf8Multilang::kOldNameCode].m_code;
}

StringUtf8Multilang::Languages constexpr allLanguages = [] consteval
{
  StringUtf8Multilang::Languages langs;
  std::ranges::copy_if(kLanguages, std::back_inserter(langs), [](StringUtf8Multilang::Lang const & lang)
  { return lang.m_code != StringUtf8Multilang::kReservedLang; });
  return langs;
}();

StringUtf8Multilang::Languages constexpr languagesWithoutService = [] consteval
{
  StringUtf8Multilang::Languages langs;
  std::ranges::copy_if(allLanguages, std::back_inserter(langs),
                       [](StringUtf8Multilang::Lang const & lang) { return !IsServiceLang(lang.m_code); });
  return langs;
}();

// Compile-time perfect hash table for O(1) language code lookup.
constexpr uint32_t LangHash(std::string_view s)
{
  uint32_t h = 0x811c9dc5u;  // FNV-1a offset basis
  for (char c : s)
  {
    h ^= static_cast<uint32_t>(static_cast<unsigned char>(c));
    h *= 0x01000193u;  // FNV-1a prime
  }
  return h;
}

struct LangHashEntry
{
  std::string_view code{};
  int8_t index = StringUtf8Multilang::kUnsupportedLanguageCode;
};

constexpr size_t kHashTableSize = 128;
constexpr size_t kHashMask = kHashTableSize - 1;

using LangHashTable = std::array<LangHashEntry, kHashTableSize>;

consteval LangHashTable BuildLangHashTable()
{
  LangHashTable table{};
  for (size_t i = 0; i < kLanguages.size(); ++i)
  {
    auto const & lang = kLanguages[i];
    if (lang.m_code == StringUtf8Multilang::kReservedLang)
      continue;

    uint32_t slot = LangHash(lang.m_code) & kHashMask;
    while (!table[slot].code.empty())
      slot = (slot + 1) & kHashMask;

    table[slot].code = lang.m_code;
    table[slot].index = static_cast<int8_t>(i);
  }
  return table;
}

constexpr LangHashTable kLangHashTable = BuildLangHashTable();

int8_t LookupLangIndex(std::string_view lang)
{
  uint32_t slot = LangHash(lang) & kHashMask;
  for (;;)
  {
    auto const & entry = kLangHashTable[slot];
    if (entry.code.empty())
      return StringUtf8Multilang::kUnsupportedLanguageCode;
    if (entry.code == lang)
      return entry.index;
    slot = (slot + 1) & kHashMask;
  }
}
}  // namespace

bool StringUtf8Multilang::IsServiceLang(std::string_view const lang)
{
  return ::IsServiceLang(lang);
}

StringUtf8Multilang::Languages const & StringUtf8Multilang::GetSupportedLanguages(bool includeServiceLangs)
{
  return includeServiceLangs ? allLanguages : languagesWithoutService;
}

// static
int8_t StringUtf8Multilang::GetLangIndex(std::string_view lang)
{
  if (lang == kReservedLang)
    return kUnsupportedLanguageCode;

  return LookupLangIndex(lang);
}

// static
std::string_view StringUtf8Multilang::GetLangByCode(int8_t langCode)
{
  if (!IsSupportedLangCode(langCode))
    return {};

  return kLanguages[langCode].m_code;
}

// static
std::string_view StringUtf8Multilang::GetLangNameByCode(int8_t langCode)
{
  if (!IsSupportedLangCode(langCode))
    return {};

  return kLanguages[langCode].m_name;
}

// static
StringUtf8Multilang::Lang::TransliteratorsList const * StringUtf8Multilang::GetTransliteratorsIdsByCode(int8_t langCode)
{
  if (!IsSupportedLangCode(langCode))
    return nullptr;

  return &kLanguages[langCode].m_transliteratorsIds;
}

std::string StringUtf8Multilang::GetOSMTagByCode(uint8_t const langCode)
{
  std::string_view const lang = GetLangByCode(static_cast<int8_t>(langCode));
  if (lang == "int_name" || lang == "alt_name" || lang == "old_name")
    return std::string{lang};
  else if (lang == "default")
    return "name";
  else if (!lang.empty())
    return std::string{"name:"}.append(lang);
  else
  {
    ASSERT_FAIL(("Language can not be an empty string"));
    return "";
  }
}

uint8_t StringUtf8Multilang::GetCodeByOSMTag(std::string const & name)
{
  std::string lang;
  if (name.starts_with("name:"))
    lang = name.substr(5);
  else if (name == "name")
    lang = "default";
  else
    lang = name;

  return StringUtf8Multilang::GetLangIndex(lang);
}

size_t StringUtf8Multilang::GetNextIndex(size_t i) const
{
  ++i;
  size_t const sz = m_s.size();

  while (i < sz && (m_s[i] & 0xC0) != 0x80)
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

  return i;
}

void StringUtf8Multilang::AddString(int8_t lang, std::string_view utf8s)
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

bool StringUtf8Multilang::GetString(int8_t lang, std::string_view & utf8s) const
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
      utf8s = {m_s.c_str() + i, next - i};

      if (utf8s.empty() && lang != kDefaultCode)
        VERIFY(GetString(kDefaultCode, utf8s) && !utf8s.empty(), ());

      return true;
    }

    i = next;
  }

  return false;
}

std::string_view StringUtf8Multilang::GetBestString(buffer_vector<int8_t, 4> const & langs) const
{
  size_t langIdx = langs.size() + 1;
  std::string_view res;

  ForEach([&res, &langIdx, &langs](int8_t code, std::string_view s)
  {
    auto it = std::find(langs.begin(), langs.end(), code);
    if (it != langs.end())
    {
      size_t const idx = std::distance(langs.begin(), it);
      if (idx < langIdx)
      {
        res = s;
        langIdx = idx;
      }
    }
  }, false /* emptyLikeDefault */);

  return res;
}

std::string_view StringUtf8Multilang::GetFirstString() const
{
  if (m_s.empty())
    return {};
  return std::string_view(m_s).substr(1, GetNextIndex(0) - 1);
}

bool StringUtf8Multilang::HasString(int8_t lang) const
{
  if (!IsSupportedLangCode(lang))
    return false;

  for (size_t i = 0; i < m_s.size(); i = GetNextIndex(i))
    if ((m_s[i] & kLangCodeMask) == lang)
      return true;

  return false;
}

int8_t StringUtf8Multilang::FindString(std::string const & utf8s) const
{
  int8_t result = kUnsupportedLanguageCode;

  ForEach([&utf8s, &result](int8_t code, std::string_view name)
  {
    if (name == utf8s)
    {
      result = code;
      return base::ControlFlow::Break;
    }
    return base::ControlFlow::Continue;
  }, false /* emptyLikeDefault */);

  return result;
}

size_t StringUtf8Multilang::CountLangs() const
{
  size_t count = 0;
  for (size_t i = 0; i < m_s.size(); i = GetNextIndex(i))
    ++count;

  return count;
}

std::string DebugPrint(StringUtf8Multilang const & s)
{
  std::string result;

  bool isFirst = true;
  s.ForEach([&result, &isFirst](int8_t code, std::string_view name)
  {
    if (isFirst)
      isFirst = false;
    else
      result += ' ';

    result.append(StringUtf8Multilang::GetLangByCode(code)).append(":").append(name);
  });

  return result;
}

StringUtf8Multilang StringUtf8Multilang::FromBuffer(std::string && s)
{
  ASSERT(!s.empty(), ());
  StringUtf8Multilang res;
  res.m_s = std::move(s);
  ASSERT_GREATER(res.CountLangs(), 0, ());
  return res;
}
