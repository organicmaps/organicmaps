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

#include "osm_time_range.hpp"

#include <iomanip>
#include <ios>
#include <cstdlib>
#include <codecvt>
#include <vector>
#include <ostream>
#include <functional>

namespace
{

template <typename T, typename SeparatorExtrauctor>
void PrintVector(std::ostream & ost, std::vector<T> const & v,
                 SeparatorExtrauctor && sepFunc)
{
  auto it = begin(v);
  if (it == end(v))
    return;

  auto sep = sepFunc(*it);
  ost << *it++;
  while (it != end(v))
  {
    ost << sep << *it;
    sep = sepFunc(*it);
    ++it;
  }
}

template <typename T>
void PrintVector(std::ostream & ost, std::vector<T> const & v, char const * const sep = ", ")
{
  PrintVector(ost, v, [&sep](T const &) { return sep; });
}


void PrintOffset(std::ostream & ost, int32_t const offset, bool const space)
{
  if (offset == 0)
    return;

  if (space)
    ost << ' ';
  if (offset > 0)
    ost << '+';
  ost << offset;
  ost << ' ' << "day";
  if (std::abs(offset) > 1)
    ost << 's';
}

class StreamFlagsKeeper
{
 public:
  StreamFlagsKeeper(std::ostream & ost):
      m_ost(ost),
      m_flags(m_ost.flags())
  {
  }

  ~StreamFlagsKeeper()
  {
    m_ost.flags(m_flags);
  }

 private:
  std::ostream & m_ost;
  std::ios_base::fmtflags m_flags;
};

void PrintPaddedNumber(std::ostream & ost, uint32_t const number, uint32_t const padding = 1)
{
  StreamFlagsKeeper keeper{ost};
  ost << std::setw(padding) << std::setfill('0') << number;
}

} // namespace

