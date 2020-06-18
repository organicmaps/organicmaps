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

#include <chrono>
#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

// Implemented in accordance with the specification
// https://wiki.openstreetmap.org/wiki/Key:opening_hours/specification

namespace osmoh
{
class HourMinutes
{
public:
  using THours = std::chrono::hours;
  using TMinutes = std::chrono::minutes;

  HourMinutes() = default;
  HourMinutes(THours const duration) { SetDuration(duration); }
  HourMinutes(TMinutes const duration) { SetDuration(duration); }

  bool IsEmpty() const { return m_empty; }
  bool IsExtended() const;

  THours GetHours() const { return m_hours; }
  TMinutes GetMinutes() const { return m_minutes; }
  TMinutes GetDuration() const { return GetMinutes() + GetHours(); }

  THours::rep GetHoursCount() const { return GetHours().count(); }
  TMinutes::rep GetMinutesCount() const { return GetMinutes().count(); }
  TMinutes::rep GetDurationCount() const { return GetDuration().count(); }

  void SetHours(THours const hours);
  void SetMinutes(TMinutes const minutes);
  void SetDuration(TMinutes const duration);

  void AddDuration(TMinutes const duration) { SetDuration(GetDuration() + duration); }

private:
  THours m_hours = THours::zero();
  TMinutes m_minutes = TMinutes::zero();
  bool m_empty = true;
};

HourMinutes operator-(HourMinutes const & hm);
std::ostream & operator<<(std::ostream & ost, HourMinutes const & hm);

inline bool operator<(HourMinutes const & a, HourMinutes const & b)
{
  return a.GetDuration() < b.GetDuration();
}

inline bool operator==(HourMinutes const & a, HourMinutes const & b)
{
  return a.GetDuration() == b.GetDuration();
}

class Time;

class TimeEvent
{
public:
  enum class Event
  {
    None,
    Sunrise,
    Sunset
  };

  TimeEvent() = default;
  TimeEvent(Event const event): m_event(event) {}

  bool IsEmpty() const { return m_event == Event::None; }
  bool HasOffset() const { return !m_offset.IsEmpty(); }

  Event GetEvent() const { return m_event; }
  void SetEvent(Event const event) { m_event = event; }

  HourMinutes const & GetOffset() const { return m_offset; }
  void SetOffset(HourMinutes const & offset) { m_offset = offset; }
  void AddDurationToOffset(HourMinutes::TMinutes const duration) { m_offset.AddDuration(duration); }

  Time GetEventTime() const;

private:
  Event m_event = Event::None;
  HourMinutes m_offset;
};

std::ostream & operator<<(std::ostream & ost, TimeEvent const te);

class Time
{
  enum class Type
  {
    None,
    HourMinutes,
    Event,
  };

 public:
  using THours = HourMinutes::THours;
  using TMinutes = HourMinutes::TMinutes;

  Time() = default;
  Time(HourMinutes const & hm) { SetHourMinutes(hm); }
  Time(TimeEvent const & te) { SetEvent(te); }

  bool IsEmpty() const { return GetType() == Type::None; }
  bool IsTime() const { return IsHoursMinutes() || IsEvent(); }
  bool IsEvent() const { return GetType() == Type::Event; }
  bool IsHoursMinutes() const { return GetType() == Type::HourMinutes; }

  Type GetType() const { return m_type; }

  THours::rep GetHoursCount() const { return GetHours().count(); }
  TMinutes::rep GetMinutesCount() const { return GetMinutes().count(); }

  THours GetHours() const;
  TMinutes GetMinutes() const;

  void AddDuration(TMinutes const duration);

  TimeEvent const & GetEvent() const { return m_event; }
  void SetEvent(TimeEvent const & event);

  HourMinutes const & GetHourMinutes() const { return m_hourMinutes; }
  HourMinutes & GetHourMinutes() { return m_hourMinutes; }
  void SetHourMinutes(HourMinutes const & hm);

 private:
  HourMinutes m_hourMinutes;
  TimeEvent m_event;

  Type m_type = Type::None;
};

inline constexpr Time::THours operator ""_h(unsigned long long int h)
{
  return Time::THours(h);
}

inline constexpr Time::TMinutes operator ""_min(unsigned long long int m)
{
  return Time::TMinutes(m);
}

std::ostream & operator<<(std::ostream & ost, Time const & time);
bool operator==(Time const & lhs, Time const & rhs);

class TimespanPeriod
{
public:
  enum class Type
  {
    None,
    Minutes,
    HourMinutes
  };

