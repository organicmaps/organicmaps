#pragma once

#include <array>
#include <string>
#include <utility>

// The list of languages which can be used by TTS.
// It shall be included in Android (jni) and iOS parts to get the languages list.
// TODO: Now it is used only on iOS.
// Manual sync with android/res/values/strings-tts.xml is needed.

namespace routing::turns::sound
{
std::array<std::pair<std::string, std::string>, 38> const kLanguageList =
{{
  {"en", "English"},
  {"id", "Bahasa Indonesia"},
  {"ca", "Català"},
  {"da", "Dansk"},
  {"de", "Deutsch"},
  {"es", "Español"},
  {"es-MX", "Español (México)"},
  {"eu", "Euskara"},
  {"fr", "Français"},
  {"hr", "Hrvatski"},
  {"it", "Italiano"},
  {"sw", "Kiswahili"},
  {"hu", "Magyar"},
  {"nl", "Nederlands"},
  {"nb", "Norsk Bokmål"},
  {"pl", "Polski"},
  {"pt", "Português"},
  {"pt-BR", "Português (Brasil)"},
  {"ro", "Română"},
  {"sk", "Slovenčina"},
  {"fi", "Suomi"},
  {"sv", "Svenska"},
  {"vi", "Tiếng Việt"},
  {"tr", "Türkçe"},
  {"cs", "Čeština"},
  {"el", "Ελληνικά"},
  {"be", "Беларуская"},
  {"ru", "Русский"},
  {"uk", "Українська"},
  {"ar", "العربية"},
  {"fa", "فارسی"},
  {"mr", "मराठी"},
  {"hi", "हिंदी"},
  {"th", "ไทย"},
  {"zh-Hans", "中文简体"},
  {"zh-Hant", "中文繁體"},
  {"ja", "日本語"},
  {"ko", "한국어"},
}};
}  // namespace routing::turns::sound
