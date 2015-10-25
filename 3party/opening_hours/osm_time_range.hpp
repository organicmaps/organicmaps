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
  enum EState
  {
    eIsNotTime = 0,
    eHaveHours = 1,
    eHaveMinutes = 2,
  };

  using TEStateRep = std::underlying_type<EState>::type;

 public:
  enum class EEvent
  {
    eNotEvent,
    eDawn,
    eSunrise,
    eSunset,
    eDusk
  };

  using THours = std::chrono::hours;
  using TMinutes = std::chrono::minutes;

 public:
  Time() = default;
  Time(Time const &) = default;
  Time(THours const hours);
  Time(TMinutes const minutes);
  Time(THours const hours, TMinutes const minutes);
  Time(EEvent const event);

  Time & operator=(Time const &) = default;

  THours::rep GetHoursCount() const;
  TMinutes::rep GetMinutesCount() const;

  THours GetHours() const;
  TMinutes GetMinutes() const;

  void SetHours(THours const hours);
  void SetMinutes(TMinutes const minutes);

  EEvent GetEvent() const {return m_event;}
  void SetEvent(EEvent const event);

  bool IsEvent() const;
  bool IsEventOffset() const;
  bool IsHoursMinutes() const;
  bool IsMinutes() const;
  bool IsTime() const;
  bool HasValue() const;

  Time & operator-(Time const & time);
  Time & operator-();

 private:
  Time GetEventTime() const;

 private:
  EEvent m_event{EEvent::eNotEvent};
  TMinutes m_duration{TMinutes::zero()};
  TEStateRep m_state{EState::eIsNotTime};
};

inline constexpr Time::THours operator ""_h(unsigned long long h)
{
  return Time::THours(h);
}

inline constexpr Time::TMinutes operator ""_min(unsigned long long m)
{
  return Time::TMinutes(m);
}

std::ostream & operator<<(std::ostream & ost, Time::EEvent const event);
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

class NthEntry
{
 public:
  enum class ENth
  {
    None,
    First,
    Second,
    Third,
    Fourth,
    Fifth
  };

 public:
  bool IsEmpty() const;
  bool HasStart() const;
  bool HasEnd() const;

  ENth GetStart() const;
  ENth GetEnd() const;

  void SetStart(ENth const s);
  void SetEnd(ENth const e);

 private:
  ENth m_start{};
  ENth m_end{};
};

std::ostream & operator<<(std::ostream & ost, NthEntry const entry);

enum class EWeekday
{
  None,
  Su,
  Mo,
  Tu,
  We,
  Th,
  Fr,
  Sa
};

inline constexpr EWeekday operator ""_day(unsigned long long day)
{
  using TDay = decltype(day);
  return ((day <= static_cast<TDay>(EWeekday::None) ||
           day > static_cast<TDay>(EWeekday::Sa))
          ? EWeekday::None
          : static_cast<EWeekday>(day));
}

std::ostream & operator<<(std::ostream & ost, EWeekday const wday);

class WeekdayRange
{
  using TNths = std::vector<NthEntry>;

 public:

 public:
  bool HasWday(EWeekday const & wday) const;

  bool HasSu() const;
  bool HasMo() const;
  bool HasTu() const;
  bool HasWe() const;
  bool HasTh() const;
  bool HasFr() const;
  bool HasSa() const;

  bool HasStart() const;
  bool HasEnd() const;
  bool IsEmpty() const;

  EWeekday GetStart() const;
  EWeekday GetEnd() const;
  size_t GetDaysCount() const;

  void SetStart(EWeekday const & wday);
  void SetEnd(EWeekday const & wday);

  int32_t GetOffset() const;
  void SetOffset(int32_t const offset);

  bool HasNth() const;
  TNths const & GetNths() const;

  void AddNth(NthEntry const & entry);

 private:
  EWeekday m_start{};
  EWeekday m_end{};
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

class Weekdays // Correspond to weekday_selector in osm opening hours
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

  EWeekday GetWDayOffset() const;
  int32_t GetOffset() const;

  void SetWDayOffset(EWeekday const wday);
  void SetOffset(int32_t const offset);
  void SetWDayOffsetPositive(bool const on);