namespace osmoh
{

Time::Time(THours const hours)
{
  SetHours(hours);
}

Time::Time(TMinutes const minutes)
{
  SetMinutes(minutes);
}

// Time::Time(THours const hours, TMinutes const minutes) { }
// Time::Time(EEvent const event, THours const hours = 0, TMinutes const minutes = 0) { }

Time::THours::rep Time::GetHoursCount() const
{
  return GetHours().count();
}

Time::TMinutes::rep Time::GetMinutesCount() const
{
  return GetMinutes().count();
}

Time::THours Time::GetHours() const
{
  if (IsEvent())
    return GetEventTime().GetHours();
  else if (IsEventOffset())
    return (GetEventTime() - *this).GetHours();
  return std::chrono::duration_cast<THours>(m_duration);
}

Time::TMinutes Time::GetMinutes() const
{
  if (IsEvent())
    return GetEventTime().GetMinutes();
  else if (IsEventOffset())
    return (GetEventTime() - *this).GetMinutes();
  return std::chrono::duration_cast<TMinutes>(m_duration) - GetHours();
}

void Time::SetHours(THours const hours)
{
  m_state |= eHaveHours | eHaveMinutes;
  m_duration = hours;
}

void Time::SetMinutes(TMinutes const minutes)
{
  m_state |= eHaveMinutes;
  m_duration = minutes;
  if (m_duration > 1_h || m_duration < -1_h)
    m_state |= eHaveHours;
}

void Time::SetEvent(EEvent const event)
{
  m_event = event;
}

bool Time::IsEvent() const
{
  return GetEvent() != EEvent::eNotEvent;
}

bool Time::IsEventOffset() const
{
  return IsEvent() && m_state != eIsNotTime;
}

bool Time::IsHoursMinutes() const
{
  return !IsEvent() && ((m_state & eHaveHours) && (m_state & eHaveMinutes));
}

bool Time::IsMinutes() const
{
  return !IsEvent() && ((m_state & eHaveMinutes) && !(m_state & eHaveHours));
}

bool Time::IsTime() const
{
  return IsHoursMinutes() || IsEvent();
}

bool Time::HasValue() const
{
  return IsEvent() || IsTime() || IsMinutes();
}

Time Time::operator+(Time const & t)
{
  Time result = *this;
  result.SetMinutes(m_duration + t.m_duration);
  return result;
}

Time Time::operator-(Time const & t)
{
  Time result = *this;
  result.SetMinutes(m_duration - t.m_duration);
  return result;
}

Time & Time::operator-()
{
  m_duration = -m_duration;
  return *this;
}

Time Time::GetEventTime() const {return {};}; // TODO(mgsergio): get real time

std::ostream & operator<<(std::ostream & ost, Time::EEvent const event)
{
  switch (event)
  {
    case Time::EEvent::eNotEvent:
      ost << "NotEvent";
      break;
    case Time::EEvent::eSunrise:
      ost << "sunrise";
      break;
    case Time::EEvent::eSunset:
      ost << "sunset";
      break;
    case Time::EEvent::eDawn:
      ost << "dawn";
      break;
    case Time::EEvent::eDusk:
      ost << "dusk";
      break;
  }
  return ost;
}

std::ostream & operator<<(std::ostream & ost, Time const & time)
{
  if (!time.HasValue())
  {
    ost << "hh:mm";
    return ost;
  }

  auto const minutes = time.GetMinutesCount();
  auto const hours = time.GetHoursCount();
  if (time.IsEvent())
  {
    if (time.IsEventOffset())
    {
      ost << '(' << time.GetEvent();
      if (hours < 0)
        ost << '-';
      else
        ost << '+';
      PrintPaddedNumber(ost, std::abs(hours), 2);
      ost << ':';
      PrintPaddedNumber(ost, std::abs(minutes), 2);
      ost << ')';
    }
    else
      ost << time.GetEvent();
  }
  else if (time.IsMinutes())
    PrintPaddedNumber(ost, std::abs(minutes), 2);
  else
  {
    PrintPaddedNumber(ost, std::abs(hours), 2);
    ost << ':';
    PrintPaddedNumber(ost, std::abs(minutes), 2);
  }

  return ost;
}


bool Timespan::IsEmpty() const
{
  return !HasStart() && !HasEnd();
}

bool Timespan::IsOpen() const
{
  return GetStart().HasValue() && !GetEnd().HasValue();
}

bool Timespan::HasStart() const
{
  return GetStart().HasValue();
}

bool Timespan::HasEnd() const
{
  return GetEnd().HasValue();
}

bool Timespan::HasPlus() const
{
  return m_plus;
}

bool Timespan::HasPeriod() const
{
  return m_period.HasValue();
}

Time const & Timespan::GetStart() const
{
  return m_start;
}

Time const & Timespan::GetEnd() const
{
  return m_end;
}

Time const & Timespan::GetPeriod() const
{
  return m_period;
}

void Timespan::SetStart(Time const & start)
{
  m_start = start;
}

void Timespan::SetEnd(Time const & end)
{
  m_end = end;
}

void Timespan::SetPeriod(Time const & period)
{
  m_period = period;
}

void Timespan::SetPlus(bool const plus)
{
  m_plus = plus;
}

bool Timespan::IsValid() const
{
  return false; // TODO(mgsergio): implement validator
}

std::ostream & operator<<(std::ostream & ost, Timespan const & span)
{
  ost << span.GetStart();
  if (!span.IsOpen())
  {
    ost << '-' << span.GetEnd();
    if (span.HasPeriod())
      ost << '/' << span.GetPeriod();
  }
  if (span.HasPlus())
    ost << '+';
  return ost;
}

std::ostream & operator<<(std::ostream & ost, osmoh::TTimespans const & timespans)
{
  PrintVector(ost, timespans);
  return ost;
}


bool NthEntry::IsEmpty() const
{
  return !HasStart() && !HasEnd();
}

bool NthEntry::HasStart() const
{
  return GetStart() != ENth::None;
}

bool NthEntry::HasEnd() const
{
  return GetEnd() != ENth::None;
}

NthEntry::ENth NthEntry::GetStart() const
{
  return m_start;
}

NthEntry::ENth NthEntry::GetEnd() const
{
  return m_end;
}

void NthEntry::SetStart(ENth const s)
{
  m_start = s;
}

void NthEntry::SetEnd(ENth const e)
{
  m_end = e;
}

std::ostream & operator<<(std::ostream & ost, NthEntry const entry)
{
  if (entry.HasStart())
    ost << static_cast<uint32_t>(entry.GetStart());
  if (entry.HasEnd())
    ost << '-' << static_cast<uint32_t>(entry.GetEnd());
  return ost;
}

bool WeekdayRange::HasWday(EWeekday const & wday) const
{
  if (IsEmpty() || wday == EWeekday::None)
    return false;

  if (!HasEnd())
    return GetStart() == wday;

  return GetStart() <= wday && wday <= GetEnd();
}

bool WeekdayRange::HasSu() const { return HasWday(EWeekday::Su); }
bool WeekdayRange::HasMo() const { return HasWday(EWeekday::Mo); }
bool WeekdayRange::HasTu() const { return HasWday(EWeekday::Tu); }
bool WeekdayRange::HasWe() const { return HasWday(EWeekday::We); }
bool WeekdayRange::HasTh() const { return HasWday(EWeekday::Th); }
bool WeekdayRange::HasFr() const { return HasWday(EWeekday::Fr); }
bool WeekdayRange::HasSa() const { return HasWday(EWeekday::Sa); }

bool WeekdayRange::HasStart() const
{
  return GetStart() != EWeekday::None;
}

bool WeekdayRange::HasEnd() const
{
  return GetEnd() != EWeekday::None;;
}

bool WeekdayRange::IsEmpty() const
{
  return GetStart() == EWeekday::None && GetEnd() == EWeekday::None;
}

EWeekday WeekdayRange::GetStart() const
{
  return m_start;
}

EWeekday WeekdayRange::GetEnd() const
{
  return m_end;
}

size_t WeekdayRange::GetDaysCount() const
{
  if (IsEmpty())
    return 0;
  return static_cast<uint32_t>(m_start) - static_cast<uint32_t>(m_end) + 1;
}

void WeekdayRange::SetStart(EWeekday const & wday)
{
  m_start = wday;
}

void WeekdayRange::SetEnd(EWeekday const & wday)
{
  m_end = wday;
}

int32_t WeekdayRange::GetOffset() const
{
  return m_offset;
}

void WeekdayRange::SetOffset(int32_t const offset)
{
  m_offset = offset;
}

bool WeekdayRange::HasNth() const
{
  return !m_nths.empty();
}

WeekdayRange::TNths const & WeekdayRange::GetNths() const
{
  return m_nths;
}

void WeekdayRange::AddNth(NthEntry const & entry)
{
  m_nths.push_back(entry);
}

std::ostream & operator<<(std::ostream & ost, EWeekday const wday)
{
  switch (wday)
  {
    case EWeekday::Su:
      ost << "Su";
      break;
    case EWeekday::Mo:
      ost << "Mo";
      break;
    case EWeekday::Tu:
      ost << "Tu";
      break;
    case EWeekday::We:
      ost << "We";
      break;
    case EWeekday::Th:
      ost << "Th";
      break;
    case EWeekday::Fr:
      ost << "Fr";
      break;
    case EWeekday::Sa:
      ost << "Sa";
      break;
    case EWeekday::None:
      ost << "not-a-day";
  }
  return ost;
}

std::ostream & operator<<(std::ostream & ost, WeekdayRange const & range)
{
  ost << range.GetStart();
  if (range.HasEnd())
    ost << '-' << range.GetEnd();
  else
  {
    if (range.HasNth())
    {
      ost << '[';
      PrintVector(ost, range.GetNths(), ",");
      ost << ']';
    }
    PrintOffset(ost, range.GetOffset(), true);
  }
  return ost;
}

std::ostream & operator<<(std::ostream & ost, TWeekdayRanges const & ranges)
{
  PrintVector(ost, ranges);
  return ost;
}


bool Holiday::IsPlural() const
{
  return m_plural;
}

void Holiday::SetPlural(bool const plural)
{
  m_plural = plural;
}

int32_t Holiday::GetOffset() const
{
  return m_offset;
}

void Holiday::SetOffset(int32_t const offset)
{
  m_offset = offset;
}

std::ostream & operator<<(std::ostream & ost, Holiday const & holiday)
{
  if (holiday.IsPlural())
    ost << "PH";
  else
  {
    ost << "SH";
    PrintOffset(ost, holiday.GetOffset(), true);
  }
  return ost;
}

std::ostream & operator<<(std::ostream & ost, THolidays const & holidays)
{
  PrintVector(ost, holidays);
  return ost;
}


bool Weekdays::IsEmpty() const
{
  return GetWeekdayRanges().empty() && GetHolidays().empty();
}

bool Weekdays::HasWeekday() const
{
  return !GetWeekdayRanges().empty();
}

bool Weekdays::HasHolidays() const
{
  return !GetHolidays().empty();
}

TWeekdayRanges const & Weekdays::GetWeekdayRanges() const
{
  return m_weekdayRanges;
}

THolidays const & Weekdays::GetHolidays() const
{
  return m_holidays;
}

void Weekdays::SetWeekdayRanges(TWeekdayRanges const ranges)
{
  m_weekdayRanges = ranges;
}

void Weekdays::SetHolidays(THolidays const & holidays)
{
  m_holidays = holidays;
}

void Weekdays::AddWeekdayRange(WeekdayRange const range)
{
  m_weekdayRanges.push_back(range);
}

void Weekdays::AddHoliday(Holiday const & holiday)
{
  m_holidays.push_back(holiday);
}

std::ostream & operator<<(std::ostream & ost, Weekdays const & weekday)
{
  ost << weekday.GetHolidays();
  if (weekday.HasWeekday() && weekday.HasHolidays())
    ost << ", ";
  ost << weekday.GetWeekdayRanges();
  return ost;
}


bool DateOffset::IsEmpty() const
{
  return !HasOffset() && !HasWDayOffset();
}

bool DateOffset::HasWDayOffset() const
{
  return m_wday_offset != EWeekday::None;
}

bool DateOffset::HasOffset() const
{
  return m_offset != 0;
}

bool DateOffset::IsWDayOffsetPositive() const
{
  return m_positive;
}

EWeekday DateOffset::GetWDayOffset() const
{
  return m_wday_offset;
}

int32_t DateOffset::GetOffset() const
{
  return m_offset;
}

void DateOffset::SetWDayOffset(EWeekday const wday)
{
  m_wday_offset = wday;
}

void DateOffset::SetOffset(int32_t const offset)
{
  m_offset = offset;
}

void DateOffset::SetWDayOffsetPositive(bool const on)
{
  m_positive = on;
}

std::ostream & operator<<(std::ostream & ost, DateOffset const & offset)
{
  if (offset.HasWDayOffset())
    ost << (offset.IsWDayOffsetPositive() ? '+' : '-')
        << offset.GetWDayOffset();
  PrintOffset(ost, offset.GetOffset(), offset.HasWDayOffset());
  return ost;
}


bool MonthDay::IsEmpty() const
{
  return !HasYear() && !HasMonth() && !HasDayNum() && !IsVariable();
}

bool MonthDay::IsVariable() const
{
  return GetVariableDate() != EVariableDate::None;
}

bool MonthDay::HasYear() const
{
  return GetYear() != 0;
}

bool MonthDay::HasMonth() const
{
  return GetMonth() != EMonth::None;
}

bool MonthDay::HasDayNum() const
{
  return GetDayNum() != 0;
}

bool MonthDay::HasOffset() const
{
  return !GetOffset().IsEmpty();
}

MonthDay::TYear MonthDay::GetYear() const
{
  return m_year;
}

MonthDay::EMonth MonthDay::GetMonth() const
{
  return m_month;
}

MonthDay::TDayNum MonthDay::GetDayNum() const
{
  return m_daynum;
}

DateOffset const & MonthDay::GetOffset() const
{
  return m_offset;
}

MonthDay::EVariableDate MonthDay::GetVariableDate() const
{
  return m_variable_date;
}

void MonthDay::SetYear(TYear const year)
{
  m_year = year;
}

void MonthDay::SetMonth(EMonth const month)
{
  m_month = month;
}

void MonthDay::SetDayNum(TDayNum const daynum)
{
  m_daynum = daynum;
}

void MonthDay::SetOffset(DateOffset const & offset)
{
  m_offset = offset;
}

void MonthDay::SetVariableDate(EVariableDate const date)
{
  m_variable_date = date;
}

std::ostream & operator<<(std::ostream & ost, MonthDay::EMonth const month)
{
  switch (month)
  {
    case MonthDay::EMonth::None:
      ost << "None";
      break;
    case MonthDay::EMonth::Jan:
      ost << "Jan";
      break;
    case MonthDay::EMonth::Feb:
      ost << "Feb";
      break;
    case MonthDay::EMonth::Mar:
      ost << "Mar";
      break;
    case MonthDay::EMonth::Apr:
      ost << "Apr";
      break;
    case MonthDay::EMonth::May:
      ost << "May";
      break;
    case MonthDay::EMonth::Jun:
      ost << "Jun";
      break;
    case MonthDay::EMonth::Jul:
      ost << "Jul";
      break;
    case MonthDay::EMonth::Aug:
      ost << "Aug";
      break;
    case MonthDay::EMonth::Sep:
      ost << "Sep";
      break;
    case MonthDay::EMonth::Oct:
      ost << "Oct";
      break;
    case MonthDay::EMonth::Nov:
      ost << "Nov";
      break;
    case MonthDay::EMonth::Dec:
      ost << "Dec";
      break;
  }
  return ost;
}

std::ostream & operator<<(std::ostream & ost, MonthDay::EVariableDate const date)
{
  switch (date)
  {
    case MonthDay::EVariableDate::None:
      ost << "none";
      break;
    case MonthDay::EVariableDate::Easter:
      ost << "easter";
      break;
  }
  return ost;
}

std::ostream & operator<<(std::ostream & ost, MonthDay const md)
{
  if (md.HasYear())
    ost << md.GetYear() << ' ';

  if (md.IsVariable())
    ost << md.GetVariableDate();
  else
  {
    if (md.HasMonth())
      ost << md.GetMonth();
    if (md.HasDayNum())
    {
      ost << ' ';
      PrintPaddedNumber(ost, md.GetDayNum(), 2);
    }

  }
  if (md.HasOffset())
    ost << ' ' << md.GetOffset();
  return ost;
}


bool MonthdayRange::IsEmpty() const
{
  return !HasStart() && !HasEnd();
}

bool MonthdayRange::HasStart() const
{
  return !m_start.IsEmpty();
}

bool MonthdayRange::HasEnd() const
{
  return !m_end.IsEmpty();
}

bool MonthdayRange::HasPeriod() const
{
  return m_period != 0;
}

bool MonthdayRange::HasPlus() const
{
  return m_plus;
}

MonthDay const & MonthdayRange::GetStart() const
{
  return m_start;
}

MonthDay const & MonthdayRange::GetEnd() const
{
  return m_end;
}

uint32_t MonthdayRange::GetPeriod() const
{
  return m_period;
}

void MonthdayRange::SetStart(MonthDay const & start)
{
  m_start = start;
}

void MonthdayRange::SetEnd(MonthDay const & end)
{
  m_end = end;
}

void MonthdayRange::SetPeriod(uint32_t const period)
{
  m_period = period;
}

void MonthdayRange::SetPlus(bool const plus)
{
  m_plus = plus;
}

std::ostream & operator<<(std::ostream & ost, MonthdayRange const & range)
{
  if (range.HasStart())
    ost << range.GetStart();
  if (range.HasEnd())
  {
    ost << '-' << range.GetEnd();
    if (range.HasPeriod())
      ost << '/' << range.GetPeriod();
  }
  else if (range.HasPlus())
    ost << '+';
  return ost;
}

std::ostream & operator<<(std::ostream & ost, TMonthdayRanges const & ranges)
{
  PrintVector(ost, ranges);
  return ost;
}


bool YearRange::IsEmpty() const
{
  return !HasStart() && !HasEnd();
}

bool YearRange::IsOpen() const
{
  return HasStart() && !HasEnd();
}

bool YearRange::HasStart() const
{
  return GetStart() != 0;
}

bool YearRange::HasEnd() const
{
  return GetEnd() != 0;
}

bool YearRange::HasPlus() const
{
  return m_plus;
}

bool YearRange::HasPeriod() const
{
  return GetPeriod() != 0;
}

YearRange::TYear YearRange::GetStart() const
{
  return m_start;
}

YearRange::TYear YearRange::GetEnd() const
{
  return m_end;
}

uint32_t YearRange::GetPeriod() const
{
  return m_period;
}

void YearRange::SetStart(TYear const start)
{
  m_start = start;
}

void YearRange::SetEnd(TYear const end)
{
  m_end = end;
}

void YearRange::SetPlus(bool const plus)
{
  m_plus = plus;
}

void YearRange::SetPeriod(uint32_t const period)
{
  m_period = period;
}

std::ostream & operator<<(std::ostream & ost, YearRange const range)
{
  if (range.IsEmpty())
    return ost;

  ost << range.GetStart();
  if (range.HasEnd())
  {
    ost << '-' << range.GetEnd();
    if (range.HasPeriod())
      ost << '/' << range.GetPeriod();
  }
  else if (range.HasPlus())
    ost << '+';
  return ost;
}

std::ostream & operator<<(std::ostream & ost, TYearRanges const ranges)
{
  PrintVector(ost, ranges);
  return ost;
}


bool WeekRange::IsEmpty() const
{
  return !HasStart() && !HasEnd();
}

bool WeekRange::IsOpen() const
{
  return HasStart() && !HasEnd();
}

bool WeekRange::HasStart() const
{
  return GetStart() != 0;
}

bool WeekRange::HasEnd() const
{
  return GetEnd() != 0;
}

bool WeekRange::HasPeriod() const
{
  return GetPeriod() != 0;
}

WeekRange::TWeek WeekRange::GetStart() const
{
  return m_start;
}

WeekRange::TWeek WeekRange::GetEnd() const
{
  return m_end;
}

uint32_t WeekRange::GetPeriod() const
{
  return m_period;
}

void WeekRange::SetStart(TWeek const start)
{
  m_start = start;
}

void WeekRange::SetEnd(TWeek const end)
{
  m_end = end;
}

void WeekRange::SetPeriod(uint32_t const period)
{
  m_period = period;
}

std::ostream & operator<<(std::ostream & ost, WeekRange const range)
{
  if (range.IsEmpty())
    return ost;

  PrintPaddedNumber(ost, range.GetStart(), 2);
  if (range.HasEnd())
  {
    ost << '-';
    PrintPaddedNumber(ost, range.GetEnd(), 2);
    if (range.HasPeriod())
      ost << '/' << range.GetPeriod();
  }
  return ost;
}

std::ostream & operator<<(std::ostream & ost, TWeekRanges const ranges)
{
  ost << "week ";
  PrintVector(ost, ranges);
  return ost;
}


bool RuleSequence::IsEmpty() const
{
  return (!HasYears() && !HasMonth() &&
          !HasWeeks() && !HasWeekdays() &&
          !HasTimes());
}

bool RuleSequence::Is24Per7() const
{
  return m_24_per_7;
}

bool RuleSequence::HasYears() const
{
  return !GetYears().empty();
}

bool RuleSequence::HasMonth() const
{
  return !GetMonths().empty();
}

bool RuleSequence::HasWeeks() const
{
  return !GetWeeks().empty();
}

bool RuleSequence::HasWeekdays() const
{
  return !GetWeekdays().IsEmpty();
}

bool RuleSequence::HasTimes() const
{
  return !GetTimes().empty();
}

bool RuleSequence::HasComment() const
{
  return !GetComment().empty();
}

bool RuleSequence::HasModifierComment() const
{
  return !GetModifierComment().empty();
}

bool RuleSequence::HasSeparatorForReadability() const
{
  return m_separator_for_readablility;
}

TYearRanges const & RuleSequence::GetYears() const
{
  return m_years;
}

TMonthdayRanges const & RuleSequence::GetMonths() const
{
  return m_months;
}

TWeekRanges const & RuleSequence::GetWeeks() const
{
  return m_weeks;
}

Weekdays const & RuleSequence::GetWeekdays() const
{
  return m_weekdays;
}

TTimespans const & RuleSequence::GetTimes() const
{
  return m_times;
}

std::string const & RuleSequence::GetComment() const
{
  return m_comment;
}

std::string const & RuleSequence::GetModifierComment() const
{
  return m_modifier_comment;
}

std::string const & RuleSequence::GetAnySeparator() const
{
  return m_any_separator;
}

RuleSequence::Modifier RuleSequence::GetModifier() const
{
  return m_modifier;
}

void RuleSequence::Set24Per7(bool const on)
{
  // std::cout << "Set24Per7: " << on << '\n';
  m_24_per_7 = on;
  // dump();
}

void RuleSequence::SetYears(TYearRanges const & years)
{
  // std::cout << "SetYears: " << years << '\n';
  m_years = years;
  // dump();
}

void RuleSequence::SetMonths(TMonthdayRanges const & months)
{
  // std::cout << "SetMonths: " << months << '\n';
  m_months = months;
  // dump();
}

void RuleSequence::SetWeeks(TWeekRanges const & weeks)
{
  // std::cout << "SetWeeks: " << weeks << '\n';
  m_weeks = weeks;
  // dump();
}

void RuleSequence::SetWeekdays(Weekdays const & weekdays)
{
  // std::cout << "SetWeekdays: " << weekdays << '\n';
  m_weekdays = weekdays;
  // dump();
}

void RuleSequence::SetTimes(TTimespans const & times)
{
  // std::cout << "SetTimes: " << times << '\n';
  m_times = times;
  // dump();
}

void RuleSequence::SetComment(std::string const & comment)
{
  // std::cout << "SetComment: " << comment << '\n';
  m_comment = comment;
  // dump();
}

void RuleSequence::SetModifierComment(std::string & comment)
{
  // std::cout << "SetModifierComment: " << comment << '\n';
  m_modifier_comment = comment;
  // dump();
}

void RuleSequence::SetAnySeparator(std::string const & separator)
{
  // std::cout << "SetAnySeparator: " << separator << '\n';
  m_any_separator = separator;
  // dump();
}

void RuleSequence::SetSeparatorForReadability(bool const on)
{
  // std::cout << "SetSeparatorForReadability: " << on << '\n';
  m_separator_for_readablility = on;
  // dump();
}

void RuleSequence::SetModifier(Modifier const modifier)
{
  // std::cout << "SetModifier " ;//<< modifier << '\n';
  m_modifier = modifier;
  // dump();
}

// uint32_t RuleSequence::id{};
// void RuleSequence::dump() const
// {
//   std::cout << "My id: " << my_id << '\n'
//             << "Years " << GetYears().size() << '\n'
//             << "Months " << GetMonths().size() << '\n'
//             << "Weeks " << GetWeeks().size() << '\n'
//             << "Holidays " << GetWeekdays().GetHolidays().size() << '\n'
//             << "Weekdays " << GetWeekdays().GetWeekdayRanges().size() << '\n'
//             << "Times " << GetTimes().size() << std::endl;
// }

std::ostream & operator<<(std::ostream & ost, RuleSequence::Modifier const modifier)
{
  switch (modifier)
  {
    case RuleSequence::Modifier::DefaultOpen:
      break;
    case RuleSequence::Modifier::Unknown:
      ost << "unknown";
      break;
    case RuleSequence::Modifier::Closed:
      ost << "closed";
      break;
    case RuleSequence::Modifier::Open:
      ost << "open";
      break;
  }
  return ost;
}

std::ostream & operator<<(std::ostream & ost, RuleSequence const & s)
{

  if (s.Is24Per7())
    ost << "24/7";
  else
  {
    if (s.HasComment())
      ost << s.GetComment() << ':';
    else
    {
      bool space = false;
      auto const putSpace = [&space, &ost] {
        if (space)
          ost << ' ';
        space = true;
      };

      if (s.HasYears())
      {
        putSpace();
        ost << s.GetYears();
      }
      if (s.HasMonth())
      {
        putSpace();
        ost << s.GetMonths();
      }
      if (s.HasWeeks())
      {
        putSpace();
        ost << s.GetWeeks();
      }

      if (s.HasSeparatorForReadability())
      {
        space = false;
        ost << ':';
      }

      if (s.HasWeekdays())
      {
        putSpace();
        ost << s.GetWeekdays();
      }
      if (s.HasTimes())
      {
        putSpace();
        ost << s.GetTimes();
      }
    }
  }
  if (s.GetModifier() != RuleSequence::Modifier::DefaultOpen)
    ost << ' ' << s.GetModifier();

  return ost;
}

std::ostream & operator<<(std::ostream & ost, TRuleSequences const & s)
{
  PrintVector(ost, s, [](RuleSequence const & r) {
      auto const sep = r.GetAnySeparator();
      return (sep == "||" ? ' ' + sep + ' ' : sep + ' ');
    });
  return ost;
}

// std::ostream & operator << (std::ostream & s, State const & w)
// {
//   static char const * st[] = {"unknown", "closed", "open"};
//   s << ' ' << st[w.state] << " " << w.comment;
//   return s;
// }

// std::ostream & operator << (std::ostream & s, TimeRule const & w)
// {
//   for (auto const & e : w.weekdays)
//     s << e;
//   if (!w.weekdays.empty() && !w.timespan.empty())
//     s << ' ';
//   for (auto const & e : w.timespan)
//     s << e;

//   std::cout << "Weekdays size " << w.weekdays.size() <<
//       " Timespan size " << w.timespan.size() << std::endl;
//   return s << w.state;
// }
} // namespace osmoh


