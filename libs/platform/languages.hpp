#pragma once

#include <array>
#include <string>
#include <utility>

#include "std/target_os.hpp"

namespace routing::turns::sound
{
/**
 * @brief The list of languages which can be used by TTS (Text-To-Speech).
 *
 * Supported language identifiers follow the format:
 *   @code
 *   language[-COUNTRY][:internal_code]
 *   @endcode
 *
 * Where:
 * - `language`: a two-letter ISO 639-1 language code (e.g., "en", "fr", "zh").
 * - `COUNTRY`: optional two-letter ISO 3166-1 alpha-2 country code (e.g., "US", "CN", "TW").
 * - `internal_code`: optional internal language code used by the TTS core.
 *                    If not specified, `language` is used as the default.
 *
 * @note Special handling for Chinese voice/sound assets:
 * - `zh-Hans` = Mandarin in Simplified script (Mainland China / Singapore).
 * - `zh-Hant` = Mandarin in Traditional script (Taiwan; also fallback for HK/MO).
 * - `yue-HK` = Cantonese (Hong Kong).
 * - `yue-MO` = Cantonese (Macau).
 * - `yue` = Cantonese (generic; not auto-selected, user picks manually).
 *
 * Auto-selection from Android device locale (see `LanguageData.matchesLocale`):
 * - Country `HK` → `yue-HK`, `MO` → `yue-MO`, `TW` → `zh-Hant`, else `zh-Hans`.
 *
 * The Android list uses `bcp47:internal` form so `LanguageData` can map a
 * Locale to the internal twine code; iOS does the equivalent mapping in
 * `LocaleTranslator.mm` and stores only the internal code here.
 */
std::array<std::pair<std::string_view, std::string_view>, 49> constexpr kLanguageList = {{
    {"en", "English"},
    {"id", "Bahasa Indonesia"},
    {"ca", "Català"},
    {"da", "Dansk"},
    {"de", "Deutsch"},
    {"et", "Eesti"},
#ifdef OMIM_OS_ANDROID
    {"es-ES:es", "Español"},
    {"es-MX:es-MX", "Español (México)"},
#else
    {"es", "Español"},
    {"es-MX", "Español (México)"},
#endif
    {"eu", "Euskara"},
    {"fr", "Français"},
    {"gl", "Galego"},
    {"hr", "Hrvatski"},
    {"it", "Italiano"},
    {"sw", "Kiswahili"},
    {"hu", "Magyar"},
    {"lt", "Lietuvių"},
    {"nl", "Nederlands"},
    {"nb", "Norsk Bokmål"},
    {"pl", "Polski"},
#ifdef OMIM_OS_ANDROID
    {"pt-PT:pt", "Português"},
    {"pt-BR:pt-BR", "Português (Brasil)"},
#else
    {"pt", "Português"},
    {"pt-BR", "Português (Brasil)"},
#endif
    {"ro", "Română"},
    {"sk", "Slovenčina"},
    {"sl", "Slovenščina"},
    {"sq", "Shqip"},
    {"fi", "Suomi"},
    {"sv", "Svenska"},
    {"vi", "Tiếng Việt"},
    {"tr", "Türkçe"},
    {"cs", "Čeština"},
    {"el", "Ελληνικά"},
    {"be", "Беларуская"},
    {"bg", "Български"},
    {"ru", "Русский"},
    {"sr", "Српски"},
    {"uk", "Українська"},
    {"he", "עברית"},
    {"ar", "العربية"},
    {"fa", "فارسی"},
    {"mr", "मराठी"},
    {"hi", "हिंदी"},
    {"th", "ไทย"},
#ifdef OMIM_OS_ANDROID
    {"zh-CN:zh-Hans", "中文（普通话）"},
    {"zh-TW:zh-Hant", "中文（國語）"},
    {"zh-HK:yue-HK", "粵語（香港）"},
    {"zh-MO:yue-MO", "粵語（澳門）"},
    {"yue:yue", "粵語"},
#else
    {"zh-Hans", "中文（普通话）"},
    {"zh-Hant", "中文（國語）"},
    {"yue-HK", "粵語（香港）"},
    {"yue-MO", "粵語（澳門）"},
    {"yue", "粵語"},
#endif
    {"ja", "日本語"},
    {"ko", "한국어"},
}};
}  // namespace routing::turns::sound
