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
  {"cs", "Čeština"},
  {"da", "Dansk"},
  {"de", "Deutsch"},
  {"es", "Español"},
  {"fr", "Français"},
  {"hr", "Hrvatski"},
  {"id", "Indonesia"},
  {"it", "Italiano"},
  {"sw", "Kiswahili"},
  {"hu", "Magyar"},
  {"nl", "Nederlands"},
  {"pl", "Polski"},
  {"pt", "Português"},
  {"ro", "Română"},
  {"sk", "Slovenčina"},
  {"fi", "Suomi"},
  {"sv", "Svenska"},
  {"vi", "Tiếng Việt"},
  {"tr", "Türkçe"},
  {"el", "Ελληνικά"},
  {"uk", "Українська"},
  {"ar", "العربية"},
  {"fa", "فارسی"},
  {"hi", "हिंदी"},
  {"ja", "日本語"},
  {"ko", "한국어"},
  {"th", "ภาษาไทย"},
  {"zh-Hant", "中文繁體"},
  {"zh-Hans", "中文简体"},
}};
}  // namespace sound
}  // namespace turns
}  // namespace routing
