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

#include "opening_hours.hpp"
#include "rules_evaluation.hpp"
#include "parse_opening_hours.hpp"

#include <cstdlib>
#include <iomanip>
#include <ios>
#include <ostream>
#include <vector>

namespace
{
template <typename T, typename SeparatorExtractor>
void PrintVector(std::ostream & ost, std::vector<T> const & v,
                 SeparatorExtractor && sepFunc)
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
  explicit StreamFlagsKeeper(std::ostream & ost):
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
  StreamFlagsKeeper keeper(ost);
  ost << std::setw(padding) << std::setfill('0') << number;
}

void PrintHoursMinutes(std::ostream & ost,
                       std::chrono::hours::rep hours,
                       std::chrono::minutes::rep minutes)
{
  PrintPaddedNumber(ost, hours, 2);
  ost << ':';
  PrintPaddedNumber(ost, minutes, 2);
}

} // namespace

namespace osmoh
{

// HourMinutes -------------------------------------------------------------------------------------
HourMinutes::HourMinutes(THours const duration)
{
  SetDuration(duration);
}

HourMinutes::HourMinutes(TMinutes const duration)
{
  SetDuration(duration);
}

bool HourMinutes::IsEmpty() const
{
  return m_empty;
}

bool HourMinutes::IsExtended() const
{
  return GetDuration() > 24_h;
}

HourMinutes::THours HourMinutes::GetHours() const
{
  return m_hours;
}

HourMinutes::TMinutes HourMinutes::GetMinutes() const
{
  return m_minutes;
}

HourMinutes::TMinutes HourMinutes::GetDuration() const
{
  return GetMinutes() + GetHours();
}

HourMinutes::THours::rep HourMinutes::GetHoursCount() const
{
  return GetHours().count();
}

HourMinutes::TMinutes::rep HourMinutes::GetMinutesCount() const
{
  return GetMinutes().count();
}

HourMinutes::TMinutes::rep HourMinutes::GetDurationCount() const
{
  return GetDuration().count();
}

void HourMinutes::SetHours(THours const hours)
{
  m_empty = false;
  m_hours = hours;
}

void HourMinutes::SetMinutes(TMinutes const minutes)
{
  m_empty = false;
  m_minutes = minutes;
}

void HourMinutes::SetDuration(TMinutes const duration)
{
  SetHours(std::chrono::duration_cast<THours>(duration));
  SetMinutes(duration - GetHours());
}

void HourMinutes::AddDuration(TMinutes const duration)
{
  SetDuration(GetDuration() + duration);
}

HourMinutes operator-(HourMinutes const & hm)
{
  HourMinutes result;
  result.SetHours(-hm.GetHours());
  result.SetMinutes(-hm.GetMinutes());
  return result;
}

std::ostream & operator<<(std::ostream & ost, HourMinutes const & hm)
{
  if (hm.IsEmpty())
    ost << "hh:mm";
  else
    PrintHoursMinutes(ost, std::abs(hm.GetHoursCount()), std::abs(hm.GetMinutesCount()));
  return ost;
}

// TimeEvent ---------------------------------------------------------------------------------------
TimeEvent::TimeEvent(Event const event): m_event(event) {}

bool TimeEvent::IsEmpty() const
{
  return m_event == Event::None;
}

bool TimeEvent::HasOffset() const
{
  return !m_offset.IsEmpty();
}

TimeEvent::Event TimeEvent::GetEvent() const
{
  return m_event;
}

void TimeEvent::SetEvent(TimeEvent::Event const event)
{
  m_event = event;
}

HourMinutes const & TimeEvent::GetOffset() const
{
  return m_offset;
}

void TimeEvent::SetOffset(HourMinutes const & offset)
{
  m_offset = offset;
}

void TimeEvent::AddDurationToOffset(HourMinutes::TMinutes const duration)
{
  m_offset.AddDuration(duration);
}

Time TimeEvent::GetEventTime() const
{
  return Time(HourMinutes(0_h + 0_min));  // TODO(mgsergio): get real time
}

std::ostream & operator<<(std::ostream & ost, TimeEvent::Event const event)
{
  switch (event)
  {
    case TimeEvent::Event::None:
      ost << "None";
    case TimeEvent::Event::Sunrise:
      ost << "sunrise";
      break;
    case TimeEvent::Event::Sunset:
      ost << "sunset";
      break;
  }
  return ost;
}

std::ostream & operator<<(std::ostream & ost, TimeEvent const te)
{
  if (te.HasOffset())
  {
    ost << '(' << te.GetEvent();

    auto const & offset = te.GetOffset();

    if (offset.GetHoursCount() < 0)
      ost << '-';
    else
      ost << '+';

    ost << offset << ')';
  }
  else
  {
    ost << te.GetEvent();
  }

  return ost;
}

// Time --------------------------------------------------------------------------------------------
Time::Time(HourMinutes const & hm)
{
  SetHourMinutes(hm);
}

Time::Time(TimeEvent const & te)
{
  SetEvent(te);
}

Time::Type Time::GetType() const
{
  return m_type;
}

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
    return GetEvent().GetEventTime().GetHours();
  return GetHourMinutes().GetHours();
}

