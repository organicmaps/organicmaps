#include "3party/opening_hours/oh/convert.hpp"

#include <chrono>

namespace
{
namespace oh = opening_hours;

// ---- enum bridges ---------------------------------------------------------

// port Weekday: Mon=0..Sun=6 ; osmoh: None=0, Sunday=1, Monday=2..Saturday=7.
osmoh::Weekday PortWday(oh::Weekday w)
{
  int const p = static_cast<int>(w);
  return static_cast<osmoh::Weekday>(p == 6 ? 1 : p + 2);
}

oh::Weekday OsmohWday(osmoh::Weekday w)
{
  int const o = static_cast<int>(w);
  return static_cast<oh::Weekday>(o == 1 ? 6 : o - 2);
}

// Months share values (Jan=1..Dec=12); osmoh has an extra None=0.
osmoh::MonthDay::Month PortMonth(oh::Month m)
{
  return static_cast<osmoh::MonthDay::Month>(static_cast<int>(m));
}

oh::Month OsmohMonth(osmoh::MonthDay::Month m)
{
  return static_cast<oh::Month>(static_cast<int>(m));
}

osmoh::RuleSequence::Modifier PortKind(oh::RuleKind k)
{
  switch (k)
  {
  case oh::RuleKind::Open: return osmoh::RuleSequence::Modifier::DefaultOpen;
  case oh::RuleKind::Closed: return osmoh::RuleSequence::Modifier::Closed;
  case oh::RuleKind::Unknown: return osmoh::RuleSequence::Modifier::Unknown;
  }
  return osmoh::RuleSequence::Modifier::DefaultOpen;
}

oh::RuleKind OsmohKind(osmoh::RuleSequence::Modifier m)
{
  switch (m)
  {
  case osmoh::RuleSequence::Modifier::Closed: return oh::RuleKind::Closed;
  case osmoh::RuleSequence::Modifier::Unknown: return oh::RuleKind::Unknown;
  default: return oh::RuleKind::Open;  // DefaultOpen, Open, Comment
  }
}

std::string PortOp(oh::RuleOperator op)
{
  switch (op)
  {
  case oh::RuleOperator::Additional: return ",";
  case oh::RuleOperator::Fallback: return "||";
  case oh::RuleOperator::Normal: return ";";
  }
  return ";";
}

oh::RuleOperator OsmohOp(std::string const & sep)
{
  if (sep == ",")
    return oh::RuleOperator::Additional;
  if (sep == "||")
    return oh::RuleOperator::Fallback;
  return oh::RuleOperator::Normal;
}

// ---- time -----------------------------------------------------------------

osmoh::Time PortTime(oh::Time const & t)
{
  if (t.tag == oh::Time::Variable)
  {
    osmoh::TimeEvent te;
    // osmoh only models Sunrise/Sunset; map dawn->sunrise, dusk->sunset.
    bool const sunset = t.variable.event == oh::TimeEvent::Sunset || t.variable.event == oh::TimeEvent::Dusk;
    te.SetEvent(sunset ? osmoh::TimeEvent::Event::Sunset : osmoh::TimeEvent::Event::Sunrise);
    if (t.variable.offset != 0)
      te.SetOffset(osmoh::HourMinutes(std::chrono::minutes(t.variable.offset)));
    return osmoh::Time(te);
  }
  osmoh::HourMinutes hm;
  hm.SetHours(std::chrono::hours(t.fixed.hour));
  hm.SetMinutes(std::chrono::minutes(t.fixed.minute));
  return osmoh::Time(hm);
}

oh::Time OsmohTime(osmoh::Time const & t)
{
  if (t.IsEvent())
  {
    oh::VariableTime vt;
    vt.event = t.GetEvent().GetEvent() == osmoh::TimeEvent::Event::Sunset ? oh::TimeEvent::Sunset : oh::TimeEvent::Sunrise;
    vt.offset = static_cast<int16_t>(t.GetEvent().GetOffset().GetDurationCount());
    return oh::Time::make_variable(vt);
  }
  auto const & hm = t.GetHourMinutes();
  return oh::Time::make_fixed(oh::ExtendedTime(static_cast<uint8_t>(hm.GetHoursCount()),
                                               static_cast<uint8_t>(hm.GetMinutesCount())));
}

osmoh::Timespan PortSpan(oh::TimeSpan const & s)
{
  osmoh::Timespan out;
  out.SetStart(PortTime(s.start));
  if (s.open_end)
  {
    // "10:00+" is open-ended: osmoh leaves the end empty (the port fills 24:00).
    out.SetPlus(true);
    bool const impliedMidnight =
        s.end.tag == oh::Time::Fixed && s.end.fixed == oh::ExtendedTime::midnight_24();
    if (!impliedMidnight)
      out.SetEnd(PortTime(s.end));
  }
  else
  {
    out.SetEnd(PortTime(s.end));
  }
  if (s.repeats_minutes)
    out.SetPeriod(osmoh::TimespanPeriod(osmoh::HourMinutes::TMinutes(*s.repeats_minutes)));
  return out;
}

oh::TimeSpan OsmohSpan(osmoh::Timespan const & s)
{
  oh::TimeSpan out;
  out.start = OsmohTime(s.GetStart());
  out.end = OsmohTime(s.GetEnd());
  out.open_end = s.HasPlus();
  if (s.HasPeriod())
    out.repeats_minutes = static_cast<int16_t>(s.GetPeriod().GetMinutesCount());
  return out;
}

// ---- weekdays + holidays --------------------------------------------------

osmoh::Weekdays PortWeekdays(std::vector<oh::WeekDayRange> const & weekday)
{
  osmoh::Weekdays out;
  for (auto const & w : weekday)
  {
    if (w.kind == oh::WeekDayRangeKind::Holiday)
    {
      osmoh::Holiday h;
      h.SetPlural(w.holiday_kind == oh::HolidayKind::Public);  // PH is "plural" in osmoh.
      if (w.offset != 0)
        h.SetOffset(static_cast<int32_t>(w.offset));
      out.AddHoliday(h);
    }
    else
    {
      osmoh::WeekdayRange r;
      r.SetStart(PortWday(w.range_start));
      if (w.range_end != w.range_start)
        r.SetEnd(PortWday(w.range_end));
      if (w.offset != 0)
        r.SetOffset(static_cast<int32_t>(w.offset));
      if (w.nth_has_filter())
      {
        for (int i = 0; i < 5; ++i)
        {
          if (w.nth_from_start[i])
          {
            osmoh::NthWeekdayOfTheMonthEntry e;
            e.SetStart(static_cast<osmoh::NthWeekdayOfTheMonthEntry::NthDayOfTheMonth>(i + 1));
            r.AddNth(e);
          }
        }
      }
      out.AddWeekdayRange(r);
    }
  }
  return out;
}

std::vector<oh::WeekDayRange> OsmohWeekdays(osmoh::Weekdays const & wd)
{
  std::vector<oh::WeekDayRange> out;
  for (auto const & r : wd.GetWeekdayRanges())
  {
    oh::Weekday const start = OsmohWday(r.GetStart());
    oh::Weekday const end = r.HasEnd() ? OsmohWday(r.GetEnd()) : start;
    auto w = oh::WeekDayRange::make_fixed(start, end, r.GetOffset());
    if (r.HasNth())
    {
      for (auto & b : w.nth_from_start)
        b = false;
      for (auto & b : w.nth_from_end)
        b = false;
      for (auto const & nth : r.GetNths())
      {
        int const s = static_cast<int>(nth.GetStart());
        int const e = nth.HasEnd() ? static_cast<int>(nth.GetEnd()) : s;
        for (int i = s; i <= e; ++i)
          if (i >= 1 && i <= 5)
            w.nth_from_start[i - 1] = true;
      }
    }
    out.push_back(w);
  }
  for (auto const & h : wd.GetHolidays())
  {
    auto w = oh::WeekDayRange::make_holiday(h.IsPlural() ? oh::HolidayKind::Public : oh::HolidayKind::School,
                                            h.GetOffset());
    out.push_back(w);
  }
  return out;
}

// ---- date / monthday ------------------------------------------------------

osmoh::DateOffset PortDateOffset(oh::DateOffset const & o)
{
  osmoh::DateOffset out;
  if (o.wday_kind != oh::WeekDayOffsetKind::None)
  {
    out.SetWDayOffset(PortWday(o.wday));
    out.SetWDayOffsetPositive(o.wday_kind == oh::WeekDayOffsetKind::Next);
  }
  if (o.day_offset != 0)
    out.SetOffset(static_cast<int32_t>(o.day_offset));
  return out;
}

oh::DateOffset OsmohDateOffset(osmoh::DateOffset const & o)
{
  oh::DateOffset out;
  if (o.HasWDayOffset())
  {
    out.wday_kind = o.IsWDayOffsetPositive() ? oh::WeekDayOffsetKind::Next : oh::WeekDayOffsetKind::Prev;
    out.wday = OsmohWday(o.GetWDayOffset());
  }
  out.day_offset = o.GetOffset();
  return out;
}

osmoh::MonthDay PortDate(oh::Date const & d, oh::DateOffset const & off)
{
  osmoh::MonthDay md;
  if (d.year)
    md.SetYear(*d.year);
  switch (d.kind)
  {
  case oh::DateKind::Fixed:
    md.SetMonth(PortMonth(d.month));
    if (d.day != 0)
      md.SetDayNum(d.day);
    break;
  case oh::DateKind::Easter:
    md.SetVariableDate(osmoh::MonthDay::VariableDate::Easter);
    break;
  case oh::DateKind::WeekdayInMonth:
    // osmoh MonthDay cannot express weekday-in-month; keep the month only.
    md.SetMonth(PortMonth(d.month));
    break;
  }
  osmoh::DateOffset const o = PortDateOffset(off);
  if (!o.IsEmpty())
    md.SetOffset(o);
  return md;
}

osmoh::MonthdayRange PortMonthday(oh::MonthdayRange const & m)
{
  osmoh::MonthdayRange out;
  if (m.kind == oh::MonthdayRangeKind::MonthOnly)
  {
    osmoh::MonthDay start;
    if (m.month_year)
      start.SetYear(*m.month_year);
    start.SetMonth(PortMonth(m.month_start));
    out.SetStart(start);
    if (m.month_end != m.month_start)
    {
      osmoh::MonthDay end;
      end.SetMonth(PortMonth(m.month_end));
      out.SetEnd(end);
    }
  }
  else
  {
    out.SetStart(PortDate(m.date_start, m.offset_start));
    if (!(m.date_start == m.date_end && m.offset_start == m.offset_end))
      out.SetEnd(PortDate(m.date_end, m.offset_end));
  }
  return out;
}

oh::Date OsmohDate(osmoh::MonthDay const & md, oh::DateOffset & offOut)
{
  offOut = OsmohDateOffset(md.GetOffset());
  std::optional<uint16_t> year;
  if (md.HasYear())
    year = md.GetYear();
  if (md.IsVariable())
    return oh::Date::make_easter(year);
  oh::Month const month = md.HasMonth() ? OsmohMonth(md.GetMonth()) : oh::Month::Jan;
  return oh::Date::make_fixed(year, month, md.GetDayNum());
}

oh::MonthdayRange OsmohMonthday(osmoh::MonthdayRange const & m)
{
  auto const & start = m.GetStart();
  auto const & end = m.GetEnd();
  auto isPureMonth = [](osmoh::MonthDay const & d)
  { return d.HasMonth() && !d.HasDayNum() && !d.IsVariable() && d.GetOffset().IsEmpty(); };

  if (isPureMonth(start) && (m.GetEnd().IsEmpty() || isPureMonth(end)))
  {
    std::optional<uint16_t> year;
    if (start.HasYear())
      year = start.GetYear();
    oh::Month const s = OsmohMonth(start.GetMonth());
    oh::Month const e = end.HasMonth() ? OsmohMonth(end.GetMonth()) : s;
    return oh::MonthdayRange::make_month(year, s, e);
  }

  oh::DateOffset os, oe;
  oh::Date const ds = OsmohDate(start, os);
  oh::Date de = ds;
  oe = os;
  if (!end.IsEmpty() || end.HasDayNum())
    de = OsmohDate(end, oe);
  return oh::MonthdayRange::make_date(ds, os, de, oe);
}

// ---- years / weeks --------------------------------------------------------

osmoh::YearRange PortYear(oh::YearRange const & y)
{
  osmoh::YearRange out;
  out.SetStart(y.start);
  if (y.end != y.start)
  {
    out.SetEnd(y.end);
    if (y.step != 1)
      out.SetPeriod(y.step);
  }
  return out;
}

oh::YearRange OsmohYear(osmoh::YearRange const & y)
{
  oh::YearRange out;
  out.start = y.GetStart();
  out.end = y.HasEnd() ? y.GetEnd() : y.GetStart();
  out.step = y.HasPeriod() ? static_cast<uint16_t>(y.GetPeriod()) : 1;
  return out;
}

osmoh::WeekRange PortWeek(oh::WeekRange const & w)
{
  osmoh::WeekRange out;
  out.SetStart(w.start);
  if (w.end != w.start)
  {
    out.SetEnd(w.end);
    if (w.step != 1)
      out.SetPeriod(w.step);
  }
  return out;
}

oh::WeekRange OsmohWeek(osmoh::WeekRange const & w)
{
  oh::WeekRange out;
  out.start = w.GetStart();
  out.end = w.HasEnd() ? w.GetEnd() : w.GetStart();
  out.step = w.HasPeriod() ? static_cast<uint8_t>(w.GetPeriod()) : 1;
  return out;
}
}  // namespace

