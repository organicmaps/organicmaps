#import "MWMOpeningHours.h"
#import "MWMOpeningHoursCommon.h"

#include "editor/ui2oh.hpp"

namespace
{
NSString * stringFromTimeSpan(osmoh::Timespan const & timeSpan)
{
  return [NSString stringWithFormat:@"%@ - %@", stringFromTime(timeSpan.GetStart()), stringFromTime(timeSpan.GetEnd())];
}

NSString * breaksFromClosedTime(osmoh::TTimespans const & closedTimes, id<IOpeningHoursLocalization> localization)
{
  NSMutableString * breaks = [@"" mutableCopy];
  auto const size = closedTimes.size();
  for (size_t i = 0; i < size; i++)
  {
    if (i)
      [breaks appendString:@"\n"];
    [breaks appendString:[NSString
                             stringWithFormat:@"%@ %@", localization.breakString, stringFromTimeSpan(closedTimes[i])]];
  }
  return [breaks copy];
}

void addToday(editor::ui::TimeTable const & tt, std::vector<Day> & allDays, id<IOpeningHoursLocalization> localization)
{
  NSString * workingDays;
  NSString * workingTimes;
  NSString * breaks;

  BOOL const everyDay = isEveryDay(tt);
  if (tt.IsTwentyFourHours())
  {
    workingDays = everyDay ? localization.twentyFourSevenString : localization.allDayString;
    workingTimes = @"";
    breaks = @"";
  }
  else
  {
    workingDays = everyDay ? localization.dailyString : localization.todayString;
    workingTimes = stringFromTimeSpan(tt.GetOpeningTime());
    breaks = breaksFromClosedTime(tt.GetExcludeTime(), localization);
  }

  allDays.emplace(allDays.begin(), workingDays, workingTimes, breaks);
}

void addClosedToday(std::vector<Day> & allDays, id<IOpeningHoursLocalization> localization)
{
  allDays.emplace(allDays.begin(), localization.dayOffString);
}

void addDay(editor::ui::TimeTable const & tt, std::vector<Day> & allDays, id<IOpeningHoursLocalization> localization)
{
  NSString * workingDays = stringFromOpeningDays(tt.GetOpeningDays());
  NSString * workingTimes;
  NSString * breaks;
  if (tt.IsTwentyFourHours())
  {
    workingTimes = localization.allDayString;
  }
  else
  {
    workingTimes = stringFromTimeSpan(tt.GetOpeningTime());
    breaks = breaksFromClosedTime(tt.GetExcludeTime(), localization);
  }
  allDays.emplace_back(workingDays, workingTimes, breaks);
}

void addUnhandledDays(editor::ui::OpeningDays const & days, std::vector<Day> & allDays)
{
  if (!days.empty())
    allDays.emplace_back(stringFromOpeningDays(days));
}

}  // namespace

namespace osmoh
{

std::pair<std::vector<osmoh::Day>, bool> processRawString(NSString * str, id<IOpeningHoursLocalization> localization)
{
  osmoh::OpeningHours oh(str.UTF8String);
  bool const isClosed = oh.IsClosed(time(nullptr));

  editor::ui::TimeTableSet timeTableSet;
  if (!editor::MakeTimeTableSet(oh, timeTableSet))
    return {{}, isClosed};

  std::vector<Day> days;

  NSCalendar * cal = NSCalendar.currentCalendar;
  cal.locale = NSLocale.currentLocale;

  auto const timeTablesSize = timeTableSet.Size();
  auto const today = static_cast<osmoh::Weekday>([cal components:NSCalendarUnitWeekday fromDate:[NSDate date]].weekday);
  auto const unhandledDays = timeTableSet.GetUnhandledDays();

  /// Schedule contains more than one rule for all days or unhandled days.
  BOOL const isExtendedSchedule = timeTablesSize != 1 || !unhandledDays.empty();
  BOOL hasCurrentDay = NO;

  for (auto const & tt : timeTableSet)
  {
    auto const & workingDays = tt.GetOpeningDays();
    if (workingDays.find(today) != workingDays.end())
    {
      hasCurrentDay = YES;
      addToday(tt, days, localization);
    }

    if (isExtendedSchedule)
      addDay(tt, days, localization);
  }

  if (!hasCurrentDay)
    addClosedToday(days, localization);

  addUnhandledDays(unhandledDays, days);
  return {std::move(days), isClosed};
}

}  // namespace osmoh