 private:
  EWeekday m_wday_offset{EWeekday::None};
  bool m_positive{true};
  int32_t m_offset{};
};

std::ostream & operator<<(std::ostream & ost, DateOffset const & offset);

class MonthDay
{
 public:
  enum class EMonth
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

  enum class EVariableDate
  {
    None,
    Easter
  };

  using TYear = uint16_t;
  using TDayNum = uint8_t;

 public:
  bool IsEmpty() const;
  bool IsVariable() const;

  bool HasYear() const;
  bool HasMonth() const;
  bool HasDayNum() const;
  bool HasOffset() const;

  TYear GetYear() const;
  EMonth GetMonth() const;
  TDayNum GetDayNum() const;
  DateOffset const & GetOffset() const;
  EVariableDate GetVariableDate() const;

  void SetYear(TYear const year);
  void SetMonth(EMonth const month);
  void SetDayNum(TDayNum const daynum);
  void SetOffset(DateOffset const & offset);
  void SetVariableDate(EVariableDate const date);

 private:
  TYear m_year{};
  EMonth m_month{EMonth::None};
  TDayNum m_daynum{};
  EVariableDate m_variable_date{EVariableDate::None};
  DateOffset m_offset{};
};

inline constexpr MonthDay::EMonth operator ""_M(unsigned long long month)
{
  using TMonth = decltype(month);
  return ((month <= static_cast<TMonth>(MonthDay::EMonth::None) ||
           month > static_cast<TMonth>(MonthDay::EMonth::Dec))
          ? MonthDay::EMonth::None
          : static_cast<osmoh::MonthDay::EMonth>(month));
}

std::ostream & operator<<(std::ostream & ost, MonthDay::EMonth const month);
std::ostream & operator<<(std::ostream & ost, MonthDay::EVariableDate const date);
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

 public:
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

 public:
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
  // static uint32_t id;
  // uint32_t my_id;
 public:
  enum class Modifier {
    DefaultOpen,
    Unknown,
    Closed,
    Open
  };

 public:
  // RuleSequence()
  // {
  //   ++id;
  //   my_id = id;
  //   std::cout << "RuleSequence(" << my_id << ")" << std::endl;
  // }

  // ~RuleSequence()
  // {
  //   std::cout << "~RuleSequence(" << my_id << ")" << std::endl;
  // }

  bool IsEmpty() const;
  bool Is24Per7() const;

  bool HasYears() const;
  bool HasMonth() const;
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

  void Set24Per7(bool const on);
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

 private:
  bool m_24_per_7{false};

  TYearRanges m_years;
  TMonthdayRanges m_months;
  TWeekRanges m_weeks;

  Weekdays m_weekdays;
  TTimespans m_times;

  std::string m_comment;
  std::string m_any_separator{";"};
  bool m_separator_for_readablility{false};

  Modifier m_modifier{Modifier::DefaultOpen};
  std::string m_modifier_comment;
};

using TRuleSequences = std::vector<RuleSequence>;

std::ostream & operator<<(std::ostream & ost, RuleSequence::Modifier const modifier);
std::ostream & operator<<(std::ostream & ost, RuleSequence const & sequence);
std::ostream & operator<<(std::ostream & ost, TRuleSequences const & sequences);
} // namespace osmoh


// class OSMTimeRange
// {
//  public:
//   OSMTimeRange() = default;

//   bool IsValid() const { return m_valid; }
//   bool IsOpen() const { return m_state == osmoh::State::eOpen; }
//   bool IsClosed() const { return m_state == osmoh::State::eClosed; }
//   bool IsUnknown() const { return m_state == osmoh::State::eUnknown; }
//   std::string const & Comment() const { return m_comment; }

//   OSMTimeRange & UpdateState(time_t timestamp);
//   OSMTimeRange & UpdateState(std::string const & timestr,
//                              char const * timefmt="%d-%m-%Y %R");

//   std::string ToString() const;

//   static OSMTimeRange FromString(std::string const & rules);

//  private:
//   bool m_valid{false};
//   osmoh::State::EState m_state{osmoh::State::eUnknown};

//   osmoh::TTimeRules m_rules;
//   std::string m_comment;
// };
