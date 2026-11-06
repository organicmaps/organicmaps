#import <Foundation/Foundation.h>

#import "IOpeningHoursLocalization.h"

#include <vector>

namespace osmoh
{
struct Day
{
  Day(NSString * workingDays, NSString * workingTimes) : m_workingDays(workingDays), m_workingTimes(workingTimes) {}

  explicit Day(NSString * workingDays) : m_workingDays(workingDays), m_isOpen(false) {}
  NSString * TodayTime() const
  {
    return m_workingTimes ? [NSString stringWithFormat:@"%@ %@", m_workingDays, m_workingTimes] : m_workingDays;
  }

  NSString * m_workingDays;
  NSString * m_workingTimes;
  bool m_isOpen = true;
};

/// @return { Days schedule list, is closed now }
std::pair<std::vector<osmoh::Day>, bool> processRawString(NSString * str, id<IOpeningHoursLocalization> localization);

}  // namespace osmoh