// namespace
// {
// bool check_timespan(osmoh::TimeSpan const &ts, boost::gregorian::date const & d, boost::posix_time::ptime const & p)
// {
//   using boost::gregorian::days;
//   using boost::posix_time::ptime;
//   using boost::posix_time::hours;
//   using boost::posix_time::minutes;
//   using boost::posix_time::time_period;

//   time_period tp1 = osmoh::make_time_period(d-days(1), ts);
//   time_period tp2 = osmoh::make_time_period(d, ts);
//   /* very useful in debug */
//   //    std::cout << ts << "\t" << tp1 << "(" << p << ")" << (tp1.contains(p) ? " hit" : " miss") << std::endl;
//   //    std::cout << ts << "\t" << tp2 << "(" << p << ")" << (tp2.contains(p) ? " hit" : " miss") << std::endl;
//   return tp1.contains(p) || tp2.contains(p);
// }

// bool check_weekday(osmoh::Weekday const & wd, boost::gregorian::date const & d)
// {
//   using namespace boost::gregorian;

//   bool hit = false;
//   typedef nth_day_of_the_week_in_month nth_dow;
//   if (wd.nth)
//   {
//     for (uint8_t i = 0; (wd.weekdays & (0xFF ^ ((1 << i) - 1))); ++i)
//     {
//       if (!(wd.weekdays & (1 << i)))
//         continue;

