#pragma once

#include "std/set.hpp"
#include "std/vector.hpp"

#include "3party/opening_hours/opening_hours.hpp"

namespace editor
{
namespace ui
{

struct Time
{
  int hours;
  int minutes;
};

struct Timespan
{
  Time from;
  Time to;
};

enum class OpeningHoursTemplates
{
  TwentyFourHours,
  Weekdays,
  AllDays
};

using TOpeningDays = set<osmoh::Weekday>;

struct TimeTable
{
  bool isAllDay;
  TOpeningDays weekday;
  Timespan openingTime;
  vector<Timespan> excludeTime;
};

using TTimeTables = vector<TimeTable>;

} // namespace ui
} // namespace editor
