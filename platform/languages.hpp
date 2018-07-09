#pragma once

#include <array>
#include <string>
#include <utility>

// The list of languages which can be used by TTS.
// It shall be included in Android (jni) and iOS parts to get the languages list.

namespace routing
{
namespace turns
{
namespace sound
{
std::array<std::pair<std::string, std::string>, 31> const kLanguageList =
{{
  {"en", "English"},
  {"ru", "Русский"},
  {"es", "Español"},
  {"de", "Deutsch"},
  {"fr", "Français"},
  {"zh-Hant", "中文繁體"},
  {"zh-Hans", "中文简体"},
  {"pt", "Português"},
  {"th", "ภาษาไทย"},
  {"tr", "Türkçe"},
  {"ar", "العربية"},
  {"cs", "Čeština"},
  {"da", "Dansk"},
  {"el", "Ελληνικά"},
  {"fi", "Suomi"},
  {"hi", "हिंदी"},
  {"hr", "Hrvatski"},
  {"hu", "Magyar"},
  {"id", "Indonesia"},
  {"it", "Italiano"},
  {"ja", "日本語"},
  {"ko", "한국어"},
  {"nl", "Nederlands"},
  {"pl", "Polski"},
  {"ro", "Română"},
  {"sk", "Slovenčina"},
  {"sv", "Svenska"},
  {"sw", "Kiswahili"},
  {"fa", "فارسی"},
  {"uk", "Українська"},
  {"vi", "Tiếng Việt"}
}};
}  // namespace sound
}  // namespace turns
}  // namespace routing
