#include "opening_hours/oh/convert.hpp"

#include <chrono>

namespace
{

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
  // A bare-comment rule means "unknown, see the comment".
  case osmoh::RuleSequence::Modifier::Comment: return oh::RuleKind::Unknown;
  default: return oh::RuleKind::Open;  // DefaultOpen, Open
  }
}

osmoh::TimeEvent::Event PortEvent(oh::TimeEvent e)
{
  switch (e)
  {
  case oh::TimeEvent::Dawn: return osmoh::TimeEvent::Event::Dawn;
  case oh::TimeEvent::Sunrise: return osmoh::TimeEvent::Event::Sunrise;
  case oh::TimeEvent::Sunset: return osmoh::TimeEvent::Event::Sunset;
  case oh::TimeEvent::Dusk: return osmoh::TimeEvent::Event::Dusk;
  }
  return osmoh::TimeEvent::Event::Sunrise;
}

oh::TimeEvent OsmohEvent(osmoh::TimeEvent::Event e)
{
  switch (e)
  {
  case osmoh::TimeEvent::Event::Dawn: return oh::TimeEvent::Dawn;
  case osmoh::TimeEvent::Event::Sunset: return oh::TimeEvent::Sunset;
  case osmoh::TimeEvent::Event::Dusk: return oh::TimeEvent::Dusk;
  // None cannot reach here: osmoh::Time only reports IsEvent() for a set event.
  case osmoh::TimeEvent::Event::None:
  case osmoh::TimeEvent::Event::Sunrise: return oh::TimeEvent::Sunrise;
  }
  return oh::TimeEvent::Sunrise;
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
    te.SetEvent(PortEvent(t.variable.event));
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
    vt.event = OsmohEvent(t.GetEvent().GetEvent());
    vt.offset = static_cast<int16_t>(t.GetEvent().GetOffset().GetDurationCount());
    return oh::Time::make_variable(vt);
  }
  auto const & hm = t.GetHourMinutes();
  return oh::Time::make_fixed(
      oh::ExtendedTime(static_cast<uint8_t>(hm.GetHoursCount()), static_cast<uint8_t>(hm.GetMinutesCount())));
}

osmoh::Timespan PortSpan(oh::TimeSpan const & s)
{
  osmoh::Timespan out;
  out.SetStart(PortTime(s.start));
  if (s.open_end)
  {
    // "10:00+" is open-ended: osmoh leaves the end empty (the port fills 24:00).
    out.SetPlus(true);
    bool const impliedMidnight = s.end.tag == oh::Time::Fixed && s.end.fixed == oh::ExtendedTime::midnight_24();
    if (!impliedMidnight)
      out.SetEnd(PortTime(s.end));
  }
  else
  {
    out.SetEnd(PortTime(s.end));
  }
  if (s.repeats_minutes)
  {
    // Print periods of an hour and more in the hh:mm form ("/21:03"), shorter
    // ones as plain minutes ("/03") -- the shapes the values were written in.
    int const minutes = *s.repeats_minutes;
    if (minutes >= 60)
      out.SetPeriod(osmoh::TimespanPeriod(osmoh::HourMinutes(osmoh::HourMinutes::TMinutes(minutes))));
    else
      out.SetPeriod(osmoh::TimespanPeriod(osmoh::HourMinutes::TMinutes(minutes)));
  }
  return out;
}

