#include "platform/locale.hpp"

#import <Foundation/Foundation.h>

namespace platform
{
Locale GetCurrentLocale()
{
  NSLocale * locale = [NSLocale currentLocale];
  return {locale.languageCode ? [locale.languageCode UTF8String] : "",
          locale.countryCode ? [locale.countryCode UTF8String] : "",
          locale.currencyCode ? [locale.currencyCode UTF8String] : ""};
}
}  // namespace platform