//       uint8_t a = wd.nth & 0xFF;
//       for (size_t j = 0; (a & (0xFF ^ ((1 << j) - 1))); ++j)
//       {
//         if (a & (1 << j))
//         {
//           nth_dow ndm(nth_dow::week_num(j + 1), nth_dow::day_of_week_type((i + 1 == 7) ? 0 : (i + 1)), d.month());
//           hit |= (d == ndm.get_date(d.year()));
//         }
//       }
//       a = (wd.nth >> 8) & 0xFF;
//       for (size_t j = 0; (a & (0xFF ^ ((1 << j) - 1))); ++j)
//       {
//         if (a & (1 << j))
//         {
//           last_day_of_the_week_in_month lwdm(nth_dow::day_of_week_type((i + 1 == 7) ? 0 : (i + 1)), d.month());
//           hit |= (d == ((lwdm.get_date(d.year()) - weeks(j)) + days(wd.offset)));
//         }
//       }
//     }
//   }
//   else
//   {
//     for (uint8_t i = 0; (wd.weekdays & (0xFF ^ ((1 << i) - 1))); ++i)
//     {
//       if (!(wd.weekdays & (1 << i)))
//         continue;
//       hit |= (d.day_of_week() == ((i + 1 == 7) ? 0 : (i + 1)));
//     }
//   }
//   /* very useful in debug */
//   //    std::cout << d.day_of_week() << " " <<  d << " --> " << wd << (hit ? " hit" : " miss") << std::endl;
//   return hit;
// }

