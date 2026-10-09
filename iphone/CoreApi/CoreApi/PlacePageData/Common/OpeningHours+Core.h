#import "OpeningHours.h"

#include "timezone/timezone.hpp"

#include <optional>

NS_ASSUME_NONNULL_BEGIN

@interface OpeningHours (Core)

/// @param timeZone POI's local time zone for the current state and "today" row.
- (nullable instancetype)initWithRawString:(NSString *)rawString
                              localization:(id<IOpeningHoursLocalization>)localization
                                  timeZone:(std::optional<om::tz::TimeZone> const &)timeZone;

@end

NS_ASSUME_NONNULL_END
