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
  static TimeTable GetUninitializedTimeTable() { return {}; }
  static TimeTable GetPredefinedTimeTable();

  bool IsTwentyFourHours() const { return m_isTwentyFourHours; }
  void SetTwentyFourHours(bool const on) { m_isTwentyFourHours = on; }

  TOpeningDays const & GetOpeningDays() const { return m_weekdays; }
  bool SetOpeningDays(TOpeningDays const & days);

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
  TOpeningDays m_weekdays;
  osmoh::Timespan m_openingTime;
  osmoh::TTimespans m_excludeTime;
};

template <typename TTimeTableSet>
class TimeTableProxyBase : public TimeTable
{
public:
  TimeTableProxyBase(TTimeTableSet & tts, size_t const index, TimeTable const & tt):
      TimeTable(tt),
      m_index(index),
      m_tts(tts)
  {
  }

  bool Commit() { return m_tts.Replace(*this, m_index); } // Slice base class on copy.

private:
  size_t const m_index;
  TTimeTableSet & m_tts;
};

class TimeTableSet;
using TTimeTableProxy = TimeTableProxyBase<TimeTableSet>;

class TimeTableSet
{
  using TTimeTableSetImpl = vector<TimeTable>;

public:
  TimeTableSet();

  TOpeningDays GetUnhandledDays() const;

  TimeTable GetComplementTimeTable() const;

  TTimeTableProxy Get(size_t const index) { return TTimeTableProxy(*this, index, m_table[index]); }
  TTimeTableProxy Front() { return Get(0); }
  TTimeTableProxy Back() { return Get(Size() - 1); }

  size_t Size() const { return m_table.size(); }
  bool Empty() const { return m_table.empty(); }

  bool IsTwentyFourPerSeven() const;

  bool Append(TimeTable const & tt);
  bool Remove(size_t const index);

  bool Replace(TimeTable const & tt, size_t const index);

  TTimeTableSetImpl::const_iterator begin() const { return m_table.begin(); }
  TTimeTableSetImpl::const_iterator end() const { return m_table.end(); }

private:
  static bool UpdateByIndex(TimeTableSet & ttSet, size_t const index);

  TTimeTableSetImpl m_table;
};
} // namespace ui
} // namespace editor
