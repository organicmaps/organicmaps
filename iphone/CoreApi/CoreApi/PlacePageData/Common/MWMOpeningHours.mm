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
// "08:00—12:00" and "13:00—16:30" (same as Android's Timetable.formatOpenShifts).
NSString * openShifts(ui::TimeTable const & tt, id<IOpeningHoursLocalization> localization)
{
  auto const shift = [&](Time const & start, Time const & end)
  { return [NSString stringWithFormat:@"%@—%@", formatTime(start, localization), formatTime(end, localization)]; };

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

// "Mon" or "Mon-Thu", same as Android's TimeFormatUtils.formatWeekdaysRange.
NSString * weekdaysRange(NSArray<NSString *> * names, Weekday first, Weekday last)
{
  NSString * firstName = names[static_cast<NSUInteger>(first) - 1];
  if (first == last)
    return firstName;
  return [NSString stringWithFormat:@"%@-%@", firstName, names[static_cast<NSUInteger>(last) - 1]];
}

ui::TimeTable const * findTimeTable(ui::TimeTableSet const & timeTables, Weekday day)
{
  for (auto const & tt : timeTables)
    if (tt.GetOpeningDays().contains(day))
      return &tt;
  return nullptr;
}
}  // namespace

namespace osmoh
{

std::optional<std::vector<Day>> processRawString(NSString * str, id<IOpeningHoursLocalization> localization,
                                                 NSInteger today)
{
  osmoh::OpeningHours const oh(str.UTF8String);
  // Open 24/7: the status already says so and there is no week to list (same as Android).
  if (oh.IsTwentyFourHours())
    return std::vector<Day>{};

  ui::TimeTableSet timeTables;
  if (!MakeTimeTableSet(oh, timeTables))
    return std::nullopt;

  // Same as Android's WeekScheduleBuilder: the week starts today, and each row joins the following days
  // that share a time table, or have none and are closed.
  auto const dayAt = [today](NSInteger i) { return static_cast<Weekday>((today - 1 + i) % 7 + 1); };
  NSArray<NSString *> * names = NSCalendar.currentCalendar.shortStandaloneWeekdaySymbols;
  std::vector<Day> days;
  for (NSInteger first = 0, last = 0; first < 7; first = ++last)
  {
    auto const * tt = findTimeTable(timeTables, dayAt(first));
    while (last + 1 < 7 && findTimeTable(timeTables, dayAt(last + 1)) == tt)
      ++last;

    NSString * weekdays = weekdaysRange(names, dayAt(first), dayAt(last));
    NSString * times = nil;
    if (tt)
      times = tt->IsTwentyFourHours() ? localization.allDayString : openShifts(*tt, localization);
    if (times.length > 0)
      days.emplace_back(weekdays, times);
    else
      days.emplace_back(weekdays);
  }
  return days;
}

}  // namespace osmoh
