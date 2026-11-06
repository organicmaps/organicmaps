#import "MWMOpeningHours.h"
#import "MWMOpeningHoursCommon.h"

#include "editor/ui2oh.hpp"

using namespace editor;
using namespace osmoh;

namespace
{
// Shows the noon and midnight labels instead of 12:00 and 00:00/24:00. In 12-hour locales both
// render as "12:00 PM" / "12:00 AM", so the labels are the only way to tell them apart at a
// glance. Only whole hours are labelled: 00:30 and 12:30 stay numeric. Sun events (sunrise-sunset)
// have no clock value here, and their 00:00 placeholder must not be labelled as midnight.
NSString * formatTime(Time const & time, id<IOpeningHoursLocalization> localization)
{
  if (time.IsHoursMinutes() && time.GetMinutesCount() == 0)
  {
    auto const hours = time.GetHoursCount();
    if (hours == 12)
      return localization.noonString;
    if (hours == 0 || hours == 24)
      return localization.midnightString;
  }
  return stringFromTime(time);
}

// Splits the opening time into open shifts around its breaks, one per line, e.g. a lunch break gives
// "08:00 - 12:00" and "13:00 - 16:30" (same as Android's Timetable.formatOpenShifts).
NSString * openShifts(ui::TimeTable const & tt, id<IOpeningHoursLocalization> localization)
{
  auto const shift = [&](Time const & start, Time const & end)
  { return [NSString stringWithFormat:@"%@ - %@", formatTime(start, localization), formatTime(end, localization)]; };

  // Equal bounds without breaks are a whole 24-hour span, e.g. 08:00-08:00, not an empty shift.
  if (tt.GetExcludeTime().empty())
    return shift(tt.GetOpeningTime().GetStart(), tt.GetOpeningTime().GetEnd());

  NSMutableArray<NSString *> * shifts = [NSMutableArray array];
  auto const appendShift = [&](Time const & start, Time const & end)
  {
    // Drop only empty shifts. A start later than end is a valid overnight shift, e.g. 23:00-04:00.
    if (start == end)
      return;
    [shifts addObject:shift(start, end)];
  };

  Time shiftStart = tt.GetOpeningTime().GetStart();
  for (auto const & closed : tt.GetExcludeTime())
  {
    appendShift(shiftStart, closed.GetStart());
    shiftStart = closed.GetEnd();
  }
  appendShift(shiftStart, tt.GetOpeningTime().GetEnd());
  return [shifts componentsJoinedByString:@"\n"];
}

void addToday(ui::TimeTable const & tt, std::vector<Day> & allDays, id<IOpeningHoursLocalization> localization)
{
  NSString * workingDays;
  NSString * workingTimes;

  BOOL const everyDay = isEveryDay(tt);
  if (tt.IsTwentyFourHours())
  {
    workingDays = everyDay ? localization.twentyFourSevenString : localization.allDayString;
    workingTimes = @"";
  }
  else
  {
    workingDays = everyDay ? localization.dailyString : localization.todayString;
    workingTimes = openShifts(tt, localization);
  }

  allDays.emplace(allDays.begin(), workingDays, workingTimes);
}

void addClosedToday(std::vector<Day> & allDays, id<IOpeningHoursLocalization> localization)
{
  allDays.emplace(allDays.begin(), localization.dayOffString);
}

void addDay(ui::TimeTable const & tt, std::vector<Day> & allDays, id<IOpeningHoursLocalization> localization)
{
  NSString * workingDays = stringFromOpeningDays(tt.GetOpeningDays());
  NSString * workingTimes = tt.IsTwentyFourHours() ? localization.allDayString : openShifts(tt, localization);
  allDays.emplace_back(workingDays, workingTimes);
}

void addUnhandledDays(ui::OpeningDays const & days, std::vector<Day> & allDays)
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

  ui::TimeTableSet timeTableSet;
  if (!MakeTimeTableSet(oh, timeTableSet))
    return {{}, isClosed};

  std::vector<Day> days;

  NSCalendar * cal = NSCalendar.currentCalendar;
  cal.locale = NSLocale.currentLocale;

  auto const timeTablesSize = timeTableSet.Size();
  auto const today = static_cast<Weekday>([cal components:NSCalendarUnitWeekday fromDate:[NSDate date]].weekday);
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
