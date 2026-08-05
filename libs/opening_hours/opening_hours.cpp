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

#include "opening_hours/opening_hours.hpp"

#include "opening_hours/oh/convert.hpp"
#include "opening_hours/oh/eval.hpp"
#include "opening_hours/oh/parser.hpp"

#include <algorithm>
#include <cstdlib>
#include <functional>
#include <iomanip>
#include <ios>
#include <ostream>
#include <sstream>
#include <tuple>
#include <type_traits>
#include <vector>

namespace
{
template <typename T, typename SeparatorExtractor>
void PrintVector(std::ostream & ost, std::vector<T> const & v, SeparatorExtractor && sepFunc)
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
  explicit StreamFlagsKeeper(std::ostream & ost) : m_ost(ost), m_flags(m_ost.flags()) {}

  ~StreamFlagsKeeper() { m_ost.flags(m_flags); }

private:
  std::ostream & m_ost;
  std::ios_base::fmtflags m_flags;
};

template <typename TNumber>
void PrintPaddedNumber(std::ostream & ost, TNumber const number, uint32_t const padding = 1)
{
  static constexpr bool isChar =
      std::is_same_v<signed char, TNumber> || std::is_same_v<unsigned char, TNumber> || std::is_same_v<char, TNumber>;

  if constexpr (isChar)
  {
    PrintPaddedNumber(ost, static_cast<int32_t>(number), padding);
  }
  else
  {
    static_assert(std::is_integral<TNumber>::value, "number should be of integral type.");
    StreamFlagsKeeper keeper(ost);
    ost << std::setw(padding) << std::setfill('0') << number;
  }
}

void PrintHoursMinutes(std::ostream & ost, std::chrono::hours::rep hours, std::chrono::minutes::rep minutes)
{
  PrintPaddedNumber(ost, hours, 2);
  ost << ':';
  PrintPaddedNumber(ost, minutes, 2);
}

}  // namespace

