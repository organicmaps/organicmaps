#import <Foundation/Foundation.h>

#import "IOpeningHoursLocalization.h"

#include "timezone/timezone.hpp"

#include <optional>
#include <vector>

namespace osmoh
{
struct Day
{
  Day(NSString * workingDays, NSString * workingTimes, NSString * breaks)
    : m_workingDays(workingDays)
    , m_workingTimes(workingTimes)
    , m_breaks(breaks)
  {}

  explicit Day(NSString * workingDays) : m_workingDays(workingDays), m_isOpen(false) {}
  NSString * TodayTime() const
  {
    return m_workingTimes ? [NSString stringWithFormat:@"%@ %@", m_workingDays, m_workingTimes] : m_workingDays;
  }

  NSString * m_workingDays;
  NSString * m_workingTimes;
  NSString * m_breaks;
  bool m_isOpen = true;
};

/// @return { Days schedule list, is closed now }
/// @param timeZone POI's local time zone used to evaluate the "is closed now" flag (see issue #1642).
std::pair<std::vector<osmoh::Day>, bool> processRawString(NSString * str, id<IOpeningHoursLocalization> localization,
                                                          std::optional<om::tz::TimeZone> const & timeZone);

}  // namespace osmoh
