#include "LocaleTranslator.h"

#include "platform/preferred_languages.hpp"

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

  // Mandarin. The script subtag wins over the region, so this also reads the tags the system hands
  // out for a preferred language, e.g. "zh-Hans-CN", which a plain "zh" prefix test cannot tell from
  // Traditional.
  switch (languages::GetChineseScript(bcp47.UTF8String))
  {
  case languages::ChineseScript::Simplified: return "zh-Hans";
  case languages::ChineseScript::Traditional: return "zh-Hant";
  case languages::ChineseScript::NotChinese: break;
  }

  // The primary subtag of everything else, e.g. ru-RU -> ru. Only a two-letter one: no twine language
  // has a longer code, and truncating would turn "fil" (Filipino) into "fi" (Finnish).
  NSString * primarySubtag =
      [bcp47 componentsSeparatedByCharactersInSet:[NSCharacterSet characterSetWithCharactersInString:@"-_"]]
          .firstObject;
  return primarySubtag.length == 2 ? primarySubtag.lowercaseString.UTF8String : std::string();
}
}  // namespace locale_translator
