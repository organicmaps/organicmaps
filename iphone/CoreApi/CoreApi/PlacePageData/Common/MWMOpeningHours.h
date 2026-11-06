#import <Foundation/Foundation.h>

#import "IOpeningHoursLocalization.h"

#include <optional>
#include <vector>

namespace osmoh
{
struct Day
{
  Day(NSString * workingDays, NSString * workingTimes) : m_workingDays(workingDays), m_workingTimes(workingTimes) {}

  explicit Day(NSString * workingDays) : m_workingDays(workingDays), m_isOpen(false) {}

  NSString * m_workingDays;
  NSString * m_workingTimes;
  bool m_isOpen = true;
};

/// @param today 1 = Sunday … 7 = Saturday, as in NSCalendar.
/// @return Rows of the week starting from today, a single 24/7 row, or nothing if the hours can't be listed.
std::optional<std::vector<osmoh::Day>> processRawString(NSString * str, id<IOpeningHoursLocalization> localization,
                                                        NSInteger today);

}  // namespace osmoh
