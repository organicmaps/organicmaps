#include "LocaleTranslator.h"

namespace locale_translator
{
std::string bcp47ToTwineLanguage(NSString const * bcp47)
{
  if (bcp47 == nil || bcp47.length < 2)
    return {};

  // Update this array if new bcp47 languages are added into data/strings/sound.txt
  if ([@[@"pt-BR", @"es-MX"] containsObject:bcp47])
    return bcp47.UTF8String;  // Unchanged original bcp47 string

  // AVFoundation reports Cantonese voices with yue-* BCP47 tags.
  // Note: zh-HK / zh-MO are intentionally NOT mapped to yue-* here — they
  // identify Mandarin (Traditional) HK/MO locales used by category localization
  // (see MWMObjectsCategorySelectorDataSource.mm).
  if ([bcp47 isEqualToString:@"yue-HK"])
    return "yue-HK";
  if ([bcp47 isEqualToString:@"yue-MO"])
    return "yue-MO";
  if ([bcp47 hasPrefix:@"yue"])
    return "yue";

  if ([@[@"zh-CN", @"zh-CHS", @"zh-SG"] containsObject:bcp47])
    return "zh-Hans";  // Mandarin (Simplified)

  if ([bcp47 hasPrefix:@"zh"])
    return "zh-Hant";  // Mandarin (Traditional), incl. HK/MO

  // Taking two first symbols of a language name. For example ru-RU -> ru
  return [bcp47 substringToIndex:2].UTF8String;
}
}  // namespace locale_translator
