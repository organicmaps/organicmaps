#include "LocaleTranslator.h"

#include "platform/preferred_languages.hpp"

namespace locale_translator
{
std::string bcp47ToTwineLanguage(NSString * bcp47)
{
  if (bcp47 == nil || bcp47.length < 2)
    return {};

  // Both speech voice tags and Foundation locale identifiers reach this function. Parse the region
  // independently of optional scripts and calendar/numbering preferences used by the editor locale.
  NSLocale * locale = [NSLocale localeWithLocaleIdentifier:bcp47];
  NSString * language = locale.languageCode;
  NSString * region = locale.countryCode;

  // These regions have their own notification translations in data/strings/sound.txt.
  if ([language isEqualToString:@"pt"] && [region isEqualToString:@"BR"])
    return "pt-BR";
  if ([language isEqualToString:@"es"] && [region isEqualToString:@"MX"])
    return "es-MX";

  // AVFoundation reports Cantonese voices with yue-* BCP47 tags.
  // Note: zh-HK / zh-MO are intentionally NOT mapped to yue-* here — they
  // identify Mandarin (Traditional) HK/MO locales used by category localization
  // (see MWMObjectsCategorySelectorDataSource.mm).
  if ([language isEqualToString:@"yue"])
  {
    if ([region isEqualToString:@"HK"])
      return "yue-HK";
    if ([region isEqualToString:@"MO"])
      return "yue-MO";
    return "yue";
  }

  // Mandarin. The script subtag wins over the region, so this also reads the tags the system hands
  // out for a preferred language, e.g. "zh-Hans-CN", which a plain "zh" prefix test cannot tell from
  // Traditional.
  NSString * tag = [locale.localeIdentifier componentsSeparatedByString:@"@"].firstObject;
  switch (languages::GetChineseScript(tag.UTF8String))
  {
  case languages::ChineseScript::Simplified: return "zh-Hans";
  case languages::ChineseScript::Traditional: return "zh-Hant";
  case languages::ChineseScript::NotChinese: break;
  }

  // Apart from Cantonese above, the supported primary codes have two letters. In particular, "fil"
  // (Filipino) must not become "fi" (Finnish).
  return language.length == 2 ? language.UTF8String : std::string();
}
}  // namespace locale_translator