namespace osmoh
{
TRuleSequences ToOsmoh(oh::OpeningHoursExpression const & expr)
{
  TRuleSequences rules;
  rules.reserve(expr.rules.size());
  for (auto const & r : expr.rules)
  {
    RuleSequence out;
    out.SetModifier(PortKind(r.kind));
    out.SetAnySeparator(PortOp(r.op));

    if (r.is_constant())
    {
      // Only an open all-day rule is "24/7". A constant closed/unknown rule
      // ("off"/"closed"/"24/7 closed") maps to an empty rule carrying just the
      // modifier, matching osmoh's historic form and keeping IsTwentyFourHours()
      // false so closed places don't show a green 24/7 badge.
      if (r.kind == opening_hours::RuleKind::Open)
        out.SetTwentyFourHours(true);
    }
    else
    {
      if (!r.day_selector.year.empty())
      {
        TYearRanges years;
        for (auto const & y : r.day_selector.year)
          years.push_back(PortYear(y));
        out.SetYears(years);
      }
      if (!r.day_selector.monthday.empty())
      {
        TMonthdayRanges months;
        for (auto const & m : r.day_selector.monthday)
          months.push_back(PortMonthday(m));
        out.SetMonths(months);
      }
      if (!r.day_selector.week.empty())
      {
        TWeekRanges weeks;
        for (auto const & w : r.day_selector.week)
          weeks.push_back(PortWeek(w));
        out.SetWeeks(weeks);
      }
      if (!r.day_selector.weekday.empty())
        out.SetWeekdays(PortWeekdays(r.day_selector.weekday));
      if (!r.time_selector.is_00_24())
      {
        TTimespans times;
        for (auto const & s : r.time_selector.time)
          times.push_back(PortSpan(s));
        out.SetTimes(times);
      }
    }

    if (!r.comments.empty())
      out.SetComment(r.comments.front());

    rules.push_back(out);
  }
  return rules;
}

oh::OpeningHoursExpression ToPort(TRuleSequences const & rules)
{
  oh::OpeningHoursExpression expr;
  expr.rules.reserve(rules.size());
  for (auto const & r : rules)
  {
    oh::RuleSequence out;
    out.kind = OsmohKind(r.GetModifier());
    out.op = OsmohOp(r.GetAnySeparator());

    if (!r.IsTwentyFourHours())
    {
      for (auto const & y : r.GetYears())
        out.day_selector.year.push_back(OsmohYear(y));
      for (auto const & m : r.GetMonths())
        out.day_selector.monthday.push_back(OsmohMonthday(m));
      for (auto const & w : r.GetWeeks())
        out.day_selector.week.push_back(OsmohWeek(w));
      if (r.HasWeekdays())
        out.day_selector.weekday = OsmohWeekdays(r.GetWeekdays());
      if (r.HasTimes())
      {
        std::vector<oh::TimeSpan> spans;
        for (auto const & s : r.GetTimes())
          spans.push_back(OsmohSpan(s));
        out.time_selector = oh::TimeSelector(std::move(spans));
      }
    }

    if (r.HasComment())
      out.comments.push_back(r.GetComment());

    expr.rules.push_back(std::move(out));
  }
  return expr;
}
}  // namespace osmoh
