#include "platform/localization.hpp"

#include <algorithm>

#import <Foundation/Foundation.h>

namespace platform
{
namespace
{
class AppleStringCollator final : public StringCollator
{
public:
  explicit AppleStringCollator(std::string const & locale)
    : m_locale([[NSLocale alloc] initWithLocaleIdentifier:@(locale.c_str())])
  {}

  bool Less(std::string const & lhs, std::string const & rhs) const override
  {
    NSString * left = @(lhs.c_str());
    NSString * right = @(rhs.c_str());
    return [left compare:right options:0 range:NSMakeRange(0, left.length) locale:m_locale] == NSOrderedAscending;
  }

private:
  NSLocale * __strong m_locale;
};
}  // namespace

std::unique_ptr<StringCollator> CreateStringCollator(std::string const & locale)
{
  return std::make_unique<AppleStringCollator>(locale);
}

std::string GetLocalizedTypeName(std::string const & type)
{
  auto key = "type." + type;
  std::replace(key.begin(), key.end(), '-', '.');
  std::replace(key.begin(), key.end(), ':', '_');

  return [NSLocalizedStringFromTableInBundle(@(key.c_str()), @"LocalizableTypes", NSBundle.mainBundle, @"") UTF8String];
}

std::string GetLocalizedBrandName(std::string const & brand)
{
  auto const key = "brand." + brand;
  return
      [NSLocalizedStringWithDefaultValue(@(key.c_str()), nil, NSBundle.mainBundle, @(brand.c_str()), @"") UTF8String];
}

std::string GetLocalizedString(std::string const & key)
{
  return [NSLocalizedString(@(key.c_str()), @"") UTF8String];
}

std::string GetCurrencySymbol(std::string const & currencyCode)
{
  NSLocale * locale = [NSLocale currentLocale];
  NSString * symbol = [locale displayNameForKey:NSLocaleCurrencySymbol value:@(currencyCode.c_str())];
  if (!symbol)
    return currencyCode;

  return [symbol UTF8String];
}

std::string GetLocalizedMyPositionBookmarkName()
{
  NSDate * now = [NSDate date];
  return [NSDateFormatter localizedStringFromDate:now
                                        dateStyle:NSDateFormatterLongStyle
                                        timeStyle:NSDateFormatterShortStyle]
      .UTF8String;
}
}  // namespace platform
