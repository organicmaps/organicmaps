#include "LocaleTranslator.h"

namespace locale_translator
{

std::string bcp47ToTwineLanguage(NSString const * bcp47LangName)
{
  if (bcp47LangName == nil || [bcp47LangName length] < 2)
    return {};

  if ([bcp47LangName isEqualToString:@"zh-CN"] || [bcp47LangName isEqualToString:@"zh-CHS"]
      || [bcp47LangName isEqualToString:@"zh-SG"])
  {
    return "zh-Hans"; // Chinese simplified
  }

  if ([bcp47LangName hasPrefix:@"zh"])
    return "zh-Hant"; // Chinese traditional

  // Taking two first symbols of a language name. For example ru-RU -> ru
  return [bcp47LangName substringToIndex:2].UTF8String;
}
} // namespace locale_translator