  TimespanPeriod() = default;
  TimespanPeriod(HourMinutes const & hm);
  TimespanPeriod(HourMinutes::TMinutes const minutes);

  bool IsEmpty() const { return m_type == Type::None; }
  bool IsHoursMinutes() const { return m_type == Type::HourMinutes; }
  bool IsMinutes() const { return m_type == Type::Minutes; }

  Type GetType() const { return m_type; }

  HourMinutes const & GetHourMinutes() const { return m_hourMinutes; }
  HourMinutes::TMinutes GetMinutes() const { return m_minutes; }
  HourMinutes::TMinutes::rep GetMinutesCount() const { return GetMinutes().count(); }

private:
  HourMinutes::TMinutes m_minutes;
  HourMinutes m_hourMinutes;

  Type m_type = Type::None;
};

std::ostream & operator<<(std::ostream & ost, TimespanPeriod const p);
bool operator==(TimespanPeriod const & lhs, TimespanPeriod const & rhs);

class Timespan
{
public:
  Timespan() = default;
  Timespan(Time const & start, Time const & end): m_start(start), m_end(end) {}
  Timespan(HourMinutes::TMinutes const & start,
           HourMinutes::TMinutes const & end): m_start(start), m_end(end) {}

  bool IsEmpty() const { return !HasStart() && !HasEnd(); }
  bool IsOpen() const { return HasStart() && !HasEnd(); }
  bool HasStart() const { return !GetStart().IsEmpty(); }
  bool HasEnd() const { return !GetEnd().IsEmpty(); }
  bool HasPlus() const { return m_plus; }
  bool HasPeriod() const { return !m_period.IsEmpty(); }
  bool HasExtendedHours() const;

  Time const & GetStart() const { return m_start; }
  Time const & GetEnd() const { return m_end; }

  Time & GetStart() { return m_start; }
  Time & GetEnd() { return m_end; }

  TimespanPeriod const & GetPeriod() const { return m_period; }

  void SetStart(Time const & start) { m_start = start; }
  void SetEnd(Time const & end) { m_end = end; }
  void SetPeriod(TimespanPeriod const & period) { m_period = period; }
  void SetPlus(bool const plus) { m_plus = plus; }

  void ExpandPlus();

private:
  Time m_start;
  Time m_end;
  TimespanPeriod m_period;
  bool m_plus = false;
};

using TTimespans = std::vector<Timespan>;

std::ostream & operator<<(std::ostream & ost, Timespan const & span);
std::ostream & operator<<(std::ostream & ost, osmoh::TTimespans const & timespans);
bool operator==(Timespan const & lhs, Timespan const & rhs);

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

  bool IsEmpty() const { return !HasStart() && !HasEnd(); }
  bool HasStart() const { return GetStart() != NthDayOfTheMonth::None; }
  bool HasEnd() const { return GetEnd() != NthDayOfTheMonth::None; }

  NthDayOfTheMonth GetStart() const { return m_start; }
  NthDayOfTheMonth GetEnd() const { return m_end; }

  void SetStart(NthDayOfTheMonth const s) { m_start = s; }
  void SetEnd(NthDayOfTheMonth const e) { m_end = e; }

  bool operator==(NthWeekdayOfTheMonthEntry const & rhs) const;

private:
  NthDayOfTheMonth m_start = NthDayOfTheMonth::None;
  NthDayOfTheMonth m_end = NthDayOfTheMonth::None;
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

inline constexpr Weekday operator ""_weekday(unsigned long long int day)
{
  return ToWeekday(day);
}

std::ostream & operator<<(std::ostream & ost, Weekday const wday);

class WeekdayRange
{
  using TNths = std::vector<NthWeekdayOfTheMonthEntry>;

public:
  bool HasWday(Weekday const wday) const;

  bool HasSunday() const { return HasWday(Weekday::Sunday); }
  bool HasMonday() const { return HasWday(Weekday::Monday); }
  bool HasTuesday() const { return HasWday(Weekday::Tuesday); }
  bool HasWednesday() const { return HasWday(Weekday::Wednesday); }
  bool HasThursday() const { return HasWday(Weekday::Thursday); }
  bool HasFriday() const { return HasWday(Weekday::Friday); }
  bool HasSaturday() const { return HasWday(Weekday::Saturday); }

