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

namespace osmoh
{

class HourMinutes
{
public:
  using THours = std::chrono::hours;
  using TMinutes = std::chrono::minutes;

  HourMinutes() = default;
  explicit HourMinutes(THours const duration);
  explicit HourMinutes(TMinutes const duration);

  bool IsEmpty() const;

  THours GetHours() const;
  TMinutes GetMinutes() const;
  TMinutes GetDuration() const;

  THours::rep GetHoursCount() const;
  TMinutes::rep GetMinutesCount() const;
  TMinutes::rep GetDurationCount() const;

  void SetHours(THours const hours);
  void SetMinutes(TMinutes const minutes);
  void SetDuration(TMinutes const duration);

  void AddDuration(TMinutes const duration);

private:
  THours m_hours = THours::zero();
  TMinutes m_minutes = TMinutes::zero();
  bool m_empty = true;
};

HourMinutes operator-(HourMinutes const & hm);
std::ostream & operator<<(std::ostream & ost, HourMinutes const & hm);

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
  TimeEvent(Event const event);

  bool IsEmpty() const;
  bool HasOffset() const;

  Event GetEvent() const;
  void SetEvent(Event const event);

  HourMinutes const & GetOffset() const;
  void SetOffset(HourMinutes const & offset);
  void AddDurationToOffset(HourMinutes::TMinutes const duration);

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
  Time(HourMinutes const & hm);
  Time(TimeEvent const & te);

  Type GetType() const;

  THours::rep GetHoursCount() const;
  TMinutes::rep GetMinutesCount() const;

  THours GetHours() const;
  TMinutes GetMinutes() const;

  void AddDuration(TMinutes const duration);

  TimeEvent const & GetEvent() const;
  void SetEvent(TimeEvent const & event);

  HourMinutes const & GetHourMinutes() const;
  void SetHourMinutes(HourMinutes const & hm);

  bool IsEmpty() const;
  bool IsTime() const;
  bool IsEvent() const;
  bool IsHoursMinutes() const;

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

  bool IsEmpty() const;
  bool IsHoursMinutes() const;
  bool IsMinutes() const;

  HourMinutes const & GetHourMinutes() const;
  HourMinutes::TMinutes GetMinutes() const;
  HourMinutes::TMinutes::rep GetMinutesCount() const;

private:
  HourMinutes::TMinutes m_minutes;
  HourMinutes m_hourMinutes;

  Type m_type = Type::None;
};

std::ostream & operator<<(std::ostream & ost, TimespanPeriod const p);

class Timespan
{
public:
  bool IsEmpty() const;
  bool IsOpen() const;
  bool HasStart() const;
  bool HasEnd() const;
  bool HasPlus() const;
  bool HasPeriod() const;

  Time const & GetStart() const;
  Time const & GetEnd() const;
  TimespanPeriod const & GetPeriod() const;

  void SetStart(Time const & start);
  void SetEnd(Time const & end);
  void SetPeriod(TimespanPeriod const & period);
  void SetPlus(bool const plus);

  bool IsValid() const;

private:
  Time m_start;
  Time m_end;
  TimespanPeriod m_period;
  bool m_plus = false;
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

  void SetStart(Weekday const & wday);
  void SetEnd(Weekday const & wday);

  int32_t GetOffset() const;
  void SetOffset(int32_t const offset);

  bool HasNth() const;
  TNths const & GetNths() const;

  void AddNth(NthWeekdayOfTheMonthEntry const & entry);

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
  bool IsPlural() const;
  void SetPlural(bool const plural);

  int32_t GetOffset() const;
  void SetOffset(int32_t const offset);

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
  OpeningHours(std::string const & rule);
  OpeningHours(TRuleSequences const & rule);

  bool IsOpen(time_t const dateTime) const;
  bool IsClosed(time_t const dateTime) const;
  bool IsUnknown(time_t const dateTime) const;

  bool IsValid() const;

private:
  TRuleSequences m_rule;
  bool const m_valid;
};
} // namespace osmoh
