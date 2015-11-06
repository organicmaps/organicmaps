/*
  The MIT License (MIT)

  Copyright (c) 2015 Mail.Ru Group

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.
*/

#pragma once

#include <string>
#include <vector>
#include <iostream>
#include <chrono>
#include <type_traits>

namespace osmoh
{

class Time
{
  enum State
  {
    IsNotTime = 0,
    HaveHours = 1,
    HaveMinutes = 2,
  };

  using TStateRep = std::underlying_type<State>::type;

public:
  enum class Event
  {
    NotEvent,
    Dawn,
    Sunrise,
    Sunset,
    Dusk
  };

  using THours = std::chrono::hours;
  using TMinutes = std::chrono::minutes;

  Time() = default;
  Time(Time const &) = default;
  Time(THours const hours);
  Time(TMinutes const minutes);

  Time & operator=(Time const &) = default;

  THours::rep GetHoursCount() const;
  TMinutes::rep GetMinutesCount() const;

  THours GetHours() const;
  TMinutes GetMinutes() const;

  void SetHours(THours const hours);
  void SetMinutes(TMinutes const minutes);

  Event GetEvent() const {return m_event;}
  void SetEvent(Event const event);

  bool IsEvent() const;
  bool IsEventOffset() const;
  bool IsHoursMinutes() const;
  bool IsMinutes() const;
  bool IsTime() const;
  bool HasValue() const;

  Time operator+(Time const & t);
  Time operator-(Time const & t);
  Time & operator-();

private:
  Time GetEventTime() const;

  Event m_event{Event::NotEvent};
  TMinutes m_duration{TMinutes::zero()};
  TStateRep m_state{State::IsNotTime};
};

inline constexpr Time::THours operator ""_h(unsigned long long int h)
{
  return Time::THours(h);
}

inline constexpr Time::TMinutes operator ""_min(unsigned long long int m)
{
  return Time::TMinutes(m);
}

std::ostream & operator<<(std::ostream & ost, Time::Event const event);
std::ostream & operator<<(std::ostream & ost, Time const & time);

class Timespan
{
public:
  Timespan() = default;
  Timespan(Timespan const &) = default;
  Timespan(Time const & start, bool plus = false);
  Timespan(Time const & start, Time const & end, bool plus = false);
  Timespan(Time const & start, Time const & end, Time const & period);

  bool IsEmpty() const;
  bool IsOpen() const;
  bool HasStart() const;
  bool HasEnd() const;
  bool HasPlus() const;
  bool HasPeriod() const;

  Time const & GetStart() const;
  Time const & GetEnd() const;
  Time const & GetPeriod() const;

  void SetStart(Time const & start);
  void SetEnd(Time const & end);
  void SetPeriod(Time const & period);
  void SetPlus(bool const plus);

  bool IsValid() const;

private:
  Time m_start;
  Time m_end;
  Time m_period;
  bool m_plus{false};
};

using TTimespans = std::vector<Timespan>;

std::ostream & operator<<(std::ostream & ost, Timespan const & span);
std::ostream & operator<<(std::ostream & ost, osmoh::TTimespans const & timespans);

class NthWeekdayOfTheMonthEntry
{
public:
  enum class NthDayOfTheMonth
  {
    None,
    First,
    Second,
    Third,
    Fourth,
    Fifth
  };

  bool IsEmpty() const;
  bool HasStart() const;
  bool HasEnd() const;

  NthDayOfTheMonth GetStart() const;
  NthDayOfTheMonth GetEnd() const;

  void SetStart(NthDayOfTheMonth const s);
  void SetEnd(NthDayOfTheMonth const e);

private:
  NthDayOfTheMonth m_start{};
  NthDayOfTheMonth m_end{};
};

std::ostream & operator<<(std::ostream & ost, NthWeekdayOfTheMonthEntry const entry);

enum class Weekday
{
  None,
  Sunday,
  Monday,
  Tuesday,
  Wednesday,
  Thursday,
  Friday,
  Saturday
};

inline constexpr Weekday ToWeekday(uint64_t const day)
{
  using TDay = decltype(day);
  return ((day <= static_cast<TDay>(Weekday::None) ||
           day > static_cast<TDay>(Weekday::Saturday))
          ? Weekday::None
          : static_cast<Weekday>(day));
}

inline constexpr Weekday operator ""_day(unsigned long long int day)
{
  return ToWeekday(day);
}

std::ostream & operator<<(std::ostream & ost, Weekday const wday);

class WeekdayRange
{
  using TNths = std::vector<NthWeekdayOfTheMonthEntry>;

public:
  bool HasWday(Weekday const & wday) const;