// bool check_rule(osmoh::TimeRule const & r, std::tm const & stm,
//                 std::ostream * hitcontext = nullptr)
// {
//   bool next = false;

//   // check 24/7
//   if (r.weekdays.empty() && r.timespan.empty() && r.state.state == osmoh::State::eOpen)
//     return true;

//   boost::gregorian::date date = boost::gregorian::date_from_tm(stm);
//   boost::posix_time::ptime pt = boost::posix_time::ptime_from_tm(stm);

//   next = r.weekdays.empty();
//   for (auto const & wd : r.weekdays)
//   {
//     if (check_weekday(wd, date))
//     {
//       if (hitcontext)
//         *hitcontext << wd << " ";
//       next = true;
//     }
//   }
//   if (!next)
//     return next;

//   next = r.timespan.empty();
//   for (auto const & ts : r.timespan)
//   {
//     if (check_timespan(ts, date, pt))
//     {
//       if (hitcontext)
//         *hitcontext << ts << " ";
//       next = true;
//     }
//   }
//   return next && !(r.timespan.empty() && r.weekdays.empty());
// }
// } // anonymouse namespace

// OSMTimeRange OSMTimeRange::FromString(std::string const & rules)
// {
//   OSMTimeRange timeRange;
//   std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter; // could not work on android
//   std::wstring src = converter.from_bytes(rules); // rules should be wstring
//   timeRange.m_valid = osmoh::parsing::parse_timerange(src.begin(), src.end(), timeRange.m_rules);
//   return timeRange;
// }