Time::TMinutes Time::GetMinutes() const
{
  if (IsEvent())
    return GetEvent().GetEventTime().GetMinutes();
  return GetHourMinutes().GetMinutes();
}

void Time::AddDuration(TMinutes const duration)
{
  if (IsEvent())
  {
    m_event.AddDurationToOffset(duration);
  }
  else if (IsHoursMinutes())
  {
    m_hourMinutes.AddDuration(duration);
  }
  else
  {
    // Undefined behaviour.
  }
}

TimeEvent const & Time::GetEvent() const
{
  return m_event;
}

void Time::SetEvent(TimeEvent const & event)
{
  m_type = Type::Event;
  m_event = event;
}

HourMinutes const & Time::GetHourMinutes() const
{
  return m_hourMinutes;
}

void Time::SetHourMinutes(HourMinutes const & hm)
{
  m_type = Type::HourMinutes;
  m_hourMinutes = hm;
}

bool Time::IsEmpty() const
{
  return GetType() == Type::None;
}

bool Time::IsTime() const
{
  return IsHoursMinutes() || IsEvent();
}

bool Time::IsEvent() const
{
  return GetType() == Type::Event;
}

bool Time::IsHoursMinutes() const
{
  return GetType() == Type::HourMinutes;
}

std::ostream & operator<<(std::ostream & ost, Time const & time)
{
  if (time.IsEmpty())
  {
    ost << "hh:mm";
    return ost;
  }

  if (time.IsEvent())
    ost << time.GetEvent();
  else
    ost << time.GetHourMinutes();

  return ost;
}

// TimespanPrion -----------------------------------------------------------------------------------
TimespanPeriod::TimespanPeriod(HourMinutes const & hm):
    m_hourMinutes(hm),
    m_type(Type::HourMinutes)
{
}

TimespanPeriod::TimespanPeriod(HourMinutes::TMinutes const minutes):
    m_minutes(minutes),
    m_type(Type::Minutes)
{
}

bool TimespanPeriod::IsEmpty() const
{
  return m_type == Type::None;
}

bool TimespanPeriod::IsHoursMinutes() const
{
  return m_type == Type::HourMinutes;
}

bool TimespanPeriod::IsMinutes() const
{
  return m_type == Type::Minutes;
}

HourMinutes const & TimespanPeriod::GetHourMinutes() const
{
  return m_hourMinutes;
}

HourMinutes::TMinutes TimespanPeriod::GetMinutes() const
{
  return m_minutes;
}

HourMinutes::TMinutes::rep TimespanPeriod::GetMinutesCount() const
{
  return GetMinutes().count();
}

std::ostream & operator<<(std::ostream & ost, TimespanPeriod const p)
{
  if (p.IsEmpty())
    ost << "None";
  else if (p.IsHoursMinutes())
    ost << p.GetHourMinutes();
  else if (p.IsMinutes())
    PrintPaddedNumber(ost, p.GetMinutesCount(), 2);
  return ost;
}

// Timespan ----------------------------------------------------------------------------------------
bool Timespan::IsEmpty() const
{
  return !HasStart() && !HasEnd();
}

bool Timespan::IsOpen() const
{
  return HasStart() && !HasEnd();
}

bool Timespan::HasStart() const
{
  return !GetStart().IsEmpty();
}

bool Timespan::HasEnd() const
{
  return !GetEnd().IsEmpty();
}

bool Timespan::HasPlus() const
{
  return m_plus;
}

bool Timespan::HasPeriod() const
{
  return !m_period.IsEmpty();
}

bool Timespan::HasExtendedHours() const
{
  if (HasStart() && HasEnd() &&
      GetStart().IsHoursMinutes() &&
      GetEnd().IsHoursMinutes())
  {
    auto const & startHM = GetStart().GetHourMinutes();
    auto const & endHM = GetEnd().GetHourMinutes();
    if (endHM.IsExtended())
      return true;
    return endHM.GetDurationCount() != 0 && (endHM.GetDuration() < startHM.GetDuration());
  }

  return false;
}

