#include "LocaleTranslator.h"

#include "platform/preferred_languages.hpp"

namespace locale_translator
{
std::string bcp47ToTwineLanguage(NSString const * bcp47)
{
  if (bcp47 == nil || bcp47.length < 2)
    return {};

  // Foundation separates the subtags of a locale identifier with an underscore ("pt_BR") where a
  // BCP-47 tag uses a hyphen, and both kinds reach this function: AVSpeechSynthesisVoice reports
  // tags, NSLocale identifiers.
  NSString * tag = [bcp47 stringByReplacingOccurrencesOfString:@"_" withString:@"-"];

  // Update this array if new bcp47 languages are added into data/strings/sound.txt
  if ([@[@"pt-BR", @"es-MX"] containsObject:tag])
    return tag.UTF8String;  // Unchanged original bcp47 string

  // AVFoundation reports Cantonese voices with yue-* BCP47 tags.
  // Note: zh-HK / zh-MO are intentionally NOT mapped to yue-* here — they
  // identify Mandarin (Traditional) HK/MO locales used by category localization
  // (see MWMObjectsCategorySelectorDataSource.mm).
  if ([tag isEqualToString:@"yue-HK"])
    return "yue-HK";
  if ([tag isEqualToString:@"yue-MO"])
    return "yue-MO";
  if ([tag hasPrefix:@"yue"])
    return "yue";

  // Mandarin. The script subtag wins over the region, so this also reads the tags the system hands
  // out for a preferred language, e.g. "zh-Hans-CN", which a plain "zh" prefix test cannot tell from
  // Traditional.
  switch (languages::GetChineseScript(tag.UTF8String))
  {
  case languages::ChineseScript::Simplified: return "zh-Hans";
  case languages::ChineseScript::Traditional: return "zh-Hant";
  case languages::ChineseScript::NotChinese: break;
  }

  // The primary subtag of everything else, e.g. ru-RU -> ru. Only a two-letter one: no twine language
  // has a longer code, and truncating would turn "fil" (Filipino) into "fi" (Finnish).
  NSString * primarySubtag = [tag componentsSeparatedByString:@"-"].firstObject;
  return primarySubtag.length == 2 ? primarySubtag.lowercaseString.UTF8String : std::string();
}
}  // namespace locale_translator
