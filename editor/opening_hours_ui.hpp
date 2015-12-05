#pragma once

#include "std/set.hpp"
#include "std/vector.hpp"

#include "3party/opening_hours/opening_hours.hpp"

namespace editor
{
namespace ui
{

using TOpeningDays = set<osmoh::Weekday>;

class TimeTable
{
public:
  static TimeTable GetPredefinedTimeTable();

  bool IsTwentyFourHours() const { return m_isTwentyFourHours; }
  void SetTwentyFourHours(bool const on) { m_isTwentyFourHours = on; }

  TOpeningDays const & GetWorkingDays() const { return m_weekdays; }
  bool SetWorkingDays(TOpeningDays const & days);

  void AddWorkingDay(osmoh::Weekday const wd);
  bool RemoveWorkingDay(osmoh::Weekday const wd);

  osmoh::Timespan const & GetOpeningTime() const { return m_openingTime; }
  bool SetOpeningTime(osmoh::Timespan const & span);

  bool AddExcludeTime(osmoh::Timespan const & span);
  bool RemoveExcludeTime(size_t const index);
  osmoh::TTimespans const & GetExcludeTime() const { return m_excludeTime; }

  bool IsValid() const;

  osmoh::Timespan GetPredifinedOpeningTime() const;
  osmoh::Timespan GetPredefinedExcludeTime() const;

private:
  bool m_isTwentyFourHours;
  TOpeningDays m_weekdays;
  osmoh::Timespan m_openingTime;
  osmoh::TTimespans m_excludeTime;
};

class TimeTableSet : public vector<TimeTable>
{
public:
  using vector<TimeTable>::vector;

  TimeTableSet();

  TOpeningDays GetUnhandledDays() const;

  TimeTable GetComplementTimeTable() const;

  bool Append(TimeTable const & tt);
  bool Remove(size_t const index);

  bool Replace(TimeTable const & tt, size_t const index);

private:
  static bool UpdateByIndex(TimeTableSet & ttSet, size_t const index);
};
} // namespace ui
} // namespace editor