  bool HasSunday() const;
  bool HasMonday() const;
  bool HasTuesday() const;
  bool HasWednesday() const;
  bool HasThursday() const;
  bool HasFriday() const;
  bool HasSaturday() const;

  bool HasStart() const;
  bool HasEnd() const;
  bool HasOffset() const;
  bool IsEmpty() const;

  Weekday GetStart() const;
  Weekday GetEnd() const;
  size_t GetDaysCount() const;

  void SetStart(Weekday const & wday);
  void SetEnd(Weekday const & wday);

  int32_t GetOffset() const;
  void SetOffset(int32_t const offset);

  bool HasNth() const;
  TNths const & GetNths() const;

  void AddNth(NthWeekdayOfTheMonthEntry const & entry);

private:
  Weekday m_start{};
  Weekday m_end{};
  int32_t m_offset{};
  TNths m_nths;
};

using TWeekdayRanges = std::vector<WeekdayRange>;

std::ostream & operator<<(std::ostream & ost, WeekdayRange const & range);
std::ostream & operator<<(std::ostream & ost, TWeekdayRanges const & ranges);

class Holiday
{
public:
  bool IsPlural() const;
  void SetPlural(bool const plural);

  int32_t GetOffset() const;
  void SetOffset(int32_t const offset);

private:
  bool m_plural{false};
  int32_t m_offset{};
};

using THolidays = std::vector<Holiday>;

std::ostream & operator<<(std::ostream & ost, Holiday const & holiday);
std::ostream & operator<<(std::ostream & ost, THolidays const & holidys);

// Correspond to weekday_selector in osm opening hours.
class Weekdays
{
public:
  bool IsEmpty() const;
  bool HasWeekday() const;
  bool HasHolidays() const;

  TWeekdayRanges const & GetWeekdayRanges() const;
  THolidays const & GetHolidays() const;

  void SetWeekdayRanges(TWeekdayRanges const ranges);
  void SetHolidays(THolidays const & holidays);

  void AddWeekdayRange(WeekdayRange const range);
  void AddHoliday(Holiday const & holiday);

private:
  TWeekdayRanges m_weekdayRanges;
  THolidays m_holidays;
};

std::ostream & operator<<(std::ostream & ost, Weekdays const & weekday);

class DateOffset
{
public:
  bool IsEmpty() const;
  bool HasWDayOffset() const;
  bool HasOffset() const;

  bool IsWDayOffsetPositive() const;

  Weekday GetWDayOffset() const;
  int32_t GetOffset() const;

  void SetWDayOffset(Weekday const wday);
  void SetOffset(int32_t const offset);
  void SetWDayOffsetPositive(bool const on);

private:
  Weekday m_wday_offset{Weekday::None};
  bool m_positive{true};
  int32_t m_offset{};
};

std::ostream & operator<<(std::ostream & ost, DateOffset const & offset);

class MonthDay
{
public:
  enum class Month
  {
    None,
    Jan,
    Feb,
    Mar,
    Apr,
    May,
    Jun,
    Jul,
    Aug,
    Sep,
    Oct,
    Nov,
    Dec
  };

  enum class VariableDate
  {
    None,
    Easter
  };

  using TYear = uint16_t;
  using TDayNum = uint8_t;

  bool IsEmpty() const;
  bool IsVariable() const;

  bool HasYear() const;
  bool HasMonth() const;
  bool HasDayNum() const;
  bool HasOffset() const;

  TYear GetYear() const;
  Month GetMonth() const;
  TDayNum GetDayNum() const;
  DateOffset const & GetOffset() const;
  VariableDate GetVariableDate() const;

  void SetYear(TYear const year);
  void SetMonth(Month const month);
  void SetDayNum(TDayNum const daynum);
  void SetOffset(DateOffset const & offset);
  void SetVariableDate(VariableDate const date);

private:
  TYear m_year{};
  Month m_month{Month::None};
  TDayNum m_daynum{};
  VariableDate m_variable_date{VariableDate::None};
  DateOffset m_offset{};
};

inline constexpr MonthDay::Month ToMonth(uint64_t const month)
{
  using TMonth = decltype(month);
  return ((month <= static_cast<TMonth>(MonthDay::Month::None) ||
           month > static_cast<TMonth>(MonthDay::Month::Dec))
          ? MonthDay::Month::None
          : static_cast<osmoh::MonthDay::Month>(month));
}

inline constexpr MonthDay::Month operator ""_M(unsigned long long int month)
{
  return ToMonth(month);
}

std::ostream & operator<<(std::ostream & ost, MonthDay::Month const month);
std::ostream & operator<<(std::ostream & ost, MonthDay::VariableDate const date);
std::ostream & operator<<(std::ostream & ost, MonthDay const md);

class MonthdayRange
{
public:
  bool IsEmpty() const;
  bool HasStart() const;
  bool HasEnd() const;
  bool HasPeriod() const;
  bool HasPlus() const;