oh::TimeSpan OsmohSpan(osmoh::Timespan const & s)
{
  oh::TimeSpan out;
  out.start = OsmohTime(s.GetStart());
  // osmoh leaves the end empty for "10:00+"; the port's canonical form is an
  // implied 24:00 end (mirrors PortSpan in the other direction).
  if (s.HasPlus() && !s.HasEnd())
    out.end = oh::Time::make_fixed(oh::ExtendedTime::midnight_24());
  else
    out.end = OsmohTime(s.GetEnd());
  out.open_end = s.HasPlus();
  if (s.HasPeriod())
  {
    auto const & period = s.GetPeriod();
    out.repeats_minutes = static_cast<int16_t>(period.IsHoursMinutes() ? period.GetHourMinutes().GetHoursCount() * 60 +
                                                                             period.GetHourMinutes().GetMinutesCount()
                                                                       : period.GetMinutesCount());
  }
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
        auto const addNth = [&r](int nth, bool fromEnd)
        {
          osmoh::NthWeekdayOfTheMonthEntry e;
          auto const day = static_cast<osmoh::NthWeekdayOfTheMonthEntry::NthDayOfTheMonth>(nth);
          // End-only is osmoh's negative form ("Su[-1]", from the month's end).
          if (fromEnd)
            e.SetEnd(day);
          else
            e.SetStart(day);
          r.AddNth(e);
        };
        for (int i = 0; i < 5; ++i)
        {
          if (w.nth_from_start[i])
            addNth(i + 1, false /* fromEnd */);
          if (w.nth_from_end[i])
            addNth(i + 1, true /* fromEnd */);
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
        if (!nth.HasStart() && nth.HasEnd())
        {
          // End-only is osmoh's negative form ("Su[-1]", from the month's end).
          int const e = static_cast<int>(nth.GetEnd());
          if (e >= 1 && e <= 5)
            w.nth_from_end[e - 1] = true;
          continue;
        }
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
    auto w =
        oh::WeekDayRange::make_holiday(h.IsPlural() ? oh::HolidayKind::Public : oh::HolidayKind::School, h.GetOffset());
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
  case oh::DateKind::Easter: md.SetVariableDate(osmoh::MonthDay::VariableDate::Easter); break;
  case oh::DateKind::WeekdayInMonth:
    md.SetMonth(PortMonth(d.month));
    md.SetWeekdayInMonth(PortWday(d.weekday), d.nth);
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

oh::Date OsmohDate(osmoh::MonthDay const & md, oh::DateOffset & offOut, bool isRangeEnd = false)
{
  offOut = OsmohDateOffset(md.GetOffset());
  std::optional<uint16_t> year;
  if (md.HasYear())
    year = md.GetYear();
  if (md.IsVariable())
    return oh::Date::make_easter(year);
  oh::Month const month = md.HasMonth() ? OsmohMonth(md.GetMonth()) : oh::Month::Jan;
  if (md.HasWeekdayInMonth())
    return oh::Date::make_weekday_in_month(year, month, OsmohWday(md.GetWeekday()), md.GetNth());
  // A month without a day means the whole month: day 1 as a range start, the
  // month's last day as a range end (31 snaps down through the evaluator's
  // valid-date clamping). Day 0 would underflow that clamping.
  uint8_t day = md.GetDayNum();
  if (day == 0)
    day = isRangeEnd ? 31 : 1;
  return oh::Date::make_fixed(year, month, day);
}

oh::MonthdayRange OsmohMonthday(osmoh::MonthdayRange const & m)
{
  auto const & start = m.GetStart();
  auto const & end = m.GetEnd();
  auto isPureMonth = [](osmoh::MonthDay const & d)
  { return d.HasMonth() && !d.HasDayNum() && !d.IsVariable() && !d.HasWeekdayInMonth() && d.GetOffset().IsEmpty(); };

  // The port's MonthOnly range carries a single year, so a range whose ends
  // lie in different years must go through the date form below.
  bool const sameYear = !start.HasYear() || !end.HasYear() || start.GetYear() == end.GetYear();
  if (isPureMonth(start) && (m.GetEnd().IsEmpty() || (isPureMonth(end) && sameYear)))
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
  if (!end.IsEmpty())
    de = OsmohDate(end, oe, true /* isRangeEnd */);
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
  // Every port construct has an osmoh form, so this conversion is total. It
  // must stay that way: a partial list would shift the rule separators onto the
  // wrong rules while still looking complete to callers.
  TRuleSequences rules;
  rules.reserve(expr.rules.size());
  for (auto const & r : expr.rules)
  {
    RuleSequence out;
    // A rule whose unknown state comes from a comment maps to osmoh's
    // Comment modifier (printed as just "comment"): the parser lowers bare
    // comments to Unknown+Fallback and selector'd comment-only modifiers to
    // Unknown. An explicit constant "unknown \"x\"" keeps its keyword so that
    // its normal (overriding) attachment survives a GetRule() round trip.
    bool const commentRule = r.kind == oh::RuleKind::Unknown && !r.comments.empty() &&
                             (!r.is_constant() || r.op == oh::RuleOperator::Fallback);
    if (commentRule)
      out.SetModifier(RuleSequence::Modifier::Comment);
    else if (r.kind == oh::RuleKind::Open && !r.comments.empty())
    {
      // DefaultOpen is not printed, which would leave the comment alone -- and
      // a bare comment means unknown, so "Mo-Fr 08:00-16:00 open \"note\"" would
      // come back as unknown. Keep the explicit keyword.
      out.SetModifier(RuleSequence::Modifier::Open);
    }
    else
      out.SetModifier(PortKind(r.kind));

    if (r.is_constant())
    {
      // Only an open all-day rule is "24/7". A constant closed/unknown rule
      // ("off"/"closed"/"24/7 closed") maps to an empty rule carrying just the
      // modifier, matching osmoh's historic form and keeping IsTwentyFourHours()
      // false so closed places don't show a green 24/7 badge.
      if (r.kind == oh::RuleKind::Open)
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
    {
      // Join multiple comments the way evaluation reports them.
      std::string joined = r.comments.front();
      for (size_t c = 1; c < r.comments.size(); ++c)
        joined.append(", ").append(r.comments[c]);
      out.SetModifierComment(std::move(joined));
    }

    rules.push_back(std::move(out));
  }

  // osmoh stores the separator FOLLOWING each rule (the printer emits rule[i],
  // then rule[i]'s separator, then rule[i+1]), while the port's op says how a
  // rule attaches to the PREVIOUS one: shift by one position. The bound is
  // rules.size(), which the all-or-nothing conversion above keeps equal to
  // expr.rules.size().
  for (size_t i = 0; i + 1 < rules.size(); ++i)
  {
    auto const nextOp = expr.rules[i + 1].op;
    rules[i].SetAnySeparator(PortOp(nextOp));

    // A rule with no printed time span followed by an additive one is
    // ambiguous: "Tu-Fr, Sa 00:00-22:00" re-parses as the single weekday list
    // "Tu-Fr,Sa". Spell the implied all-day span out to keep the rules apart.
    if (nextOp == oh::RuleOperator::Additional && !rules[i].HasTimes() && !rules[i].IsTwentyFourHours())
      rules[i].SetTimes(
          {PortSpan(oh::TimeSpan::fixed_range(oh::ExtendedTime::midnight_00(), oh::ExtendedTime::midnight_24()))});

    // The reverse ambiguity: an additive rule with times but no day selector
    // ("Mo-Fr 08:00-18:00 open, 19:00-21:00") would print as a bare time list
    // and re-parse as extra spans of the previous rule. Spell its implied
    // every-day selector out, mirroring the port printer's force_day_selector.
    auto & next = rules[i + 1];
    if (nextOp == oh::RuleOperator::Additional && next.HasTimes() && !next.GetWeekdays().HasWeekday() &&
        !next.GetWeekdays().HasHolidays() && next.GetMonths().empty() && next.GetYears().empty() &&
        next.GetWeeks().empty())
    {
      WeekdayRange range;
      range.SetStart(Weekday::Monday);
      range.SetEnd(Weekday::Sunday);
      Weekdays weekdays;
      weekdays.SetWeekdayRanges({range});
      next.SetWeekdays(weekdays);
    }
  }

  return rules;
}

oh::OpeningHoursExpression ToPort(TRuleSequences const & rules)
{
  oh::OpeningHoursExpression expr;
  expr.rules.reserve(rules.size());
  for (size_t i = 0; i < rules.size(); ++i)
  {
    auto const & r = rules[i];
    oh::RuleSequence out;
    out.kind = OsmohKind(r.GetModifier());
    // The separator preceding rule i is stored on rule i-1 (see ToOsmoh).
    out.op = i == 0 ? oh::RuleOperator::Normal : OsmohOp(rules[i - 1].GetAnySeparator());
    // A bare-comment rule (no selectors at all) must stay a fallback: as a
    // normal rule it would shadow every preceding rule (it matches all days).
    // Selector'd comment rules ("Mo-Fr \"call us\"") keep their attachment.
    if (r.GetModifier() == osmoh::RuleSequence::Modifier::Comment && r.IsEmpty() && !r.IsTwentyFourHours())
      out.op = oh::RuleOperator::Fallback;

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

    if (r.HasModifierComment())
      out.comments.push_back(r.GetModifierComment());

    expr.rules.push_back(std::move(out));
  }
  return expr;
}
}  // namespace osmoh