// OSMTimeRange & OSMTimeRange::UpdateState(time_t timestamp)
// {
//   std::tm stm = *localtime(&timestamp);

//   osmoh::State::EState true_state[3][3] = {
//     {osmoh::State::eUnknown, osmoh::State::eClosed, osmoh::State::eOpen},
//     {osmoh::State::eClosed , osmoh::State::eClosed, osmoh::State::eOpen},
//     {osmoh::State::eOpen   , osmoh::State::eClosed, osmoh::State::eOpen}
//   };

//   osmoh::State::EState false_state[3][3] = {
//     {osmoh::State::eUnknown, osmoh::State::eOpen   , osmoh::State::eClosed},
//     {osmoh::State::eClosed , osmoh::State::eClosed , osmoh::State::eClosed},
//     {osmoh::State::eOpen   , osmoh::State::eOpen   , osmoh::State::eOpen}
//   };

//   m_state = osmoh::State::eUnknown;
//   m_comment = std::string();

//   for (auto const & el : m_rules)
//   {
//     bool hit = false;
//     if ((hit = check_rule(el, stm)))
//     {
//       m_state = true_state[m_state][el.state.state];
//       m_comment = el.state.comment;
//     }
//     else
//     {
//       m_state = false_state[m_state][el.state.state];
//     }
//     /* very useful in debug */
//     //    char const * st[] = {"unknown", "closed", "open"};
//     //    std::cout << "-[" << hit << "]-------------------[" << el << "]: " << st[m_state] << "--------------------" << std::endl;
//   }
//   return *this;
// }

// OSMTimeRange & OSMTimeRange::UpdateState(std::string const & timestr, char const * timefmt)
// {
//   std::tm when = {};
//   std::stringstream ss(timestr);
//   ss >> std::get_time(&when, timefmt);
//   return UpdateState(std::mktime(&when));
// }