namespace osmoh
{

// HourMinutes -------------------------------------------------------------------------------------

bool HourMinutes::IsExtended() const
{
  return GetDuration() > 24_h;
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
Time TimeEvent::GetEventTime() const
{
  // Sun events have no clock value without a date and coordinates. The port
  // computes the real one (oh::event_angle/hour_angle, with a fixed fallback of
  // dawn 06:00 / sunrise 07:00 / sunset 19:00 / dusk 20:00), but this type
  // cannot reach either input, so callers must check IsEvent() instead of
  // treating the placeholder below as a clock time.
  return Time(HourMinutes(0_h + 0_min));
}

std::ostream & operator<<(std::ostream & ost, TimeEvent::Event const event)
{
  switch (event)
  {
  case TimeEvent::Event::None: ost << "None"; break;
  case TimeEvent::Event::Dawn: ost << "dawn"; break;
  case TimeEvent::Event::Sunrise: ost << "sunrise"; break;
  case TimeEvent::Event::Sunset: ost << "sunset"; break;
  case TimeEvent::Event::Dusk: ost << "dusk"; break;
  }
  return ost;
}

std::ostream & operator<<(std::ostream & ost, TimeEvent const te)
{
  if (te.HasOffset())
  {
    ost << '(' << te.GetEvent();

    auto const & offset = te.GetOffset();

    if (offset.GetDurationCount() < 0)
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

void Time::SetEvent(TimeEvent const & event)
{
  m_type = Type::Event;
  m_event = event;
}

void Time::SetHourMinutes(HourMinutes const & hm)
{
  m_type = Type::HourMinutes;
  m_hourMinutes = hm;
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

bool operator==(Time const & lhs, Time const & rhs)
{
  if (lhs.IsEmpty() && rhs.IsEmpty())
    return true;
  if (lhs.GetType() != rhs.GetType())
    return false;

  // An event has no clock value -- GetHours()/GetMinutes() return the same
  // placeholder for all of them -- so compare the event itself. Otherwise
  // sunrise, sunset, dawn, dusk and "sunrise+01:00" would all be equal.
  if (lhs.IsEvent())
    return lhs.GetEvent().GetEvent() == rhs.GetEvent().GetEvent() &&
           lhs.GetEvent().GetOffset() == rhs.GetEvent().GetOffset();

  return lhs.GetHours() == rhs.GetHours() && lhs.GetMinutes() == rhs.GetMinutes();
}

// TimespanPeriod ----------------------------------------------------------------------------------
TimespanPeriod::TimespanPeriod(HourMinutes const & hm) : m_hourMinutes(hm), m_type(Type::HourMinutes) {}

TimespanPeriod::TimespanPeriod(HourMinutes::TMinutes const minutes) : m_minutes(minutes), m_type(Type::Minutes) {}

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

bool operator==(TimespanPeriod const & lhs, TimespanPeriod const & rhs)
{
  if (lhs.IsEmpty() && rhs.IsEmpty())
    return true;

  return lhs.GetType() == rhs.GetType() && lhs.GetHourMinutes() == rhs.GetHourMinutes() &&
         lhs.GetMinutes() == rhs.GetMinutes();
}

// Timespan ----------------------------------------------------------------------------------------
bool Timespan::HasExtendedHours() const
{
  bool const canHaveExtendedHours = HasStart() && HasEnd() && GetStart().IsHoursMinutes() && GetEnd().IsHoursMinutes();
  if (!canHaveExtendedHours)
    return false;

  auto const & startHM = GetStart().GetHourMinutes();
  auto const & endHM = GetEnd().GetHourMinutes();

  if (endHM.IsExtended())
    return true;

  return endHM.GetDuration() <= startHM.GetDuration();
}

void Timespan::ExpandPlus()
{
  if (HasPlus())
    SetEnd(HourMinutes(24_h));
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

bool operator==(Timespan const & lhs, Timespan const & rhs)
{
  if (lhs.IsEmpty() && rhs.IsEmpty())
    return true;

  if (lhs.IsEmpty() != rhs.IsEmpty() || lhs.HasStart() != rhs.HasStart() || lhs.HasEnd() != rhs.HasEnd() ||
      lhs.HasPlus() != rhs.HasPlus() || lhs.HasPeriod() != rhs.HasPeriod())
  {
    return false;
  }

  return lhs.GetStart() == rhs.GetStart() && lhs.GetEnd() == rhs.GetEnd() && lhs.GetPeriod() == rhs.GetPeriod();
}

// NthWeekdayOfTheMonthEntry -----------------------------------------------------------------------
std::ostream & operator<<(std::ostream & ost, NthWeekdayOfTheMonthEntry const entry)
{
  if (entry.HasStart())
    ost << static_cast<uint32_t>(entry.GetStart());
  if (entry.HasEnd())
    ost << '-' << static_cast<uint32_t>(entry.GetEnd());
  return ost;
}

bool NthWeekdayOfTheMonthEntry::operator==(NthWeekdayOfTheMonthEntry const & rhs) const
{
  return m_start == rhs.m_start && m_end == rhs.m_end;
}

// WeekdayRange ------------------------------------------------------------------------------------
bool WeekdayRange::HasWday(Weekday const wday) const
{
  if (IsEmpty() || wday == Weekday::None)
    return false;

  if (!HasEnd())
    return GetStart() == wday;

  return (GetStart() <= GetEnd()) ? GetStart() <= wday && wday <= GetEnd() : wday <= GetEnd() || GetStart() <= wday;
}

bool WeekdayRange::operator==(WeekdayRange const & rhs) const
{
  return m_start == rhs.m_start && m_end == rhs.m_end && m_offset == rhs.m_offset && m_nths == rhs.m_nths;
}

std::ostream & operator<<(std::ostream & ost, Weekday wday)
{
  switch (wday)
  {
  case Weekday::Sunday: ost << "Su"; break;
  case Weekday::Monday: ost << "Mo"; break;
  case Weekday::Tuesday: ost << "Tu"; break;
  case Weekday::Wednesday: ost << "We"; break;
  case Weekday::Thursday: ost << "Th"; break;
  case Weekday::Friday: ost << "Fr"; break;
  case Weekday::Saturday: ost << "Sa"; break;
  case Weekday::None: ost << "None";
  }
  return ost;
}

std::ostream & operator<<(std::ostream & ost, WeekdayRange const & range)
{
  ost << range.GetStart();
  if (range.HasEnd())
    ost << '-' << range.GetEnd();
  // The parser accepts nths and offsets on ranges too; dropping them here
  // would misrepresent the value while conversion reports it as complete.
  if (range.HasNth())
  {
    ost << '[';
    PrintVector(ost, range.GetNths(), ",");
    ost << ']';
  }
  PrintOffset(ost, range.GetOffset(), true);
  return ost;
}

std::ostream & operator<<(std::ostream & ost, TWeekdayRanges const & ranges)
{
  PrintVector(ost, ranges);
  return ost;
}

// Holiday -----------------------------------------------------------------------------------------
std::ostream & operator<<(std::ostream & ost, Holiday const & holiday)
{
  ost << (holiday.IsPlural() ? "PH" : "SH");
  // "PH +1 day" carries its offset exactly like "SH".
  PrintOffset(ost, holiday.GetOffset(), true);
  return ost;
}

std::ostream & operator<<(std::ostream & ost, THolidays const & holidays)
{
  PrintVector(ost, holidays);
  return ost;
}

bool Holiday::operator==(Holiday const & rhs) const
{
  return m_plural == rhs.m_plural && m_offset == rhs.m_offset;
}

// Weekdays ----------------------------------------------------------------------------------------

std::ostream & operator<<(std::ostream & ost, Weekdays const & weekday)
{
  ost << weekday.GetHolidays();
  if (weekday.HasWeekday() && weekday.HasHolidays())
    ost << ", ";
  ost << weekday.GetWeekdayRanges();
  return ost;
}

bool Weekdays::operator==(Weekdays const & rhs) const
{
  return m_weekdayRanges == rhs.m_weekdayRanges && m_holidays == rhs.m_holidays;
}

// DateOffset --------------------------------------------------------------------------------------
std::ostream & operator<<(std::ostream & ost, DateOffset const & offset)
{
  if (offset.HasWDayOffset())
    ost << (offset.IsWDayOffsetPositive() ? '+' : '-') << offset.GetWDayOffset();
  PrintOffset(ost, offset.GetOffset(), offset.HasWDayOffset());
  return ost;
}

bool DateOffset::operator==(DateOffset const & rhs) const
{
  return m_wdayOffest == rhs.m_wdayOffest && m_positive == rhs.m_positive && m_offset == rhs.m_offset;
}

bool DateOffset::operator<(DateOffset const & rhs) const
{
  return std::tie(m_wdayOffest, m_positive, m_offset) < std::tie(rhs.m_wdayOffest, rhs.m_positive, rhs.m_offset);
}

// MonthDay ----------------------------------------------------------------------------------------
std::ostream & operator<<(std::ostream & ost, MonthDay::Month const month)
{
  switch (month)
  {
  case MonthDay::Month::None: ost << "None"; break;
  case MonthDay::Month::Jan: ost << "Jan"; break;
  case MonthDay::Month::Feb: ost << "Feb"; break;
  case MonthDay::Month::Mar: ost << "Mar"; break;
  case MonthDay::Month::Apr: ost << "Apr"; break;
  case MonthDay::Month::May: ost << "May"; break;
  case MonthDay::Month::Jun: ost << "Jun"; break;
  case MonthDay::Month::Jul: ost << "Jul"; break;
  case MonthDay::Month::Aug: ost << "Aug"; break;
  case MonthDay::Month::Sep: ost << "Sep"; break;
  case MonthDay::Month::Oct: ost << "Oct"; break;
  case MonthDay::Month::Nov: ost << "Nov"; break;
  case MonthDay::Month::Dec: ost << "Dec"; break;
  }
  return ost;
}

std::ostream & operator<<(std::ostream & ost, MonthDay::VariableDate const date)
{
  switch (date)
  {
  case MonthDay::VariableDate::None: ost << "none"; break;
  case MonthDay::VariableDate::Easter: ost << "easter"; break;
  }
  return ost;
}

std::ostream & operator<<(std::ostream & ost, MonthDay const md)
{
  bool space = false;
  auto const putSpace = [&space, &ost]
  {
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
    if (md.HasWeekdayInMonth())
    {
      putSpace();
      ost << md.GetWeekday();
      if (md.GetNth() != 0)
        ost << '[' << static_cast<int>(md.GetNth()) << ']';
    }
    if (md.HasDayNum())
    {
      putSpace();
      PrintPaddedNumber(ost, md.GetDayNum(), 2);
    }
  }
  if (md.HasOffset())
    ost << ' ' << md.GetOffset();
  return ost;
}

bool MonthDay::operator==(MonthDay const & rhs) const
{
  return m_year == rhs.m_year && m_month == rhs.m_month && m_daynum == rhs.m_daynum &&
         m_variable_date == rhs.m_variable_date && m_offset == rhs.m_offset && m_weekday == rhs.m_weekday &&
         m_nth == rhs.m_nth;
}

bool MonthDay::operator<(MonthDay const & rhs) const
{
  return std::tie(m_year, m_month, m_daynum, m_variable_date, m_offset, m_weekday, m_nth) <
         std::tie(rhs.m_year, rhs.m_month, rhs.m_daynum, rhs.m_variable_date, rhs.m_offset, rhs.m_weekday, rhs.m_nth);
}

// MonthdayRange -----------------------------------------------------------------------------------
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

bool MonthdayRange::operator==(MonthdayRange const & rhs) const
{
  return m_start == rhs.m_start && m_end == rhs.m_end && m_period == rhs.m_period && m_plus == rhs.m_plus;
}

// YearRange ---------------------------------------------------------------------------------------
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

bool YearRange::operator==(YearRange const & rhs) const
{
  return m_start == rhs.m_start && m_end == rhs.m_end && m_plus == rhs.m_plus && m_period == rhs.m_period;
}

// WeekRange ---------------------------------------------------------------------------------------
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

bool WeekRange::operator==(WeekRange const & rhs) const
{
  return m_start == rhs.m_start && m_end == rhs.m_end && m_period == rhs.m_period;
}

// RuleSequence ------------------------------------------------------------------------------------
bool RuleSequence::HasMonthDay() const
{
  for (auto const & monthRange : GetMonths())
  {
    if (monthRange.GetStart().GetDayNum())
      return true;

    if (monthRange.GetEnd().GetDayNum())
      return true;
  }

  return false;
}

bool RuleSequence::operator==(RuleSequence const & rhs) const
{
  return m_twentyFourHours == rhs.m_twentyFourHours && m_years == rhs.m_years && m_months == rhs.m_months &&
         m_weeks == rhs.m_weeks && m_weekdays == rhs.m_weekdays && m_times == rhs.m_times &&
         m_anySeparator == rhs.m_anySeparator && m_modifier == rhs.m_modifier &&
         m_modifierComment == rhs.m_modifierComment;
}

std::ostream & operator<<(std::ostream & ost, RuleSequence::Modifier const modifier)
{
  switch (modifier)
  {
  case RuleSequence::Modifier::DefaultOpen:
  case RuleSequence::Modifier::Comment: break;
  case RuleSequence::Modifier::Unknown: ost << "unknown"; break;
  case RuleSequence::Modifier::Closed: ost << "closed"; break;
  case RuleSequence::Modifier::Open: ost << "open"; break;
  }
  return ost;
}

std::ostream & operator<<(std::ostream & ost, RuleSequence const & s)
{
  bool space = false;
  auto const putSpace = [&space, &ost]
  {
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
  if (s.GetModifier() != RuleSequence::Modifier::DefaultOpen && s.GetModifier() != RuleSequence::Modifier::Comment)
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
  PrintVector(ost, s, [](RuleSequence const & r)
  {
    auto const sep = r.GetAnySeparator();
    return (sep == "||" ? ' ' + sep + ' ' : sep + ' ');
  });
  return ost;
}

// OpeningHours ------------------------------------------------------------------------------------
namespace
{

// Wall-clock seconds (as if UTC) in the POI's zone: gmtime(result) is the local
// wall clock. With a zone we use the mwm timezone db; otherwise device-local.
int64_t ToZonedSeconds(time_t t, std::optional<om::tz::TimeZone> const & tz)
{
  if (tz)
    return om::tz::Convert(t, *tz);

  std::tm lt{};
#ifdef _WIN32
  localtime_s(&lt, &t);
#else
  localtime_r(&t, &lt);
#endif
  auto const ymd = std::chrono::year{lt.tm_year + 1900} / std::chrono::month{static_cast<unsigned>(lt.tm_mon + 1)} /
                   std::chrono::day{static_cast<unsigned>(lt.tm_mday)};
  int64_t const days = std::chrono::sys_days{ymd}.time_since_epoch().count();
  return days * 86400 + lt.tm_hour * 3600 + lt.tm_min * 60 + lt.tm_sec;
}

oh::NaiveDateTime ToNaive(int64_t zonedSeconds)
{
  int64_t const days = oh::floordiv(zonedSeconds, 86400);
  int const minute = static_cast<int>((zonedSeconds - days * 86400) / 60);
  return oh::NaiveDateTime{oh::NaiveDate{std::chrono::sys_days{std::chrono::days{days}}}, minute};
}

int64_t NaiveToZoned(oh::NaiveDateTime n)
{
  return static_cast<int64_t>(n.date().day_number()) * 86400 + static_cast<int64_t>(n.minute_of_day()) * 60;
}

RuleState ToRuleState(oh::RuleKind k)
{
  switch (k)
  {
  case oh::RuleKind::Open: return RuleState::Open;
  case oh::RuleKind::Closed: return RuleState::Closed;
  case oh::RuleKind::Unknown: return RuleState::Unknown;
  }
  return RuleState::Unknown;
}

oh::OpeningHours<> MakeEval(std::shared_ptr<oh::OpeningHoursExpression const> const & expr)
{
  return oh::OpeningHours<>(expr, oh::Context<oh::NoLocation>{});
}

// A value that did not parse has no state to report, which is Unknown -- the
// same answer GetInfo() gives.
RuleState EvalState(std::shared_ptr<oh::OpeningHoursExpression const> const & expr, time_t dateTime)
{
  if (!expr)
    return RuleState::Unknown;
  return ToRuleState(MakeEval(expr).state(ToNaive(ToZonedSeconds(dateTime, std::nullopt))).first);
}
}  // namespace

OpeningHours::OpeningHours(std::string_view rule)
{
  if (auto parsed = oh::parse(rule))
  {
    m_expr = std::make_shared<oh::OpeningHoursExpression const>(std::move(*parsed));
    m_rule = ToOsmoh(*m_expr);
  }
}

OpeningHours::OpeningHours(TRuleSequences const & rule)
  : m_rule(rule)
  , m_expr(std::make_shared<oh::OpeningHoursExpression const>(ToPort(rule)))
{}

bool OpeningHours::IsOpen(time_t const dateTime) const
{
  return EvalState(m_expr, dateTime) == RuleState::Open;
}

bool OpeningHours::IsClosed(time_t const dateTime) const
{
  return EvalState(m_expr, dateTime) == RuleState::Closed;
}

bool OpeningHours::IsUnknown(time_t const dateTime) const
{
  return EvalState(m_expr, dateTime) == RuleState::Unknown;
}

OpeningHours::InfoT OpeningHours::GetInfo(time_t const dateTime, std::optional<om::tz::TimeZone> const & timeZone) const
{
  InfoT info;
  if (!m_expr)
  {
    info.state = RuleState::Unknown;
    return info;
  }

  int64_t const baseZoned = ToZonedSeconds(dateTime, timeZone);
  oh::NaiveDateTime const now = ToNaive(baseZoned);
  auto const eval = MakeEval(m_expr);
  info.state = ToRuleState(eval.state(now).first);

  if (info.state == RuleState::Unknown)
    return info;

  // First transition to `target` state at or after `now`, back in time_t.
  // Scanning a bounded window keeps seasonal schedules cheap; kTimeTMax means
  // "no change found" (consumers render this as "never"). Iterate lazily and
  // stop at the first matching interval -- for typical schedules that is 1-3
  // intervals, while materializing the whole window costs hundreds of
  // allocations per call (and GetInfo runs for every search result).
  time_t constexpr kTimeTMax = std::numeric_limits<time_t>::max();

  // The default window covers a full seasonal cycle; schedules pinned to
  // explicit future years ("2028 Jan 01 10:00-11:00") extend it through their
  // last mentioned year, otherwise they would report "never opens".
  oh::NaiveDateTime to = now.add_minutes(static_cast<int64_t>(400) * 24 * 60);
  {
    int lastYear = 0;
    auto const account = [&](int const year)
    {
      if (year > lastYear && year < 9999)  // 9999 is the open-end sentinel.
        lastYear = year;
    };
    for (auto const & rule : m_expr->rules)
    {
      for (auto const & year : rule.day_selector.year)
      {
        account(year.start);
        account(year.end);
      }
      for (auto const & monthday : rule.day_selector.monthday)
      {
        if (monthday.month_year)
          account(*monthday.month_year);
        if (monthday.date_start.year)
          account(*monthday.date_start.year);
        if (monthday.date_end.year)
          account(*monthday.date_end.year);
      }
    }
    lastYear = std::min(lastYear, now.date().year() + 50);  // Clamp runaway data.
    if (lastYear >= now.date().year())
    {
      oh::NaiveDateTime const horizon{oh::NaiveDate::ymd_unchecked(lastYear + 2, 1, 1), 0};
      if (to < horizon)
        to = horizon;
    }
  }

  // Selectors recurring with multi-year gaps need more than the seasonal
  // window: a year-less "Feb 29" next matches up to 8 years out (skipped
  // century leap years), ISO week 53 up to 7 years. The scan is hint-driven,
  // so the wider window only costs anything on these rare values.
  {
    oh::Date const feb29 = oh::Date::make_fixed(std::nullopt, oh::Month::Feb, 29);
    bool longRecurrence = false;
    for (auto const & rule : m_expr->rules)
    {
      for (auto const & week : rule.day_selector.week)
        if (week.start <= 53 && 53 <= week.end)
          longRecurrence = true;
      for (auto const & monthday : rule.day_selector.monthday)
        if (monthday.date_start == feb29 && monthday.date_end == feb29)
          longRecurrence = true;
    }
    if (longRecurrence)
    {
      oh::NaiveDateTime const horizon = now.add_minutes(static_cast<int64_t>(9) * 366 * 24 * 60);
      if (to < horizon)
        to = horizon;
    }
  }

  // Convert a zoned wall-clock instant back to time_t. The naive delta is off
  // by the DST shift when a transition lies between `now` and the target;
  // correcting through the forward conversion handles the device zone and an
  // explicit POI zone alike.
  auto const toTimeT = [&](int64_t const targetZoned) -> time_t
  {
    time_t t = dateTime + (targetZoned - baseZoned);
    for (int i = 0; i < 2; ++i)
    {
      int64_t const diff = ToZonedSeconds(t, timeZone) - targetZoned;
      if (diff == 0)
        return t;
      t -= diff;
    }
    if (ToZonedSeconds(t, timeZone) == targetZoned)
      return t;

    // No instant maps to the target: it falls into a spring-forward gap and
    // the correction loop oscillates around it. Answer the first valid
    // instant (the transition itself) -- the evaluator's zone contract snaps
    // nonexistent local times forward the same way, so IsOpen() agrees.
    time_t lo = t, hi = t;
    for (int i = 0; i < 48 && ToZonedSeconds(lo, timeZone) >= targetZoned; ++i)
      lo -= 1800;
    for (int i = 0; i < 48 && ToZonedSeconds(hi, timeZone) < targetZoned; ++i)
      hi += 1800;
    while (lo + 1 < hi)
    {
      time_t const mid = lo + (hi - lo) / 2;
      if (ToZonedSeconds(mid, timeZone) < targetZoned)
        lo = mid;
      else
        hi = mid;
    }
    return hi;
  };

  auto nextTimeOf = [&](oh::RuleKind target) -> time_t
  {
    oh::TimeDomainIterator<oh::NoLocation> it(m_expr, oh::Context<oh::NoLocation>{}, now, to);
    while (auto const interval = it.next())
    {
      if (!(interval->start < to))
        break;
      if (interval->kind == target)
        return toTimeT(NaiveToZoned(std::max(interval->start, now)));
    }
    return kTimeTMax;
  };

  info.nextTimeOpen = info.state == RuleState::Open ? dateTime : nextTimeOf(oh::RuleKind::Open);
  info.nextTimeClosed = info.state == RuleState::Closed ? dateTime : nextTimeOf(oh::RuleKind::Closed);
  return info;
}

bool OpeningHours::IsValid() const
{
  return m_expr != nullptr;
}

bool OpeningHours::IsTwentyFourHours() const
{
  return m_rule.size() == 1 && m_rule[0].IsTwentyFourHours();
}

bool OpeningHours::HasWeekdaySelector() const
{
  return std::any_of(m_rule.cbegin(), m_rule.cend(), std::mem_fn(&osmoh::RuleSequence::HasWeekdays));
}

bool OpeningHours::HasMonthSelector() const
{
  return std::any_of(m_rule.cbegin(), m_rule.cend(), std::mem_fn(&osmoh::RuleSequence::HasMonths));
}

bool OpeningHours::HasWeekSelector() const
{
  return std::any_of(m_rule.cbegin(), m_rule.cend(), std::mem_fn(&osmoh::RuleSequence::HasWeeks));
}

bool OpeningHours::HasYearSelector() const
{
  return std::any_of(m_rule.cbegin(), m_rule.cend(), std::mem_fn(&osmoh::RuleSequence::HasYears));
}

bool OpeningHours::operator==(OpeningHours const & rhs) const
{
  return IsValid() == rhs.IsValid() && m_rule == rhs.m_rule;
}

std::ostream & operator<<(std::ostream & ost, OpeningHours const & oh)
{
  ost << oh.GetRule();
  return ost;
}

std::string ToString(osmoh::OpeningHours const & openingHours)
{
  if (!openingHours.IsValid())
    return {};

  std::ostringstream stream;
  stream << openingHours;
  return stream.str();
}
}  // namespace osmoh