  bool HasStart() const { return GetStart() != Weekday::None; }
  bool HasEnd() const  {return GetEnd() != Weekday::None; }
  bool HasOffset() const { return GetOffset() != 0; }
  bool IsEmpty() const { return GetStart() == Weekday::None &&
                                GetEnd() == Weekday::None; }

  Weekday GetStart() const { return m_start; }
  Weekday GetEnd() const { return m_end; }

  void SetStart(Weekday const & wday) { m_start = wday; }
  void SetEnd(Weekday const & wday) { m_end = wday; }

  int32_t GetOffset() const { return m_offset; }
  void SetOffset(int32_t const offset) { m_offset = offset; }

  bool HasNth() const { return !m_nths.empty(); }
  TNths const & GetNths() const { return m_nths; }

  void AddNth(NthWeekdayOfTheMonthEntry const & entry) { m_nths.push_back(entry); }

  bool operator==(WeekdayRange const & rhs) const;

private:
  Weekday m_start = Weekday::None;
  Weekday m_end = Weekday::None;
  int32_t m_offset = 0;
  TNths m_nths;
};

using TWeekdayRanges = std::vector<WeekdayRange>;

std::ostream & operator<<(std::ostream & ost, WeekdayRange const & range);
std::ostream & operator<<(std::ostream & ost, TWeekdayRanges const & ranges);

class Holiday
{
public:
  bool IsPlural() const { return m_plural; }
  void SetPlural(bool const plural) { m_plural = plural; }

  int32_t GetOffset() const { return m_offset; }
  void SetOffset(int32_t const offset) { m_offset = offset; }

  bool operator==(Holiday const & rhs) const;

private:
  bool m_plural = false;
  int32_t m_offset = 0;
};

using THolidays = std::vector<Holiday>;

std::ostream & operator<<(std::ostream & ost, Holiday const & holiday);
std::ostream & operator<<(std::ostream & ost, THolidays const & holidys);

// Correspond to weekday_selector in osm opening hours.
class Weekdays
{
public:
  bool IsEmpty() const { return GetWeekdayRanges().empty() && GetHolidays().empty(); }
  bool HasWeekday() const { return !GetWeekdayRanges().empty(); }
  bool HasHolidays() const { return !GetHolidays().empty(); }

  TWeekdayRanges const & GetWeekdayRanges() const { return m_weekdayRanges; }
  THolidays const & GetHolidays() const { return m_holidays; }

  void SetWeekdayRanges(TWeekdayRanges const ranges) { m_weekdayRanges = ranges; }
  void SetHolidays(THolidays const & holidays) { m_holidays = holidays; }

  void AddWeekdayRange(WeekdayRange const range) { m_weekdayRanges.push_back(range); }
  void AddHoliday(Holiday const & holiday) { m_holidays.push_back(holiday); }

  bool operator==(Weekdays const & rhs) const;

private:
  TWeekdayRanges m_weekdayRanges;
  THolidays m_holidays;
};

std::ostream & operator<<(std::ostream & ost, Weekdays const & weekday);

class DateOffset
{
public:
  bool IsEmpty() const { return !HasOffset() && !HasWDayOffset(); }
  bool HasWDayOffset() const { return m_wdayOffest != Weekday::None; }
  bool HasOffset() const { return m_offset != 0; }

  bool IsWDayOffsetPositive() const { return m_positive; }

  Weekday GetWDayOffset() const { return m_wdayOffest; }
  int32_t GetOffset() const { return m_offset; }

  void SetWDayOffset(Weekday const wday) { m_wdayOffest = wday; }
  void SetOffset(int32_t const offset) { m_offset = offset; }
  void SetWDayOffsetPositive(bool const on) { m_positive = on; }

  bool operator==(DateOffset const & rhs) const;
  bool operator!=(DateOffset const & rhs) const { return !(*this == rhs); }
  bool operator<(DateOffset const & rhs) const;

private:
  Weekday m_wdayOffest = Weekday::None;
  bool m_positive = true;
  int32_t m_offset = 0;
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

  bool IsEmpty() const { return !HasYear() && !HasMonth() && !HasDayNum() && !IsVariable(); }
  bool IsVariable() const { return GetVariableDate() != VariableDate::None; }