Time const & Timespan::GetStart() const
{
  return m_start;
}

Time const & Timespan::GetEnd() const
{
  return m_end;
}

TimespanPeriod const & Timespan::GetPeriod() const
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

void Timespan::SetPeriod(TimespanPeriod const & period)
{
  m_period = period;
}

void Timespan::SetPlus(bool const plus)
{
  m_plus = plus;
}

bool Timespan::IsValid() const
{
  // TODO(mgsergio): implement validator.
  // See https://trello.com/c/e4pbOhDC/24-opening-hours
  return false;
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

// NthWeekdayOfTheMonthEntry -----------------------------------------------------------------------
bool NthWeekdayOfTheMonthEntry::IsEmpty() const
{
  return !HasStart() && !HasEnd();
}

bool NthWeekdayOfTheMonthEntry::HasStart() const
{
  return GetStart() != NthDayOfTheMonth::None;
}

bool NthWeekdayOfTheMonthEntry::HasEnd() const
{
  return GetEnd() != NthDayOfTheMonth::None;
}

NthWeekdayOfTheMonthEntry::NthDayOfTheMonth NthWeekdayOfTheMonthEntry::GetStart() const
{
  return m_start;
}

NthWeekdayOfTheMonthEntry::NthDayOfTheMonth NthWeekdayOfTheMonthEntry::GetEnd() const
{
  return m_end;
}

void NthWeekdayOfTheMonthEntry::SetStart(NthDayOfTheMonth const s)
{
  m_start = s;
}

void NthWeekdayOfTheMonthEntry::SetEnd(NthDayOfTheMonth const e)
{
  m_end = e;
}

std::ostream & operator<<(std::ostream & ost, NthWeekdayOfTheMonthEntry const entry)
{
  if (entry.HasStart())
    ost << static_cast<uint32_t>(entry.GetStart());
  if (entry.HasEnd())
    ost << '-' << static_cast<uint32_t>(entry.GetEnd());
  return ost;
}

// WeekdayRange ------------------------------------------------------------------------------------
bool WeekdayRange::HasWday(Weekday const & wday) const
{
  if (IsEmpty() || wday == Weekday::None)
    return false;

  if (!HasEnd())
    return GetStart() == wday;

  return GetStart() <= wday && wday <= GetEnd();
}

bool WeekdayRange::HasSunday() const { return HasWday(Weekday::Sunday); }
bool WeekdayRange::HasMonday() const { return HasWday(Weekday::Monday); }
bool WeekdayRange::HasTuesday() const { return HasWday(Weekday::Tuesday); }
bool WeekdayRange::HasWednesday() const { return HasWday(Weekday::Wednesday); }
bool WeekdayRange::HasThursday() const { return HasWday(Weekday::Thursday); }
bool WeekdayRange::HasFriday() const { return HasWday(Weekday::Friday); }
bool WeekdayRange::HasSaturday() const { return HasWday(Weekday::Saturday); }

bool WeekdayRange::HasStart() const
{
  return GetStart() != Weekday::None;
}

bool WeekdayRange::HasEnd() const
{
  return GetEnd() != Weekday::None;;
}

bool WeekdayRange::HasOffset() const
{
  return GetOffset() != 0;
}

bool WeekdayRange::IsEmpty() const
{
  return GetStart() == Weekday::None && GetEnd() == Weekday::None;
}

Weekday WeekdayRange::GetStart() const
{
  return m_start;
}

Weekday WeekdayRange::GetEnd() const
{
  return m_end;
}

void WeekdayRange::SetStart(Weekday const & wday)
{
  m_start = wday;
}

void WeekdayRange::SetEnd(Weekday const & wday)
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

void WeekdayRange::AddNth(NthWeekdayOfTheMonthEntry const & entry)
{
  m_nths.push_back(entry);
}

std::ostream & operator<<(std::ostream & ost, Weekday const wday)
{
  switch (wday)
  {
    case Weekday::Sunday:
      ost << "Su";
      break;
    case Weekday::Monday:
      ost << "Mo";
      break;
    case Weekday::Tuesday:
      ost << "Tu";
      break;
    case Weekday::Wednesday:
      ost << "We";
      break;
    case Weekday::Thursday:
      ost << "Th";
      break;
    case Weekday::Friday:
      ost << "Fr";
      break;
    case Weekday::Saturday:
      ost << "Sa";
      break;
    case Weekday::None:
      ost << "None";
  }
  return ost;
}

std::ostream & operator<<(std::ostream & ost, WeekdayRange const & range)
{
  ost << range.GetStart();
  if (range.HasEnd())
  {
    ost << '-' << range.GetEnd();
  }
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

// Holiday -----------------------------------------------------------------------------------------
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
  {
    ost << "PH";
  }
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

// Weekdays ----------------------------------------------------------------------------------------
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

// DateOffset --------------------------------------------------------------------------------------
bool DateOffset::IsEmpty() const
{
  return !HasOffset() && !HasWDayOffset();
}

bool DateOffset::HasWDayOffset() const
{
  return m_wdayOffest != Weekday::None;
}

bool DateOffset::HasOffset() const
{
  return m_offset != 0;
}

bool DateOffset::IsWDayOffsetPositive() const
{
  return m_positive;
}

Weekday DateOffset::GetWDayOffset() const
{
  return m_wdayOffest;
}

int32_t DateOffset::GetOffset() const
{
  return m_offset;
}

void DateOffset::SetWDayOffset(Weekday const wday)
{
  m_wdayOffest = wday;
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
  {
    ost << (offset.IsWDayOffsetPositive() ? '+' : '-')
        << offset.GetWDayOffset();
  }
  PrintOffset(ost, offset.GetOffset(), offset.HasWDayOffset());
  return ost;
}

// MonthDay ----------------------------------------------------------------------------------------
bool MonthDay::IsEmpty() const
{
  return !HasYear() && !HasMonth() && !HasDayNum() && !IsVariable();
}

bool MonthDay::IsVariable() const
{
  return GetVariableDate() != VariableDate::None;
}

bool MonthDay::HasYear() const
{
  return GetYear() != 0;
}

bool MonthDay::HasMonth() const
{
  return GetMonth() != Month::None;
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

MonthDay::Month MonthDay::GetMonth() const
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

MonthDay::VariableDate MonthDay::GetVariableDate() const
{
  return m_variable_date;
}

void MonthDay::SetYear(TYear const year)
{
  m_year = year;
}

void MonthDay::SetMonth(Month const month)
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

void MonthDay::SetVariableDate(VariableDate const date)
{
  m_variable_date = date;
}

std::ostream & operator<<(std::ostream & ost, MonthDay::Month const month)
{
  switch (month)
  {
    case MonthDay::Month::None:
      ost << "None";
      break;
    case MonthDay::Month::Jan:
      ost << "Jan";
      break;
    case MonthDay::Month::Feb:
      ost << "Feb";
      break;
    case MonthDay::Month::Mar:
      ost << "Mar";
      break;
    case MonthDay::Month::Apr:
      ost << "Apr";
      break;
    case MonthDay::Month::May:
      ost << "May";
      break;
    case MonthDay::Month::Jun:
      ost << "Jun";
      break;
    case MonthDay::Month::Jul:
      ost << "Jul";
      break;
    case MonthDay::Month::Aug:
      ost << "Aug";
      break;
    case MonthDay::Month::Sep:
      ost << "Sep";
      break;
    case MonthDay::Month::Oct:
      ost << "Oct";
      break;
    case MonthDay::Month::Nov:
      ost << "Nov";
      break;
    case MonthDay::Month::Dec:
      ost << "Dec";
      break;
  }
  return ost;
}

std::ostream & operator<<(std::ostream & ost, MonthDay::VariableDate const date)
{
  switch (date)
  {
    case MonthDay::VariableDate::None:
      ost << "none";
      break;
    case MonthDay::VariableDate::Easter:
      ost << "easter";
      break;
  }
  return ost;
}

std::ostream & operator<<(std::ostream & ost, MonthDay const md)
{
  bool space = false;
  auto const putSpace = [&space, &ost] {
    if (space)
      ost << ' ';
    space = true;
  };

  if (md.HasYear())
  {
    putSpace();
    ost << md.GetYear();
  }

  if (md.IsVariable())
  {
    putSpace();
    ost << md.GetVariableDate();
  }
  else
  {
    if (md.HasMonth())
    {
      putSpace();
      ost << md.GetMonth();
    }
    if (md.HasDayNum())
    {
      putSpace();
      PrintPaddedNumber(ost, md.GetDayNum(), 2);
    }
  }
  if (md.HasOffset())
  {
    ost << ' ' << md.GetOffset();
  }
  return ost;
}

// MonthdayRange -----------------------------------------------------------------------------------
bool MonthdayRange::IsEmpty() const
{
  return !HasStart() && !HasEnd();
}

bool MonthdayRange::HasStart() const
{
  return !GetStart().IsEmpty();
}

bool MonthdayRange::HasEnd() const
{
  return !GetEnd().IsEmpty() || GetEnd().HasDayNum();
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

// YearRange ---------------------------------------------------------------------------------------
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
  {
    ost << '+';
  }

  return ost;
}

std::ostream & operator<<(std::ostream & ost, TYearRanges const ranges)
{
  PrintVector(ost, ranges);
  return ost;
}

// WeekRange ---------------------------------------------------------------------------------------
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

// RuleSequence ------------------------------------------------------------------------------------
bool RuleSequence::IsEmpty() const
{
  return (!HasYears() && !HasMonths() &&
          !HasWeeks() && !HasWeekdays() &&
          !HasTimes());
}

bool RuleSequence::IsTwentyFourHours() const
{
  return m_twentyFourHours;
}

bool RuleSequence::HasYears() const
{
  return !GetYears().empty();
}

bool RuleSequence::HasMonths() const
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
  return m_separatorForReadability;
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
  return m_modifierComment;
}

std::string const & RuleSequence::GetAnySeparator() const
{
  return m_anySeparator;
}

RuleSequence::Modifier RuleSequence::GetModifier() const
{
  return m_modifier;
}

void RuleSequence::SetTwentyFourHours(bool const on)
{
  m_twentyFourHours = on;
}

void RuleSequence::SetYears(TYearRanges const & years)
{
  m_years = years;
}

void RuleSequence::SetMonths(TMonthdayRanges const & months)
{
  m_months = months;
}

void RuleSequence::SetWeeks(TWeekRanges const & weeks)
{
  m_weeks = weeks;
}

void RuleSequence::SetWeekdays(Weekdays const & weekdays)
{
  m_weekdays = weekdays;
}

void RuleSequence::SetTimes(TTimespans const & times)
{
  m_times = times;
}

void RuleSequence::SetComment(std::string const & comment)
{
  m_comment = comment;
}

void RuleSequence::SetModifierComment(std::string & comment)
{
  m_modifierComment = comment;
}

void RuleSequence::SetAnySeparator(std::string const & separator)
{
  m_anySeparator = separator;
}

void RuleSequence::SetSeparatorForReadability(bool const on)
{
  m_separatorForReadability = on;
}

void RuleSequence::SetModifier(Modifier const modifier)
{
  m_modifier = modifier;
}

std::ostream & operator<<(std::ostream & ost, RuleSequence::Modifier const modifier)
{
  switch (modifier)
  {
    case RuleSequence::Modifier::DefaultOpen:
    case RuleSequence::Modifier::Comment:
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
  bool space = false;
  auto const putSpace = [&space, &ost] {
    if (space)
      ost << ' ';
    space = true;
  };

  if (s.IsTwentyFourHours())
  {
    putSpace();
    ost << "24/7";
  }
  else
  {
    if (s.HasComment())
      ost << s.GetComment() << ':';
    else
    {
      if (s.HasYears())
      {
        putSpace();
        ost << s.GetYears();
      }
      if (s.HasMonths())
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
        ost << ':';

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
  if (s.GetModifier() != RuleSequence::Modifier::DefaultOpen &&
      s.GetModifier() != RuleSequence::Modifier::Comment)
  {
    putSpace();
    ost << s.GetModifier();
  }
  if (s.HasModifierComment())
  {
    putSpace();
    ost << '"' << s.GetModifierComment() << '"';
  }

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

// OpeningHours ------------------------------------------------------------------------------------
OpeningHours::OpeningHours(std::string const & rule):
    m_valid(Parse(rule, m_rule))
{
}

OpeningHours::OpeningHours(TRuleSequences const & rule):
    m_rule(rule),
    m_valid(true)
{
}

bool OpeningHours::IsOpen(time_t const dateTime) const
{
  return osmoh::IsOpen(m_rule, dateTime);
}

bool OpeningHours::IsClosed(time_t const dateTime) const
{
  return osmoh::IsClosed(m_rule, dateTime);
}

bool OpeningHours::IsUnknown(time_t const dateTime) const
{
  return osmoh::IsUnknown(m_rule, dateTime);
}

bool OpeningHours::IsValid() const
{
  return m_valid;
}
} // namespace osmoh
