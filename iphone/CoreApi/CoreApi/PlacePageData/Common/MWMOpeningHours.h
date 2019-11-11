#import <Foundation/Foundation.h>

#import "IOpeningHoursLocalization.h"

#include <vector>

namespace osmoh
{
struct Day
{
  Day(NSString * workingDays, NSString * workingTimes, NSString * breaks)
    : m_workingDays(workingDays), m_workingTimes(workingTimes), m_breaks(breaks)
  {
  }

  explicit Day(NSString * workingDays) : m_workingDays(workingDays), m_isOpen(false) {}
  NSString * TodayTime() const
  {
    return m_workingTimes ? [NSString stringWithFormat:@"%@ %@", m_workingDays, m_workingTimes]
                          : m_workingDays;
  }

  NSString * m_workingDays;
  NSString * m_workingTimes;
  NSString * m_breaks;
  bool m_isOpen = true;
};

std::vector<osmoh::Day> processRawString(NSString *str, id<IOpeningHoursLocalization> localization);

}  // namespace osmoh