  MonthDay const & GetStart() const;
  MonthDay const & GetEnd() const;
  uint32_t GetPeriod() const;

  void SetStart(MonthDay const & start);
  void SetEnd(MonthDay const & end);
  void SetPeriod(uint32_t const period);
  void SetPlus(bool const plus);

private:
  MonthDay m_start;
  MonthDay m_end;
  uint32_t m_period{};
  bool m_plus{false};
};

using TMonthdayRanges = std::vector<MonthdayRange>;

std::ostream & operator<<(std::ostream & ost, MonthdayRange const & range);
std::ostream & operator<<(std::ostream & ost, TMonthdayRanges const & ranges);

class YearRange
{
public:
  using TYear = uint16_t;

  bool IsEmpty() const;
  bool IsOpen() const;
  bool HasStart() const;
  bool HasEnd() const;
  bool HasPlus() const;
  bool HasPeriod() const;

  TYear GetStart() const;
  TYear GetEnd() const;
  uint32_t GetPeriod() const;

  void SetStart(TYear const start);
  void SetEnd(TYear const end);
  void SetPlus(bool const plus);
  void SetPeriod(uint32_t const period);

private:
  TYear m_start{};
  TYear m_end{};
  bool m_plus{false};
  uint32_t m_period{0};
};

using TYearRanges = std::vector<YearRange>;

std::ostream & operator<<(std::ostream & ost, YearRange const range);
std::ostream & operator<<(std::ostream & ost, TYearRanges const ranges);

class WeekRange
{
public:
  using TWeek = uint8_t;

  bool IsEmpty() const;
  bool IsOpen() const;
  bool HasStart() const;
  bool HasEnd() const;
  bool HasPeriod() const;

  TWeek GetStart() const;
  TWeek GetEnd() const;
  uint32_t GetPeriod() const;

  void SetStart(TWeek const start);
  void SetEnd(TWeek const end);
  void SetPeriod(uint32_t const period);

private:
  TWeek m_start{};
  TWeek m_end{};
  uint32_t m_period{0};
};

using TWeekRanges = std::vector<WeekRange>;

std::ostream & operator<<(std::ostream & ost, WeekRange const range);
std::ostream & operator<<(std::ostream & ost, TWeekRanges const ranges);

class RuleSequence
{
public:
  enum class Modifier
  {
    DefaultOpen,
    Open,
    Closed,
    Unknown,
    Comment
  };

  bool IsEmpty() const;
  bool IsTwentyFourHours() const;

  bool HasYears() const;
  bool HasMonths() const;
  bool HasWeeks() const;
  bool HasWeekdays() const;
  bool HasTimes() const;
  bool HasComment() const;
  bool HasModifierComment() const;
  bool HasSeparatorForReadability() const;

  TYearRanges const & GetYears() const;
  TMonthdayRanges const & GetMonths() const;
  TWeekRanges const & GetWeeks() const;
  Weekdays const & GetWeekdays() const;
  TTimespans const & GetTimes() const;

  std::string const & GetComment() const;
  std::string const & GetModifierComment() const;
  std::string const & GetAnySeparator() const;

  Modifier GetModifier() const;

  void SetTwentyFourHours(bool const on);
  void SetYears(TYearRanges const & years);
  void SetMonths(TMonthdayRanges const & months);
  void SetWeeks(TWeekRanges const & weeks);

  void SetWeekdays(Weekdays const & weekdays);
  void SetTimes(TTimespans const & times);

  void SetComment(std::string const & comment);
  void SetModifierComment(std::string & comment);
  void SetAnySeparator(std::string const & separator);
  void SetSeparatorForReadability(bool const on);

  void SetModifier(Modifier const modifier);

private:
  void dump() const;

  bool m_twentyFourHours{false};

  TYearRanges m_years;
  TMonthdayRanges m_months;
  TWeekRanges m_weeks;

  Weekdays m_weekdays;
  TTimespans m_times;

  std::string m_comment;
  std::string m_anySeparator{";"};
  bool m_separatorForReadability{false};

  Modifier m_modifier{Modifier::DefaultOpen};
  std::string m_modifierComment;
};

using TRuleSequences = std::vector<RuleSequence>;

std::ostream & operator<<(std::ostream & ost, RuleSequence::Modifier const modifier);
std::ostream & operator<<(std::ostream & ost, RuleSequence const & sequence);
std::ostream & operator<<(std::ostream & ost, TRuleSequences const & sequences);
} // namespace osmoh
