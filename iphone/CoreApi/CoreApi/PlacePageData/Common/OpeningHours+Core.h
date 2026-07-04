#import "OpeningHours.h"

#include "timezone/timezone.hpp"

#include <optional>

NS_ASSUME_NONNULL_BEGIN

@interface OpeningHours (Core)

/// @param timeZone POI's local time zone used to evaluate the current open/closed state (see issue #1642).
- (nullable instancetype)initWithRawString:(NSString *)rawString
                              localization:(id<IOpeningHoursLocalization>)localization
                                  timeZone:(std::optional<om::tz::TimeZone> const &)timeZone;

@end

NS_ASSUME_NONNULL_END
