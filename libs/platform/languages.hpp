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
 * @note Special handling for Chinese:
 * - `zh_TW`, `zh_MO`, and `zh_HK` are treated as `zh-Hant` (Traditional Chinese).
 * - All other variants default to `zh-Hans` (Simplified Chinese).
 *
 */
std::array<std::pair<std::string_view, std::string_view>, 40> constexpr kLanguageList = {{
    {"en", "English"},
    {"id", "Bahasa Indonesia"},
    {"ca", "Català"},
    {"da", "Dansk"},
    {"de", "Deutsch"},
#ifdef OMIM_OS_ANDROID
    {"es-ES:es", "Español"},
    {"es-MX:es-MX", "Español (México)"},
#else
    {"es", "Español"},       {"es-MX", "Español (México)"},
#endif
    {"eu", "Euskara"},
    {"fr", "Français"},
    {"hr", "Hrvatski"},
    {"it", "Italiano"},
    {"sw", "Kiswahili"},
    {"hu", "Magyar"},
    {"nl", "Nederlands"},
    {"nb", "Norsk Bokmål"},
    {"pl", "Polski"},
#ifdef OMIM_OS_ANDROID
    {"pt-PT:pt", "Português"},
    {"pt-BR:pt-BR", "Português (Brasil)"},
#else
    {"pt", "Português"},     {"pt-BR", "Português (Brasil)"},
#endif
    {"ro", "Română"},
    {"sk", "Slovenčina"},
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
    {"ar", "العربية"},
    {"fa", "فارسی"},
    {"mr", "मराठी"},
    {"hi", "हिंदी"},
    {"th", "ไทย"},
#ifdef OMIM_OS_ANDROID
    {"zh-CN:zh-Hans", "中文简体"},
    {"zh-TW:zh-Hant", "中文繁體"},
#else
    {"zh-Hans", "中文简体"}, {"zh-Hant", "中文繁體"},
#endif
    {"ja", "日本語"},
    {"ko", "한국어"},
}};
}  // namespace routing::turns::sound
