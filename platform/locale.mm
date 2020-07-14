#include "platform/locale.hpp"

#import <Foundation/Foundation.h>

namespace platform
{
Locale GetCurrentLocale()
{
  NSLocale * locale = [NSLocale currentLocale];
  return {[locale.languageCode UTF8String], [locale.countryCode UTF8String],
          [locale.currencyCode UTF8String]};
}
}  // namespace platform