  bool HasYear() const { return GetYear() != 0; }
  bool HasMonth() const { return GetMonth() != Month::None; }
  bool HasDayNum() const { return GetDayNum() != 0; }
  bool HasOffset() const { return !GetOffset().IsEmpty(); }

  TYear GetYear() const { return m_year; }
  Month GetMonth() const { return m_month; }
  TDayNum GetDayNum() const { return m_daynum; }
  DateOffset const & GetOffset() const { return m_offset; }
  VariableDate GetVariableDate() const { return m_variable_date; }

  void SetYear(TYear const year) { m_year = year; }
  void SetMonth(Month const month) { m_month = month; }
  void SetDayNum(TDayNum const daynum) { m_daynum = daynum; }
  void SetOffset(DateOffset const & offset) { m_offset = offset; }
  void SetVariableDate(VariableDate const date) { m_variable_date = date; }

  bool operator==(MonthDay const & rhs) const;
  bool operator<(MonthDay const & rhs) const;
  bool operator<=(MonthDay const & rhs) const { return *this < rhs || *this == rhs; }

private:
  TYear m_year = 0;
  Month m_month = Month::None;
  TDayNum m_daynum = 0;
  VariableDate m_variable_date = VariableDate::None;
  DateOffset m_offset;
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
  bool IsEmpty() const { return !HasStart() && !HasEnd(); }
  bool HasStart() const { return !GetStart().IsEmpty(); }
  bool HasEnd() const { return !GetEnd().IsEmpty() || GetEnd().HasDayNum(); }
  bool HasPeriod() const { return m_period != 0; }
  bool HasPlus() const { return m_plus; }

  MonthDay const & GetStart() const { return m_start; }
  MonthDay const & GetEnd() const { return m_end; }
  uint32_t GetPeriod() const { return m_period; }

  void SetStart(MonthDay const & start) { m_start = start; }
  void SetEnd(MonthDay const & end) { m_end = end; }
  void SetPeriod(uint32_t const period) { m_period = period; }
  void SetPlus(bool const plus) { m_plus = plus; }

  bool operator==(MonthdayRange const & rhs) const;

private:
  MonthDay m_start;
  MonthDay m_end;
  uint32_t m_period = 0;
  bool m_plus = false;
};

using TMonthdayRanges = std::vector<MonthdayRange>;

std::ostream & operator<<(std::ostream & ost, MonthdayRange const & range);
std::ostream & operator<<(std::ostream & ost, TMonthdayRanges const & ranges);

class YearRange
{
public:
  using TYear = uint16_t;

  bool IsEmpty() const { return !HasStart() && !HasEnd(); }
  bool IsOpen() const { return HasStart() && !HasEnd(); }
  bool HasStart() const { return GetStart() != 0; }
  bool HasEnd() const { return GetEnd() != 0; }
  bool HasPlus() const { return m_plus; }
  bool HasPeriod() const { return GetPeriod() != 0; }

  TYear GetStart() const { return m_start; }
  TYear GetEnd() const { return m_end; }
  uint32_t GetPeriod() const { return m_period; }

  void SetStart(TYear const start) { m_start = start; }
  void SetEnd(TYear const end) { m_end = end; }
  void SetPlus(bool const plus) { m_plus = plus; }
  void SetPeriod(uint32_t const period) { m_period = period; }

  bool operator==(YearRange const & rhs) const;

private:
  TYear m_start = 0;
  TYear m_end = 0;
  bool m_plus = false;
  uint32_t m_period = 0;
};

using TYearRanges = std::vector<YearRange>;

std::ostream & operator<<(std::ostream & ost, YearRange const range);
std::ostream & operator<<(std::ostream & ost, TYearRanges const ranges);

class WeekRange
{
public:
  using TWeek = uint8_t;

  bool IsEmpty() const { return !HasStart() && !HasEnd(); }
  bool IsOpen() const { return HasStart() && !HasEnd(); }
  bool HasStart() const { return GetStart() != 0; }
  bool HasEnd() const { return GetEnd() != 0; }
  bool HasPeriod() const { return GetPeriod() != 0; }

  TWeek GetStart() const { return m_start; }
  TWeek GetEnd() const { return m_end; }
  uint32_t GetPeriod() const { return m_period; }

