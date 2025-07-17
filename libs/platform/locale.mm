#include "platform/locale.hpp"

#import <Foundation/Foundation.h>

namespace platform
{
Locale NSLocale2Locale(NSLocale *locale)
{
  return {locale.languageCode ? [locale.languageCode UTF8String] : "",
          locale.countryCode ? [locale.countryCode UTF8String] : "",
          locale.currencyCode ? [locale.currencyCode UTF8String] : "",
          locale.decimalSeparator ? [locale.decimalSeparator UTF8String] : ".",
          locale.groupingSeparator ? [locale.groupingSeparator UTF8String] : kNarrowNonBreakingSpace};
}

Locale GetCurrentLocale()
{
  return NSLocale2Locale([NSLocale currentLocale]);
}

bool GetLocale(std::string localeName, Locale& result)
{
  NSLocale *loc = [[NSLocale alloc] initWithLocaleIdentifier: @(localeName.c_str())];

  if (!loc)
    return false;

  result = NSLocale2Locale(loc);
  return true;
}
}  // namespace platform
