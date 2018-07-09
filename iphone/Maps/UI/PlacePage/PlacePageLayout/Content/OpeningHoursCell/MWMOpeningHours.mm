#import "MWMOpeningHours.h"
#import "MWMOpeningHoursCommon.h"

#include "3party/opening_hours/opening_hours.hpp"
#include "editor/opening_hours_ui.hpp"
#include "editor/ui2oh.hpp"

using namespace editor;
using namespace osmoh;

namespace
{
NSString * stringFromTimeSpan(Timespan const & timeSpan)
{
  return [NSString stringWithFormat:@"%@ - %@", stringFromTime(timeSpan.GetStart()),
                                    stringFromTime(timeSpan.GetEnd())];
}

NSString * breaksFromClosedTime(TTimespans const & closedTimes)
{
  NSMutableString * breaks = [@"" mutableCopy];
  auto const size = closedTimes.size();
  for (size_t i = 0; i < size; i++)
  {
    if (i)
      [breaks appendString:@"\n"];
    [breaks appendString:[NSString stringWithFormat:@"%@ %@", L(@"editor_hours_closed"),
                                                              stringFromTimeSpan(closedTimes[i])]];
  }
  return [breaks copy];
}

void addToday(ui::TimeTable const & tt, vector<Day> & allDays)
{
  NSString * workingDays;
  NSString * workingTimes;
  NSString * breaks;

  BOOL const everyDay = isEveryDay(tt);
  if (tt.IsTwentyFourHours())
  {
    workingDays = everyDay ? L(@"twentyfour_seven") : L(@"editor_time_allday");
    workingTimes = @"";
    breaks = @"";
  }
  else
  {
    workingDays = everyDay ? L(@"daily") : L(@"today");
    workingTimes = stringFromTimeSpan(tt.GetOpeningTime());
    breaks = breaksFromClosedTime(tt.GetExcludeTime());
  }

  allDays.emplace(allDays.begin(), workingDays, workingTimes, breaks);
}

void addClosedToday(vector<Day> & allDays)
{
  allDays.emplace(allDays.begin(), L(@"day_off_today"));
}

void addDay(ui::TimeTable const & tt, vector<Day> & allDays)
{
  NSString * workingDays = stringFromOpeningDays(tt.GetOpeningDays());
  NSString * workingTimes;
  NSString * breaks;
  if (tt.IsTwentyFourHours())
  {
    workingTimes = L(@"editor_time_allday");
  }
  else
  {
    workingTimes = stringFromTimeSpan(tt.GetOpeningTime());
    breaks = breaksFromClosedTime(tt.GetExcludeTime());
  }
  allDays.emplace_back(workingDays, workingTimes, breaks);
}

void addUnhandledDays(ui::TOpeningDays const & days, vector<Day> & allDays)
{
  if (!days.empty())
    allDays.emplace_back(stringFromOpeningDays(days));
}

}  // namespace

@implementation MWMOpeningHours

+ (vector<Day>)processRawString:(NSString *)str
{
  ui::TimeTableSet timeTableSet;
  osmoh::OpeningHours oh(str.UTF8String);
  if (!MakeTimeTableSet(oh, timeTableSet))
    return {};

  vector<Day> days;

  NSCalendar * cal = NSCalendar.currentCalendar;
  cal.locale = NSLocale.currentLocale;

  auto const timeTablesSize = timeTableSet.Size();
  auto const today =
      static_cast<Weekday>([cal components:NSCalendarUnitWeekday fromDate:[NSDate date]].weekday);
  auto const unhandledDays = timeTableSet.GetUnhandledDays();

  /// Schedule contains more than one rule for all days or unhandled days.
  BOOL const isExtendedSchedule = timeTablesSize != 1 || !unhandledDays.empty();
  BOOL hasCurrentDay = NO;

  for (auto const & tt : timeTableSet)
  {
    ui::TOpeningDays const & workingDays = tt.GetOpeningDays();
    if (workingDays.find(today) != workingDays.end())
    {
      hasCurrentDay = YES;
      addToday(tt, days);
    }

    if (isExtendedSchedule)
      addDay(tt, days);
  }

  if (!hasCurrentDay)
    addClosedToday(days);

  addUnhandledDays(unhandledDays, days);
  return days;
}

@end