  void SetStart(TWeek const start) { m_start = start; }
  void SetEnd(TWeek const end) { m_end = end; }
  void SetPeriod(uint32_t const period) { m_period = period; }

  bool operator==(WeekRange const & rhs) const;

private:
  TWeek m_start = 0;
  TWeek m_end = 0;
  uint32_t m_period = 0;
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

  bool IsEmpty() const { return !HasYears() && !HasMonths() && !HasWeeks() &&
                                !HasWeekdays() && !HasTimes(); }
  bool IsTwentyFourHours() const { return m_twentyFourHours; }

  bool HasYears() const { return !GetYears().empty(); }
  bool HasMonths() const { return !GetMonths().empty(); }
  bool HasMonthDay() const;
  bool HasWeeks() const { return !GetWeeks().empty(); }
  bool HasWeekdays() const { return !GetWeekdays().IsEmpty(); }
  bool HasTimes() const { return !GetTimes().empty(); }
  bool HasComment() const { return !GetComment().empty(); }
  bool HasModifierComment() const { return !GetModifierComment().empty(); }
  bool HasSeparatorForReadability() const { return m_separatorForReadability; }

  TYearRanges const & GetYears() const { return m_years; }
  TMonthdayRanges const & GetMonths() const { return m_months; }
  TWeekRanges const & GetWeeks() const { return m_weeks; }
  Weekdays const & GetWeekdays() const { return m_weekdays; }
  TTimespans const & GetTimes() const { return m_times; }

  std::string const & GetComment() const { return m_comment; }
  std::string const & GetModifierComment() const { return m_modifierComment; }
  std::string const & GetAnySeparator() const { return m_anySeparator; }

  Modifier GetModifier() const { return m_modifier; }

  void SetTwentyFourHours(bool const on) { m_twentyFourHours = on; }
  void SetYears(TYearRanges const & years) { m_years = years; }
  void SetMonths(TMonthdayRanges const & months) { m_months = months; }
  void SetWeeks(TWeekRanges const & weeks) { m_weeks = weeks; }

  void SetWeekdays(Weekdays const & weekdays) { m_weekdays = weekdays; }
  void SetTimes(TTimespans const & times) { m_times = times; }

  void SetComment(std::string const & comment) { m_comment = comment; }
  void SetModifierComment(std::string & comment) { m_modifierComment = comment; }
  void SetAnySeparator(std::string const & separator) { m_anySeparator = separator; }
  void SetSeparatorForReadability(bool const on) { m_separatorForReadability = on; }

  void SetModifier(Modifier const modifier) { m_modifier = modifier; }

  bool operator==(RuleSequence const & rhs) const;

private:
  bool m_twentyFourHours{false};

  TYearRanges m_years;
  TMonthdayRanges m_months;
  TWeekRanges m_weeks;

  Weekdays m_weekdays;
  TTimespans m_times;

  std::string m_comment;
  std::string m_anySeparator = ";";
  bool m_separatorForReadability = false;

  Modifier m_modifier = Modifier::DefaultOpen;
  std::string m_modifierComment;
};

using TRuleSequences = std::vector<RuleSequence>;

std::ostream & operator<<(std::ostream & ost, RuleSequence::Modifier const modifier);
std::ostream & operator<<(std::ostream & ost, RuleSequence const & sequence);
std::ostream & operator<<(std::ostream & ost, TRuleSequences const & sequences);

class OpeningHours
{
public:
  OpeningHours() = default;
  OpeningHours(std::string const & rule);
  OpeningHours(TRuleSequences const & rule);

  bool IsOpen(time_t const dateTime) const;
  bool IsClosed(time_t const dateTime) const;
  bool IsUnknown(time_t const dateTime) const;

  bool IsValid() const;

  bool IsTwentyFourHours() const;
  bool HasWeekdaySelector() const;
  bool HasMonthSelector() const;
  bool HasWeekSelector() const;
  bool HasYearSelector() const;

  TRuleSequences const & GetRule() const { return m_rule; }

  friend void swap(OpeningHours & lhs, OpeningHours & rhs);

  bool operator==(OpeningHours const & rhs) const;

private:
  TRuleSequences m_rule;
  bool m_valid = false;
};

std::ostream & operator<<(std::ostream & ost, OpeningHours const & oh);
std::string ToString(osmoh::OpeningHours const & openingHours);
} // namespace osmoh
