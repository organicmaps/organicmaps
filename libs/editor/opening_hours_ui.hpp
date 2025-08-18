#pragma once

#include <set>
#include <vector>

#include "3party/opening_hours/opening_hours.hpp"

namespace editor
{
namespace ui
{
using OpeningDays = std::set<osmoh::Weekday>;

class TimeTable
{
public:
  static TimeTable GetUninitializedTimeTable() { return {}; }
  static TimeTable GetPredefinedTimeTable();

  bool IsTwentyFourHours() const { return m_isTwentyFourHours; }
  void SetTwentyFourHours(bool const on) { m_isTwentyFourHours = on; }

  OpeningDays const & GetOpeningDays() const { return m_weekdays; }
  bool SetOpeningDays(OpeningDays const & days);

  void AddWorkingDay(osmoh::Weekday const wd);
  bool RemoveWorkingDay(osmoh::Weekday const wd);

  osmoh::Timespan const & GetOpeningTime() const { return m_openingTime; }
  bool SetOpeningTime(osmoh::Timespan const & span);

  bool CanAddExcludeTime() const;
  bool AddExcludeTime(osmoh::Timespan const & span);
  bool ReplaceExcludeTime(osmoh::Timespan const & span, size_t const index);
  bool RemoveExcludeTime(size_t const index);
  osmoh::TTimespans const & GetExcludeTime() const { return m_excludeTime; }

  bool IsValid() const;

  osmoh::Timespan GetPredefinedOpeningTime() const;
  osmoh::Timespan GetPredefinedExcludeTime() const;

private:
  TimeTable() = default;

  bool m_isTwentyFourHours;
  OpeningDays m_weekdays;
  osmoh::Timespan m_openingTime;
  osmoh::TTimespans m_excludeTime;
};

class TimeTableSet
{
  using TimeTableSetImpl = std::vector<TimeTable>;

public:
  class Proxy : public TimeTable
  {
  public:
    Proxy(TimeTableSet & tts, size_t const index, TimeTable const & tt) : TimeTable(tt), m_index(index), m_tts(tts) {}

    bool Commit() { return m_tts.Replace(*this, m_index); }  // Slice base class on copy.

  private:
    size_t const m_index;
    TimeTableSet & m_tts;
  };

  TimeTableSet();

  OpeningDays GetUnhandledDays() const;

  TimeTable GetComplementTimeTable() const;

  Proxy Get(size_t const index) { return Proxy(*this, index, m_table[index]); }
  Proxy Front() { return Get(0); }
  Proxy Back() { return Get(Size() - 1); }

  size_t Size() const { return m_table.size(); }
  bool Empty() const { return m_table.empty(); }

  bool IsTwentyFourPerSeven() const;

  bool Append(TimeTable const & tt);
  bool Remove(size_t const index);

  bool Replace(TimeTable const & tt, size_t const index);

  TimeTableSetImpl::const_iterator begin() const { return m_table.begin(); }
  TimeTableSetImpl::const_iterator end() const { return m_table.end(); }

private:
  static bool UpdateByIndex(TimeTableSet & ttSet, size_t const index);

  TimeTableSetImpl m_table;
};
}  // namespace ui
}  // namespace editor
