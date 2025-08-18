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

  if ([@[@"zh-CN", @"zh-CHS", @"zh-SG"] containsObject:bcp47])
    return "zh-Hans";  // Chinese simplified

  if ([bcp47 hasPrefix:@"zh"])
    return "zh-Hant";  // Chinese traditional

  // Taking two first symbols of a language name. For example ru-RU -> ru
  return [bcp47 substringToIndex:2].UTF8String;
}
}  // namespace locale_translator
