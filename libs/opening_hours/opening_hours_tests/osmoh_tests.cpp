// Compatibility tests for the osmoh AST and OpeningHours facade (MIT,
// (c) 2015 Mail.Ru Group). The AST backs GetRule() for the editor, routing
// serdes and transit; evaluation follows the opening-hours-rs semantics.

#include "testing/testing.hpp"

#include "opening_hours/opening_hours.hpp"

#include <cstdlib>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <string>

namespace osmoh_tests
{
using osmoh::OpeningHours;

template <typename T>
std::string ToString(T const & t)
{
  std::stringstream sstr;
  sstr << t;
  return sstr.str();
}

std::string RoundTrip(std::string const & str)
{
  OpeningHours const oh(str);
  if (!oh.IsValid())
    return ":CAN'T PARSE:";
  return osmoh::ToString(oh);
}

void CheckRoundTrip(std::string const & rule)
{
  TEST_EQUAL(RoundTrip(rule), rule, ());
}

void CheckRoundTrip(std::string const & rule, std::string const & expected)
{
  TEST_EQUAL(RoundTrip(rule), expected, ());
}

bool GetTimeTuple(std::string const & strTime, std::string const & fmt, std::tm & tm)
{
  std::istringstream input(strTime);
  input >> std::get_time(&tm, fmt.data());
  if (input.fail())
    return false;
  // std::get_time doesn't set tm_isdst. Set to -1 so mktime auto-detects DST,
  // otherwise mktime assumes standard time (tm_isdst=0) and shifts the
  // timestamp by 1 hour during DST periods, potentially crossing midnight.
  tm.tm_isdst = -1;
  return true;
}

time_t MakeTimestamp(std::string const & dateTime, char const * fmt = "%Y-%m-%d %H:%M")
{
  std::tm time = {};
  TEST(GetTimeTuple(dateTime, fmt, time), (dateTime));
  return mktime(&time);
}

bool Parse(std::string const & str, OpeningHours & oh)
{
  oh = OpeningHours(str);
  return oh.IsValid();
}

bool IsOpen(OpeningHours const & oh, std::string const & dateTime)
{
  return oh.IsOpen(MakeTimestamp(dateTime));
}

bool IsClosed(OpeningHours const & oh, std::string const & dateTime)
{
  return oh.IsClosed(MakeTimestamp(dateTime));
}

bool IsUnknown(OpeningHours const & oh, std::string const & dateTime)
{
  return oh.IsUnknown(MakeTimestamp(dateTime));
}

std::string FormatTime(time_t t, char const * fmt)
{
  std::tm const tm = *localtime(&t);
  char buffer[30];
  std::strftime(buffer, sizeof(buffer) / sizeof(buffer[0]), fmt, &tm);
  return std::string(buffer);
}

std::string GetNextTimeOpen(OpeningHours const & oh, char const * fmt, std::string const & dateTime)
{
  return FormatTime(oh.GetInfo(MakeTimestamp(dateTime, fmt)).nextTimeOpen, fmt);
}

std::string GetNextTimeClosed(OpeningHours const & oh, char const * fmt, std::string const & dateTime)
{
  return FormatTime(oh.GetInfo(MakeTimestamp(dateTime, fmt)).nextTimeClosed, fmt);
}

UNIT_TEST(OpeningHours_TestHourMinutes)
{
  using namespace osmoh;

  {
    TEST(HourMinutes().IsEmpty(), ());
    TEST_EQUAL(ToString(HourMinutes()), "hh:mm", ());
  }
  {
    HourMinutes hm(10_min);
    TEST(!hm.IsEmpty(), ());
    TEST_EQUAL(ToString(hm), "00:10", ());
  }
  {
    HourMinutes hm(100_min);
    TEST(!hm.IsEmpty(), ());
    TEST_EQUAL(hm.GetHoursCount(), 1, ());
    TEST_EQUAL(hm.GetMinutesCount(), 40, ());

    TEST_EQUAL(ToString(hm), "01:40", ());
  }
  {
    HourMinutes hm;
    hm.SetHours(22_h);
    hm.SetMinutes(15_min);
    TEST(!hm.IsEmpty(), ());
    TEST(!hm.IsExtended(), ());

    TEST_EQUAL(hm.GetHoursCount(), 22, ());
    TEST_EQUAL(hm.GetMinutesCount(), 15, ());

    TEST_EQUAL(ToString(hm), "22:15", ());
  }
  {
    HourMinutes hm;
    hm.SetHours(39_h);
    hm.SetMinutes(15_min);
    TEST(!hm.IsEmpty(), ());
    TEST(hm.IsExtended(), ());

    TEST_EQUAL(hm.GetHoursCount(), 39, ());
    TEST_EQUAL(hm.GetMinutesCount(), 15, ());

    TEST_EQUAL(ToString(hm), "39:15", ());
  }
}

UNIT_TEST(OpeningHours_TestTimeEvent)
{
  using namespace osmoh;

  {
    TEST(TimeEvent().IsEmpty(), ());
    TEST(!TimeEvent().HasOffset(), ());
  }
  {
    TimeEvent te(TimeEvent::Event::Sunrise);
    TEST(!te.IsEmpty(), ());
    TEST(!te.HasOffset(), ());

    TEST_EQUAL(ToString(te), "sunrise", ());
  }
  {
    TimeEvent te(TimeEvent::Event::Sunset);
    TEST(!te.IsEmpty(), ());
    TEST(!te.HasOffset(), ());

    TEST_EQUAL(ToString(te), "sunset", ());
  }
  {
    TimeEvent te(TimeEvent::Event::Sunrise);
    te.SetOffset(-HourMinutes(100_min));
    TEST(!te.IsEmpty(), ());
    TEST(te.HasOffset(), ());

    TEST_EQUAL(ToString(te), "(sunrise-01:40)", ());

    te.SetOffset(HourMinutes(100_min));
    TEST_EQUAL(ToString(te), "(sunrise+01:40)", ());
  }
}

UNIT_TEST(OpeningHours_TestTime)
{
  using namespace osmoh;

  {
    TEST(Time().IsEmpty(), ());
    TEST(!Time().IsHoursMinutes(), ());
    TEST(!Time().IsTime(), ());
    TEST(!Time().IsEvent(), ());
  }
  {
    Time time;
    time.SetEvent(TimeEvent::Event::Sunrise);
    TEST(!time.IsEmpty(), ());
    TEST(!time.IsHoursMinutes(), ());
    TEST(time.IsTime(), ());
    TEST(time.IsEvent(), ());

    TEST_EQUAL(ToString(time), "sunrise", ());

    time.AddDuration(-90_min);
    TEST_EQUAL(ToString(time), "(sunrise-01:30)", ());
  }
  {
    Time time{};
    time.SetHourMinutes(HourMinutes(22_h + 5_min));
    TEST(!time.IsEmpty(), ());
    TEST(time.IsHoursMinutes(), ());
    TEST(time.IsTime(), ());
    TEST(!time.IsEvent(), ());

    TEST_EQUAL(ToString(time), "22:05", ());

    time.AddDuration(10_min);
    TEST_EQUAL(ToString(time), "22:15", ());
  }
  {
    Time time{};
    time.SetHourMinutes(HourMinutes(22_h + 5_min));

    time.SetEvent(TimeEvent::Event::Sunset);
    TEST_EQUAL(ToString(time), "sunset", ());
  }
  {
    Time time(HourMinutes(27_h + 30_min));
    TEST_EQUAL(ToString(time), "27:30", ());
  }
}

UNIT_TEST(OpeningHours_TestTimespan)
{
  using namespace osmoh;

  {
    Timespan span;
    TEST(span.IsEmpty(), ());
    TEST(!span.HasStart(), ());
    TEST(!span.HasEnd(), ());
    TEST(!span.HasExtendedHours(), ());
    TEST_EQUAL(ToString(span), "hh:mm-hh:mm", ());

    span.SetStart(HourMinutes(10_h));
    TEST(span.HasStart(), ());
    TEST(span.IsOpen(), ());
    TEST(!span.HasExtendedHours(), ());
    TEST_EQUAL(ToString(span), "10:00", ());

    span.SetEnd(HourMinutes(12_h));
    TEST(span.HasEnd(), ());
    TEST(!span.IsOpen(), ());
    TEST(!span.HasExtendedHours(), ());
    TEST_EQUAL(ToString(span), "10:00-12:00", ());

    TEST(!span.HasPeriod(), ());
    span.SetPeriod(10_min);
    TEST(span.HasPeriod(), ());
    TEST_EQUAL(ToString(span), "10:00-12:00/10", ());
  }
  {
    Timespan span;

    span.SetStart(HourMinutes(10_h));
    span.SetEnd(HourMinutes(47_h));

    TEST(span.HasExtendedHours(), ());
    TEST_EQUAL(ToString(span), "10:00-47:00", ());
  }
  {
    Timespan span;

    span.SetStart(HourMinutes(10_h));
    span.SetEnd(HourMinutes(06_h));

    TEST(span.HasExtendedHours(), ());
    TEST_EQUAL(ToString(span), "10:00-06:00", ());
  }
  {
    Timespan span;

    span.SetStart(HourMinutes(10_h));
    span.SetEnd(HourMinutes(00_h));

    TEST(span.HasExtendedHours(), ());
    TEST_EQUAL(ToString(span), "10:00-00:00", ());
  }
}

UNIT_TEST(OpeningHours_TestNthWeekdayOfTheMonthEntry)
{
  using namespace osmoh;

  {
    NthWeekdayOfTheMonthEntry entry;
    TEST(entry.IsEmpty(), ());
    TEST(!entry.HasStart(), ());
    TEST(!entry.HasEnd(), ());
    TEST_EQUAL(ToString(entry), "", ());

    entry.SetStart(NthWeekdayOfTheMonthEntry::NthDayOfTheMonth::Third);
    TEST(!entry.IsEmpty(), ());
    TEST(entry.HasStart(), ());
    TEST(!entry.HasEnd(), ());
    TEST_EQUAL(ToString(entry), "3", ());

    entry.SetEnd(NthWeekdayOfTheMonthEntry::NthDayOfTheMonth::Fifth);
    TEST(!entry.IsEmpty(), ());
    TEST(entry.HasStart(), ());
    TEST(entry.HasEnd(), ());
    TEST_EQUAL(ToString(entry), "3-5", ());

    entry.SetStart(NthWeekdayOfTheMonthEntry::NthDayOfTheMonth::None);
    TEST(!entry.IsEmpty(), ());
    TEST(!entry.HasStart(), ());
    TEST(entry.HasEnd(), ());
    TEST_EQUAL(ToString(entry), "-5", ());
  }
}

UNIT_TEST(OpeningHours_TestWeekdayRange)
{
  using namespace osmoh;

  {
    WeekdayRange range;
    TEST(range.IsEmpty(), ());
    TEST(!range.HasEnd(), ());
    TEST(!range.HasSunday(), ());
    TEST(!range.HasWednesday(), ());
    TEST(!range.HasSaturday(), ());
    TEST(!range.HasNth(), ());
  }
  {
    WeekdayRange range;
    TEST(!range.HasNth(), ());

    range.SetStart(Weekday::Tuesday);
    TEST(!range.IsEmpty(), ());
    TEST(!range.HasEnd(), ());
    TEST(!range.HasSunday(), ());
    TEST(!range.HasWednesday(), ());
    TEST(range.HasTuesday(), ());
    TEST(!range.HasSaturday(), ());

    range.SetEnd(Weekday::Saturday);
    TEST(!range.IsEmpty(), ());
    TEST(range.HasStart(), ());
    TEST(range.HasEnd(), ());
    TEST(!range.HasSunday(), ());
    TEST(range.HasWednesday(), ());
    TEST(range.HasTuesday(), ());
    TEST(range.HasSaturday(), ());
  }
  {
    WeekdayRange range;
    TEST(!range.HasNth(), ());

    NthWeekdayOfTheMonthEntry entry;
    entry.SetStart(NthWeekdayOfTheMonthEntry::NthDayOfTheMonth::First);
    range.AddNth(entry);
    TEST(range.HasNth(), ());
  }
  {
    WeekdayRange range;
    range.SetStart(Weekday::Monday);
    range.SetEnd(Weekday::Sunday);

    TEST(range.HasSunday(), ());
    TEST(range.HasMonday(), ());
    TEST(range.HasTuesday(), ());
    TEST(range.HasWednesday(), ());
    TEST(range.HasThursday(), ());
    TEST(range.HasFriday(), ());
    TEST(range.HasSaturday(), ());
  }
}

UNIT_TEST(OpeningHours_Holidays)
{
  using namespace osmoh;

  {
    Holiday h;
    TEST(!h.IsPlural(), ());
    TEST_EQUAL(h.GetOffset(), 0, ());
    TEST_EQUAL(ToString(h), "SH", ());

    h.SetOffset(11);

    TEST_EQUAL(h.GetOffset(), 11, ());
    TEST_EQUAL(ToString(h), "SH +11 days", ());

    h.SetOffset(-1);
    TEST_EQUAL(ToString(h), "SH -1 day", ());

    h.SetPlural(true);
    TEST(h.IsPlural(), ());
    TEST_EQUAL(ToString(h), "PH -1 day", ());
  }
}

UNIT_TEST(OpeningHours_Weekdays)
{
  using namespace osmoh;

  {
    Weekdays w;
    TEST(w.IsEmpty(), ());
    TEST(!w.HasWeekday(), ());
    TEST(!w.HasHolidays(), ());

    TEST_EQUAL(ToString(w), "", ());

    WeekdayRange r;
    r.SetStart(Weekday::Sunday);
    w.AddHoliday(Holiday{});
    w.AddWeekdayRange(r);

    TEST_EQUAL(ToString(w), "SH, Su", ());
  }
}

UNIT_TEST(OpeningHours_DayOffset)
{
  using namespace osmoh;

  {
    DateOffset offset;
    TEST(offset.IsEmpty(), ());
    TEST(!offset.HasWDayOffset(), ());
    TEST(!offset.HasOffset(), ());
    TEST_EQUAL(ToString(offset), "", ());

    offset.SetWDayOffset(Weekday::Monday);
    TEST(!offset.IsEmpty(), ());
    TEST(offset.HasWDayOffset(), ());
    TEST_EQUAL(ToString(offset), "+Mo", ());

    offset.SetOffset(11);
    TEST(offset.HasOffset(), ());
    TEST_EQUAL(ToString(offset), "+Mo +11 days", ());

    TEST(offset.IsWDayOffsetPositive(), ());
    offset.SetWDayOffsetPositive(false);
    TEST(!offset.IsWDayOffsetPositive(), ());
    TEST_EQUAL(ToString(offset), "-Mo +11 days", ());
  }
}

UNIT_TEST(OpeningHours_TestMonthDay)
{
  using namespace osmoh;

  {
    MonthDay md;
    TEST(md.IsEmpty(), ());
    TEST(!md.HasYear(), ());
    TEST(!md.HasMonth(), ());
    TEST(!md.HasDayNum(), ());
    TEST(!md.HasOffset(), ());
    TEST(!md.IsVariable(), ());
    TEST_EQUAL(ToString(md), "", ());
  }
  {
    MonthDay md;
    md.SetVariableDate(MonthDay::VariableDate::Easter);
    TEST(!md.IsEmpty(), ());
    TEST_EQUAL(ToString(md), "easter", ());
  }
  {
    MonthDay md;
    md.SetMonth(MonthDay::Month::Jul);
    TEST(!md.IsEmpty(), ());
    TEST(md.HasMonth(), ());
    TEST_EQUAL(ToString(md), "Jul", ());

    md.SetYear(1990);
    TEST(!md.IsEmpty(), ());
    TEST(md.HasYear(), ());
    TEST(md.HasYear(), ());
    TEST_EQUAL(ToString(md), "1990 Jul", ());

    md.SetDayNum(17);
    TEST(!md.IsEmpty(), ());
    TEST(md.HasYear(), ());
    TEST(md.HasMonth(), ());
    TEST(md.HasDayNum(), ());
    TEST_EQUAL(ToString(md), "1990 Jul 17", ());

    DateOffset offset;
    offset.SetWDayOffset(Weekday::Monday);
    md.SetOffset(offset);
    TEST(md.HasOffset(), ());
    TEST_EQUAL(ToString(md), "1990 Jul 17 +Mo", ());
  }
}

UNIT_TEST(OpeningHours_TestMonthdayRange)
{
  using namespace osmoh;

  {
    MonthdayRange range;
    TEST(range.IsEmpty(), ());
    TEST(!range.HasStart(), ());
    TEST(!range.HasEnd(), ());
    TEST(!range.HasPeriod(), ());
    TEST(!range.HasPlus(), ());
    TEST_EQUAL(ToString(range), "", ());
  }
  {
    MonthdayRange range;
    MonthDay md;

    md.SetYear(1990);
    md.SetMonth(MonthDay::Month::Sep);
    range.SetStart(md);

    TEST(!range.IsEmpty(), ());
    TEST(range.HasStart(), ());
    TEST(!range.HasEnd(), ());
    TEST(!range.HasPeriod(), ());
    TEST(!range.HasPlus(), ());
    TEST_EQUAL(ToString(range), "1990 Sep", ());
  }
  {
    MonthdayRange range;
    MonthDay md;

    md.SetYear(1990);
    range.SetEnd(md);

    TEST(!range.IsEmpty(), ());
    TEST(!range.HasStart(), ());
    TEST(range.HasEnd(), ());
    TEST(!range.HasPeriod(), ());
    TEST(!range.HasPlus(), ());
  }
  {
    MonthdayRange range;

    range.SetPlus(true);
    TEST(range.IsEmpty(), ());
    TEST(!range.HasStart(), ());
    TEST(!range.HasEnd(), ());
    TEST(!range.HasPeriod(), ());
    TEST(range.HasPlus(), ());

    range.SetPeriod(7);
    TEST(range.IsEmpty(), ());
    TEST(!range.HasStart(), ());
    TEST(!range.HasEnd(), ());
    TEST(range.HasPeriod(), ());
    TEST(range.HasPlus(), ());
  }
}

UNIT_TEST(OpeningHours_YearRange)
{
  using namespace osmoh;

  {
    YearRange range;
    TEST(range.IsEmpty(), ());
    TEST(!range.HasStart(), ());
    TEST(!range.HasEnd(), ());
    TEST(!range.HasPlus(), ());
    TEST_EQUAL(ToString(range), "", ());

    range.SetStart(1812);
    TEST(range.HasStart(), ());
    TEST(range.IsOpen(), ());
    TEST_EQUAL(ToString(range), "1812", ());

    range.SetEnd(1815);
    TEST(range.HasEnd(), ());
    TEST(!range.IsOpen(), ());
    TEST_EQUAL(ToString(range), "1812-1815", ());

    TEST(!range.HasPeriod(), ());
    range.SetPeriod(10);
    TEST(range.HasPeriod(), ());
    TEST_EQUAL(ToString(range), "1812-1815/10", ());
  }
  {
    YearRange range;
    range.SetStart(1812);
    range.SetPlus(true);
    TEST(range.HasStart(), ());
    TEST(range.IsOpen(), ());
    TEST(range.HasPlus(), ());
    TEST_EQUAL(ToString(range), "1812+", ());
  }
}

UNIT_TEST(OpeningHours_WeekRange)
{
  using namespace osmoh;

  {
    WeekRange range;
    TEST(range.IsEmpty(), ());
    TEST(!range.HasStart(), ());
    TEST(!range.HasEnd(), ());
    TEST_EQUAL(ToString(range), "", ());

    range.SetStart(18);
    TEST(range.HasStart(), ());
    TEST(range.IsOpen(), ());
    TEST_EQUAL(ToString(range), "18", ());

    range.SetEnd(42);
    TEST(range.HasEnd(), ());
    TEST(!range.IsOpen(), ());
    TEST_EQUAL(ToString(range), "18-42", ());

    TEST(!range.HasPeriod(), ());
    range.SetPeriod(10);
    TEST(range.HasPeriod(), ());
    TEST_EQUAL(ToString(range), "18-42/10", ());
  }
}

UNIT_TEST(OpeningHours_RuleSequence)
{
  using namespace osmoh;

  {
    RuleSequence s;
  }
}

UNIT_TEST(OpeningHoursTimerange_TestParseUnparse)
{
  CheckRoundTrip("06:00+");
  CheckRoundTrip("06:00-02:00");
  CheckRoundTrip("06:00-31:41");
  CheckRoundTrip("06:00-02:00+");
  CheckRoundTrip("06:00-02:00/03");
  CheckRoundTrip("06:00-02:00/21:03");
  CheckRoundTrip("dusk+");
  CheckRoundTrip("dawn+");
  CheckRoundTrip("sunrise-sunset");
  CheckRoundTrip("(sunset-12:12)+");
  CheckRoundTrip("(dusk-12:12)+");
  CheckRoundTrip("(sunrise-12:12)-sunset");
}

UNIT_TEST(OpeningHoursWeekdays_TestParseUnparse)
{
  CheckRoundTrip("We[4] -2 days");
  CheckRoundTrip("Sa[4,5]");
  CheckRoundTrip("Mo[1,3]");
  CheckRoundTrip("Tu[4,5] +1 day");
  CheckRoundTrip("SH -2 days");
  CheckRoundTrip("SH +2 days");
  CheckRoundTrip("SH +1 day");
  CheckRoundTrip("PH");
  CheckRoundTrip("SH");
  CheckRoundTrip("Mo, We, Th, Fr");
  CheckRoundTrip("Fr-Sa");
  CheckRoundTrip("PH, Sa, Su");
  CheckRoundTrip("Sa");
}

UNIT_TEST(OpeningHoursMonthdayRanges_TestParseUnparse)
{
  CheckRoundTrip("Jan");
  // The port materializes the open end; the meaning is unchanged.
  CheckRoundTrip("Mar 10+", "Mar 10-Dec 31");
  CheckRoundTrip("Jan-Feb");
  // The port materializes the open end; the meaning is unchanged.
  CheckRoundTrip("easter -2 days+", "easter -2 days-Dec 31");
  // Month period lists are outside the specification and absent from the corpus.
  CheckRoundTrip("Jan-Feb/10", ":CAN'T PARSE:");
  CheckRoundTrip("Jan 11-Dec 10, Apr 01-Jun 02");
  CheckRoundTrip("2011 Jan");
  // The port materializes the open end; the meaning is unchanged.
  CheckRoundTrip("1989 Mar 10+", "1989 Mar 10-9999 Dec 31");
  // The port materializes the open end; the meaning is unchanged.
  CheckRoundTrip("Jan 11 +Mo+", "Jan 11 +Mo-Dec 31");
  // The port materializes the open end; the meaning is unchanged.
  CheckRoundTrip("Jan 11 +3 days+", "Jan 11 +3 days-Dec 31");
  // The port materializes the open end; the meaning is unchanged.
  CheckRoundTrip("Feb 03 -Mo -2 days+", "Feb 03 -Mo -2 days-Dec 31");
  CheckRoundTrip("Feb 03 -Mo -2 days-Jan 11 +3 days");
  CheckRoundTrip("Feb 03 -Mo -2 days-Jan 11 +3 days, Mar, Apr");
  CheckRoundTrip("1941 Feb 03 -Mo -2 days-1945 Jan 11 +3 days, Mar, Apr");
}

UNIT_TEST(OpeningHoursYearRanges_TestParseUnparse)
{
  /// @todo Single year was removed here:
  /// https://github.com/organicmaps/organicmaps/commit/ebe26a41da0744b3bc81d6b213406361f14d39b2
  /*
  CheckRoundTrip("1995");
  */

  // The port materializes the open end; the meaning is unchanged.
  CheckRoundTrip("1997+", "1997-9999");
  CheckRoundTrip("2018-2019");
  CheckRoundTrip("2018-2036/11");
}

UNIT_TEST(OpeningHoursWeekRanges_TestParseUnparse)
{
  CheckRoundTrip("week 15");
  CheckRoundTrip("week 19-31");
  CheckRoundTrip("week 18-36/3");
  // The port sorts week ranges; the meaning is unchanged.
  CheckRoundTrip("week 18-36/3, 11", "week 11, 18-36/3");
}

UNIT_TEST(OpeningHoursRuleSequence_TestParseUnparse)
{
  // An empty value is not a valid opening_hours.
  CheckRoundTrip("", ":CAN'T PARSE:");
  CheckRoundTrip("24/7");
  CheckRoundTrip("06:00-09:00/03");
  CheckRoundTrip("Apr-Sep Su[1,3] 14:30-17:00");
  CheckRoundTrip("06:00+");
  CheckRoundTrip("06:00-07:00+");
  CheckRoundTrip("Mo-Su 08:00-23:00");
  CheckRoundTrip("Mo-Sa; PH closed");
  CheckRoundTrip("Jan-Mar 07:00-19:00; Apr-Sep 07:00-22:00; Oct-Dec 07:00-19:00");
  CheckRoundTrip("10:00-13:30, 17:00-20:30");
  // The optional colon after wide-range selectors is dropped.
  CheckRoundTrip("Apr-Sep: Mo-Fr 09:00-13:00, 14:00-18:00; Apr-Sep: Sa 10:00-13:00",
                 "Apr-Sep Mo-Fr 09:00-13:00, 14:00-18:00; Apr-Sep Sa 10:00-13:00");
  // One selector list (the spaced comma is not an extra rule); the port prints holidays first.
  CheckRoundTrip("Tu-Su, PH 10:00-18:00", "PH, Tu-Su 10:00-18:00");
  CheckRoundTrip("Mo, We, Th, Fr 12:00-18:00; Sa-Su 12:00-17:00");
  CheckRoundTrip("2016-2025");
  CheckRoundTrip("Feb 03 -Mo -2 days-Jan 11 +3 days");
  CheckRoundTrip("week 19-31");
  CheckRoundTrip("06:00-02:00/21:03, 18:15-sunset");
  CheckRoundTrip("06:13-15:00; 16:30+");
  CheckRoundTrip("We-Sa; Mo[1,3] closed");
  CheckRoundTrip("Mo-Fr 10:00-18:00, Sa 10:00-13:00");
  {
    // The port drops the redundant explicit "open" modifier.
    auto const parsedUnparsed = RoundTrip(
        "We-Sa; Mo[1,3] closed; Su[-1,-2] closed; "
        "Fr[2] open; Fr[-2], Fr open; Su[-2] -2 days");
    TEST_EQUAL(parsedUnparsed,
               "We-Sa; Mo[1,3] closed; Su[-1,-2] closed; "
               "Fr[2]; Fr[-2], Fr; Su[-2] -2 days",
               ());
  }
  // The port materializes the open end; the meaning is unchanged.
  CheckRoundTrip("easter -2 days+: closed", "easter -2 days-Dec 31 closed");
  // The port drops the redundant open modifier and colon; the meaning is unchanged.
  CheckRoundTrip("easter: open", "easter");
  {
    // Canonicalizations: the redundant "open" is dropped, the bare-comment
    // fallback rule prints in its explicit "||" form, the open date end is
    // materialized. The meaning is unchanged.
    auto const parsedUnparsed = RoundTrip(
        "PH, Tu-Su 10:00-18:00; Sa[1] 10:00-18:00 open; "
        "\"Eintritt ins gesamte Haus frei\"; "
        "Jan 01, Dec 24, Dec 25, easter -2 days+: closed");
    TEST_EQUAL(parsedUnparsed,
               "PH, Tu-Su 10:00-18:00; Sa[1] 10:00-18:00 || "
               "\"Eintritt ins gesamte Haus frei\"; "
               "Jan 01, Dec 24, Dec 25, easter -2 days-Dec 31 closed",
               ());
  }
  CheckRoundTrip("Su-Th sunrise-(sunset-24:00); Fr-Sa (sunrise+12:12)-sunset");
  // The port canonicalizes this form; the meaning is unchanged.
  CheckRoundTrip("2010 Apr 01-30: Mo-Su 17:00-24:00", "2010 Apr 01-2010 Apr 30 Mo-Su 17:00-24:00");
  {
    auto const rule =
        ("Mo-Th 14:00-22:00; Fr 14:00-24:00; "
         "Sa 00:00-01:00, 14:00-24:00; "
         "Su 00:00-01:00, 14:00-22:00");
    auto const parsedUnparsed = RoundTrip(rule);
    TEST_EQUAL(parsedUnparsed, rule, ());
  }
  // A closed rule is not 24/7; the port drops the redundant selector.
  CheckRoundTrip("24/7 closed \"always closed\"", "closed \"always closed\"");
  CheckRoundTrip("Mo-Fr closed \"always closed\"");
  {
    auto const rule = "Sa; Su";

    auto const parsedUnparsed = RoundTrip(rule);
    TEST_EQUAL(parsedUnparsed, rule, ());
  }
  // 00:00-24:00 is the whole day; the port drops the redundant span.
  CheckRoundTrip("Sa-Su 00:00-24:00", "Sa-Su");
}

UNIT_TEST(OpeningHours_TestIsOpen)
{
  using namespace osmoh;

  {
    OpeningHours rules;
    TEST(Parse("2010 Apr 01-30: Mo-Su 17:00-24:00", rules), ());

    TEST(IsOpen(rules, "2010-04-12 19:15"), ());
    TEST(IsClosed(rules, "2010-04-12 14:15"), ());
    TEST(IsClosed(rules, "2011-04-12 20:15"), ());
  }
  {
    OpeningHours rules;
    TEST(Parse("Mo-Th 14:00-22:00; Fr 14:00-16:00;"
               "Sa 00:00-01:00, 14:00-24:00 closed; "
               "Su 00:00-01:00, 14:00-22:00",
               rules),
         ());

    TEST(IsOpen(rules, "2010-05-05 19:15"), ());
    TEST(IsClosed(rules, "2010-05-05 12:15"), ());

    TEST(IsClosed(rules, "2010-04-10 15:15"), ());  // Saturday
    /// If no selectors with `open' modifier match than state is closed.
    TEST(IsClosed(rules, "2010-04-10 11:15"), ());

    TEST(IsOpen(rules, "2010-04-11 14:15"), ());
    TEST(IsClosed(rules, "2010-04-11 23:45"), ());
  }
  {
    OpeningHours rules;
    TEST(Parse("Mo-Tu 15:00-18:00; We off; "
               "Th-Fr 15:00-18:00; Sa 10:00-12:00",
               rules),
         ());

    TEST(IsClosed(rules, "2015-11-04 16:00"), ());
    TEST(IsOpen(rules, "2015-11-02 16:00"), ());
  }
  {
    OpeningHours rules;
    TEST(Parse("Su 11:00-17:00; \"Wochentags auf Anfrage\"", rules), ());

    TEST(IsOpen(rules, "2015-11-08 12:30"), ());
    TEST(IsUnknown(rules, "2015-11-09 12:30"), ());
  }
  {
    OpeningHours rules;
    TEST(Parse("PH open", rules), ());
    TEST(Parse("PH closed", rules), ());
    TEST(Parse("PH on", rules), ());
    TEST(Parse("PH off", rules), ());

    /// @todo Single PH entries are not supported yet, always closed?!
    TEST(IsClosed(rules, "2021-05-07 11:23"), ());  // Friday
    TEST(IsClosed(rules, "2015-11-08 12:30"), ());  // Sunday
  }
  {
    OpeningHours rules;
    TEST(Parse("Mo-Sa 08:00-20:00; Dec Mo-Sa 08:00-14:00; Dec 25 off", rules), ());

    TEST(IsClosed(rules, "2020-12-25 11:11"), ());

    TEST(IsOpen(rules, "2020-12-24 13:50"), ());    // Thursday
    TEST(IsClosed(rules, "2020-12-24 14:10"), ());  // Thursday
    TEST(IsClosed(rules, "2020-12-27 12:00"), ());  // Sunday

    TEST(IsOpen(rules, "2021-05-07 13:50"), ());    // Friday
    TEST(IsOpen(rules, "2021-05-08 19:40"), ());    // Saturdaya
    TEST(IsClosed(rules, "2021-05-09 12:00"), ());  // Sunday
  }

  /// @todo Synthetic example with ill-formed OH, but documented behaviour.
  /// @see rules_evaluation.cpp/IsR1IncludesR2
  /*
  {
    OpeningHours rules;
    TEST(Parse("Mo-Sa 08:00-20:00; Fr 08:00-14:00", rules), ());

    TEST(IsOpen(rules, "2021-05-07 13:50"), ());     // Friday
    TEST(IsClosed(rules, "2021-05-07 14:10"), ());   // Friday
  }
  */

  {
    OpeningHours rules;
    TEST(Parse("Mo-Sa 08:00-20:00; Dec 24 Mo-Sa 08:00-14:00; PH off", rules), ());

    TEST(IsClosed(rules, "2020-12-24 14:10"), ());  // Thursday

    TEST(IsOpen(rules, "2021-05-07 11:12"), ());    // Friday
    TEST(IsOpen(rules, "2021-05-08 13:14"), ());    // Saturday
    TEST(IsClosed(rules, "2021-05-09 15:16"), ());  // Sunday
  }
  {
    OpeningHours rules;
    TEST(Parse("Apr 01-Sep 30 11:00-15:00, "
               "Mo off, Fr off; "
               "week 27-32 11:00-17:00",
               rules),
         ());

    TEST(IsClosed(rules, "2015-11-9 12:20"), ());
    TEST(IsClosed(rules, "2015-11-13 12:20"), ());

    TEST(IsOpen(rules, "2015-04-08 12:20"), ());
    TEST(IsOpen(rules, "2015-09-15 12:20"), ());

    /// week 28th of 2015, Tu
    TEST(IsOpen(rules, "2015-07-09 16:50"), ());
    TEST(IsClosed(rules, "2015-08-14 12:00"), ());
  }
  {
    OpeningHours rules;
    TEST(Parse("06:13-15:00; 16:30+", rules), ());

    // The ';' operator is overriding, so the second all-day rule replaces the
    // first. A union requires ','.
    TEST(IsClosed(rules, "2013-12-12 7:00"), ());
    TEST(IsOpen(rules, "2013-12-12 20:00"), ());
    TEST(IsClosed(rules, "2013-12-12 16:00"), ());
  }
  {
    OpeningHours rules;
    TEST(Parse("10:00-12:00", rules), ());

    TEST(IsOpen(rules, "2013-12-12 10:01"), ());
    TEST(IsClosed(rules, "2013-12-12 12:01"), ());
  }
  {
    OpeningHours rules;
    TEST(Parse("24/7; Mo 15:00-16:00 off", rules), ());

    TEST(IsOpen(rules, "2012-10-08 00:01"), ());
    TEST(IsClosed(rules, "2012-10-08 15:59"), ());
  }
  {
    OpeningHours rules;
    TEST(Parse("Mo-Su 12:00-23:00", rules), ());

    TEST(IsOpen(rules, "2015-11-06 18:40"), ());
    TEST(!IsClosed(rules, "2015-11-06 18:40"), ());
  }
  {
    OpeningHours rules;
    TEST(Parse("2015 Apr 01-30: Mo-Fr 17:00-08:00", rules), ());

    TEST(IsOpen(rules, "2015-04-10 07:15"), ());
    TEST(IsOpen(rules, "2015-05-01 07:15"), ());
    TEST(IsOpen(rules, "2015-04-11 07:15"), ());
    TEST(IsClosed(rules, "2015-04-12 14:15"), ());
    TEST(IsClosed(rules, "2016-04-12 20:15"), ());
  }
  {
    OpeningHours rules;
    TEST(Parse("Mo-Th 15:00+; Fr-Su 13:00+", rules), ());

    TEST(!IsOpen(rules, "2016-06-06 13:14"), ());
    TEST(IsOpen(rules, "2016-06-06 17:06"), ());
    TEST(IsOpen(rules, "2016-06-05 13:06"), ());
    TEST(IsOpen(rules, "2016-05-31 18:28"), ());
  }
  {
    OpeningHours rules;
    TEST(Parse("Mo-We 00:00-24:00", rules), ());

    TEST(IsOpen(rules, "2016-10-03 05:35"), ());
    TEST(IsOpen(rules, "2017-01-17 15:35"), ());
    TEST(IsOpen(rules, "2017-05-31 23:35"), ());
    TEST(!IsOpen(rules, "2017-02-10 05:35"), ());
    TEST(!IsOpen(rules, "2017-05-21 06:01"), ());
  }
  {
    OpeningHours rules;
    TEST(Parse("Mo-Su 00:00-24:00; Mo-We 00:00-24:00 off", rules), ());

    TEST(!IsOpen(rules, "2016-10-03 05:35"), ());
    TEST(!IsOpen(rules, "2017-01-17 15:35"), ());
    TEST(!IsOpen(rules, "2017-05-31 23:35"), ());
    TEST(IsOpen(rules, "2017-02-10 05:35"), ());
    TEST(IsOpen(rules, "2017-05-21 06:01"), ());
  }
}

UNIT_TEST(OpeningHours_GetNextTimeOpen)
{
  using namespace osmoh;

  OpeningHours rules;

  char constexpr fmt[] = "%Y-%m-%d %H:%M";

  TEST(Parse("Mo-Tu 15:00-18:00; We off; Th on; Fr 15:00-18:00; Sa 10:00-12:00", rules), ());
  TEST(GetNextTimeOpen(rules, fmt, "2022-01-03 09:00") == "2022-01-03 15:00", ());    // Mo
  TEST(GetNextTimeClosed(rules, fmt, "2022-01-03 16:00") == "2022-01-03 18:00", ());  // Mo
  TEST(GetNextTimeOpen(rules, fmt, "2022-01-04 09:00") == "2022-01-04 15:00", ());    // Tu
  TEST(GetNextTimeClosed(rules, fmt, "2022-01-04 16:00") == "2022-01-04 18:00", ());  // Tu
  TEST(GetNextTimeOpen(rules, fmt, "2022-01-05 09:00") == "2022-01-06 00:00", ());    // We
  TEST(GetNextTimeClosed(rules, fmt, "2022-01-06 16:00") == "2022-01-07 00:00", ());  // Th
  TEST(GetNextTimeOpen(rules, fmt, "2022-01-07 09:00") == "2022-01-07 15:00", ());    // Fr
  TEST(GetNextTimeClosed(rules, fmt, "2022-01-07 16:00") == "2022-01-07 18:00", ());  // Fr
  TEST(GetNextTimeOpen(rules, fmt, "2022-01-08 09:00") == "2022-01-08 10:00", ());    // Sa
  TEST(GetNextTimeClosed(rules, fmt, "2022-01-08 11:00") == "2022-01-08 12:00", ());  // Sa
  TEST(GetNextTimeOpen(rules, fmt, "2022-01-09 09:00") == "2022-01-10 15:00", ());    // Su

  TEST(Parse("Mo-Fr 09:00-12:00, 13:00-20:00; We 10:00-11:00 off; Fr off", rules), ());
  TEST(GetNextTimeOpen(rules, fmt, "2022-01-03 07:00") == "2022-01-03 09:00", ());    // Mo morning
  TEST(GetNextTimeClosed(rules, fmt, "2022-01-03 10:00") == "2022-01-03 12:00", ());  // Mo morning
  TEST(GetNextTimeOpen(rules, fmt, "2022-01-03 12:30") == "2022-01-03 13:00", ());    // Mo afternoon
  TEST(GetNextTimeClosed(rules, fmt, "2022-01-03 13:30") == "2022-01-03 20:00", ());  // Mo afternoon
  TEST(GetNextTimeOpen(rules, fmt, "2022-01-03 21:00") == "2022-01-04 09:00", ());    // Mo night
  TEST(GetNextTimeClosed(rules, fmt, "2022-01-05 09:30") == "2022-01-05 10:00", ());  // We off
  TEST(GetNextTimeOpen(rules, fmt, "2022-01-05 10:30") == "2022-01-05 11:00", ());    // We off
  TEST(GetNextTimeOpen(rules, fmt, "2022-01-07 07:30") == "2022-01-10 09:00", ());    // Fr off

  TEST(Parse("Mo-Sa 08:00-20:00; Feb Mo-Sa 09:00-14:00; Jan 06 off", rules), ());
  TEST(GetNextTimeOpen(rules, fmt, "2022-01-04 07:00") == "2022-01-04 08:00", ());    // Tu Jan
  TEST(GetNextTimeClosed(rules, fmt, "2022-01-04 09:00") == "2022-01-04 20:00", ());  // Tu Jan
  TEST(GetNextTimeOpen(rules, fmt, "2022-02-08 07:00") == "2022-02-08 09:00", ());    // Tu Feb
  TEST(GetNextTimeClosed(rules, fmt, "2022-02-08 09:00") == "2022-02-08 14:00", ());  // Tu Feb
  TEST(GetNextTimeOpen(rules, fmt, "2020-01-06 07:00") == "2020-01-07 08:00", ());    // Jan 06

  TEST(Parse("24/7; Mo 15:00-16:00 off", rules), ());
  TEST(GetNextTimeOpen(rules, fmt, "2022-01-03 15:30") == "2022-01-03 16:00", ());    // Mo
  TEST(GetNextTimeClosed(rules, fmt, "2022-01-01 15:30") == "2022-01-03 15:00", ());  // Sa

  TEST(Parse("Mo-Th 15:00+; Fr-Su 13:00+", rules), ());
  TEST(GetNextTimeOpen(rules, fmt, "2022-01-03 07:30") == "2022-01-03 15:00", ());    // Mo
  TEST(GetNextTimeClosed(rules, fmt, "2022-01-03 15:30") == "2022-01-04 00:00", ());  // Mo
  TEST(GetNextTimeOpen(rules, fmt, "2022-01-08 07:30") == "2022-01-08 13:00", ());    // Sa
  TEST(GetNextTimeClosed(rules, fmt, "2022-01-08 15:30") == "2022-01-09 00:00", ());  // Sa

  TEST(Parse("Mo-Su 00:00-24:00; Mo-We 00:00-24:00 off", rules), ());
  TEST(GetNextTimeOpen(rules, fmt, "2022-01-03 15:30") == "2022-01-06 00:00", ());    // Mo
  TEST(GetNextTimeClosed(rules, fmt, "2022-01-01 15:30") == "2022-01-03 00:00", ());  // Sa
}

UNIT_TEST(OpeningHours_TestOpeningHours)
{
  // OpeningHours is just a wrapper. So a couple of tests is
  // enough to check if it works.

  using namespace osmoh;

  {
    OpeningHours oh("Su 11:00-17:00; \"Wochentags auf Anfrage\"; Tu off");
    TEST(oh.IsValid(), ());

    TEST(IsOpen(oh, "2015-11-08 12:30"), ());
    TEST(IsUnknown(oh, "2015-11-09 12:30"), ());
    TEST(IsClosed(oh, "2015-11-10 12:30"), ());
  }
  {
    OpeningHours oh("Nov +1");
    TEST(!oh.IsValid(), ());
  }
  {
    OpeningHours oh("Mo-Th 15:00+; Fr-Su 13:00+");
    TEST(oh.IsValid(), ());

    TEST(IsOpen(oh, "2016-05-31 18:28"), ());
    TEST(IsOpen(oh, "2016-05-31 22:28"), ());
    TEST(IsClosed(oh, "2016-05-31 10:30"), ());
  }
  {
    /// @todo Make valid years range (construction). https://www.openstreetmap.org/way/273985562
    OpeningHours oh("2025-08-04 - 2026-12-13");
    TEST(!oh.IsValid(), ());

    // std::tm time = {};
    // TEST(GetTimeTuple("2026-02-20 12:00", fmt, time), ());
    // TEST(oh.IsOpen(mktime(&time)), ());

    // TEST(GetTimeTuple("2027-02-20 12:00", fmt, time), ());
    // TEST(!oh.IsOpen(mktime(&time)), ());
  }
}

UNIT_TEST(OpeningHours_WeekdayInMonthDates)
{
  using namespace osmoh;

  // A weekday-in-month date names a single day ("the first Monday of
  // February"), so it must not degrade to its whole month in GetRule() --
  // routing serialization reads that.
  OpeningHours const oh("Feb Mo[1] +2 days 10:00-12:00");
  TEST(oh.IsValid(), ());
  TEST_EQUAL(ToString(oh), "Feb Mo[1] +2 days 10:00-12:00", ());
  // First Monday of Feb 2026 is Feb 2; +2 days = Wednesday Feb 4.
  TEST(IsOpen(oh, "2026-02-04 11:00"), ());
  TEST(IsClosed(oh, "2026-02-11 11:00"), ());
  TEST(IsClosed(oh, "2026-02-04 13:00"), ());
}

UNIT_TEST(OpeningHours_WeekdayInMonthDatesAmongOtherRules)
{
  using namespace osmoh;

  // Real corpus values; the DST-changeover idiom is the common form. Each has
  // several rules, so a partial conversion would also misplace the separators.
  CheckRoundTrip("Mo-Fr 08:00-18:00; Mar Su[-1] 02:00-04:00, Sa 10:00-12:00");
  CheckRoundTrip(
      "Mar Su[-1]-Oct Su[-1] 09:30-12:30,16:00-19:00; "
      "Oct Su[-1]-Mar Su[-1] 09:30-12:30,15:00-18:00",
      "Mar Su[-1]-Oct Su[-1] 09:30-12:30, 16:00-19:00; "
      "Oct Su[-1]-Mar Su[-1] 09:30-12:30, 15:00-18:00");

  OpeningHours const oh("Mo-Fr 08:00-18:00; Mar Su[-1] 02:00-04:00, Sa 10:00-12:00");
  TEST(IsOpen(oh, "2026-07-06 09:00"), ());
  TEST(IsClosed(oh, "2026-07-06 19:00"), ());
}

UNIT_TEST(OpeningHours_AdditiveRuleKeepsItsOwnTimespan)
{
  using namespace osmoh;

  // A rule whose all-day span is implied must spell it out when an additive
  // rule follows, otherwise the printed form re-parses as one weekday list.
  CheckRoundTrip("Mo 06:00-24:00, Tu-Fr 00:00-24:00, Sa 00:00-22:00");
  TEST(IsOpen(OpeningHours("Mo 06:00-24:00, Tu-Fr 00:00-24:00, Sa 00:00-22:00"), "2026-07-09 23:00"), ());

  // Without a following additive rule the span stays implied.
  CheckRoundTrip("Mo-Fr");
  CheckRoundTrip("Mo-Fr; Sa 10:00-12:00");
}

UNIT_TEST(OpeningHours_PrinterKeepsParsedSelectors)
{
  using namespace osmoh;

  // Selectors the parser accepts must survive printing: conversion reports
  // these rules as complete, so a printer dropping fields would silently
  // misrepresent the value ("PH +1 day off" became plain "PH off").
  CheckRoundTrip("PH +1 day closed");
  CheckRoundTrip("Mo-Fr[1] 10:00-16:00");
  CheckRoundTrip("Jan 01 +Su 10:00-18:00");
}

UNIT_TEST(OpeningHours_MultipleCommentsJoined)
{
  using namespace osmoh;

  // Evaluation reports both comments joined; GetRule() must not keep just
  // the first one.
  OpeningHours const oh("\"winter\": Mo-Fr 10:00-16:00 unknown \"call first\"");
  TEST(oh.IsValid(), ());
  TEST_EQUAL(osmoh::ToString(oh), "Mo-Fr 10:00-16:00 \"call first, winter\"", ());
  TEST(IsUnknown(oh, "2026-07-06 12:00"), ());
}

UNIT_TEST(OpeningHours_AdditiveRuleWithoutDaysGetsWeekdays)
{
  using namespace osmoh;

  // The mirror case: an additive rule with times but no day selector must
  // print an explicit "Mo-Su", otherwise its spans re-parse as extra
  // timespans of the previous rule and Saturday evening flips to closed.
  CheckRoundTrip("Mo-Fr 08:00-18:00 open, 19:00-21:00", "Mo-Fr 08:00-18:00, Mo-Su 19:00-21:00");
  TEST(IsOpen(OpeningHours("Mo-Fr 08:00-18:00 open, 19:00-21:00"), "2026-07-11 19:30" /* Saturday */), ());
  CheckRoundTrip("Mo-Fr open, 19:00-21:00", "Mo-Fr 00:00-24:00, Mo-Su 19:00-21:00");
}

UNIT_TEST(OpeningHours_ExplicitOpenSurvivesAComment)
{
  using namespace osmoh;

  // "open" is the default modifier and is normally not printed, but dropping
  // it next to a comment leaves a bare comment, which means unknown.
  CheckRoundTrip("Mo-Fr 08:00-16:00 open \"note\"");
  TEST(IsOpen(OpeningHours("Mo-Fr 08:00-16:00 open \"note\""), "2026-07-09 10:00"), ());
  CheckRoundTrip("Mo-Fr open \"school hours\"; SH,PH off", "Mo-Fr open \"school hours\"; PH, SH closed");
}

UNIT_TEST(OpeningHours_EventTimesCompareByEventNotByClock)
{
  using namespace osmoh;

  // Time::GetHours()/GetMinutes() return the same placeholder for every event,
  // so comparing those made all sun events equal to each other and to 00:00.
  Time const dawn{TimeEvent{TimeEvent::Event::Dawn}};
  Time const sunrise{TimeEvent{TimeEvent::Event::Sunrise}};
  Time const sunset{TimeEvent{TimeEvent::Event::Sunset}};
  TimeEvent shifted{TimeEvent::Event::Sunrise};
  shifted.SetOffset(HourMinutes(60_min));

  TEST(!(dawn == sunrise), ());
  TEST(!(sunrise == sunset), ());
  TEST(!(sunrise == Time{shifted}), ());
  TEST(sunrise == Time{TimeEvent{TimeEvent::Event::Sunrise}}, ());
  TEST(!(sunrise == Time{HourMinutes(0_h)}), ());

  TEST(!(OpeningHours("Mo-Su dawn-dusk").GetRule()[0].GetTimes()[0] ==
         OpeningHours("Mo-Su sunrise-sunset").GetRule()[0].GetTimes()[0]),
       ());
}

UNIT_TEST(OpeningHours_DawnDuskAreNotSunriseSunset)
{
  using namespace osmoh;

  // Dawn and dusk are civil twilight, about an hour either side of
  // sunrise/sunset. They must remain distinct in GetRule().
  // The printer's canonical form adds a space after the timespan comma.
  for (auto const & [value, printed] :
       {std::pair{"Mo-Su dawn-dusk", "Mo-Su dawn-dusk"}, std::pair{"08:00-dusk", "08:00-dusk"},
        std::pair{"Mo-Fr dusk-00:00,04:00-dawn", "Mo-Fr dusk-00:00, 04:00-dawn"}})
  {
    OpeningHours const oh(value);
    TEST(oh.IsValid(), (value));
    TEST_EQUAL(ToString(oh), printed, (value));
  }

  OpeningHours const sun("Mo-Su sunrise-sunset");
  TEST(sun.IsValid(), ());
  TEST_EQUAL(ToString(sun), "Mo-Su sunrise-sunset", ());
}
UNIT_TEST(OpeningHours_GetInfoHorizonAndDst)
{
  using namespace osmoh;

  {
    // A schedule pinned to a future year must not report "never opens" just
    // because it lies beyond the default seasonal scan window.
    OpeningHours const oh("2028 Jan 01 10:00-11:00");
    TEST(oh.IsValid(), ());
    auto const info = oh.GetInfo(MakeTimestamp("2026-07-30 12:00"));
    TEST_EQUAL(info.state, RuleState::Closed, ());
    TEST_EQUAL(FormatTime(info.nextTimeOpen, "%Y-%m-%d %H:%M"), "2028-01-01 10:00", ());
  }
  {
    // Crossing a DST transition must not shift the reported opening time:
    // Europe/Berlin springs forward on 2026-03-29 02:00 -> 03:00.
    char const * const oldTz = getenv("TZ");
    setenv("TZ", "Europe/Berlin", 1);
    tzset();
    {
      OpeningHours const oh("Mo-Su 10:00-18:00");
      auto const info = oh.GetInfo(MakeTimestamp("2026-03-28 23:00"));
      TEST_EQUAL(FormatTime(info.nextTimeOpen, "%Y-%m-%d %H:%M"), "2026-03-29 10:00", ());
    }
    {
      // An opening boundary inside the gap snaps to the first valid instant:
      // nonexistent 02:30 becomes 03:00, exactly where IsOpen() flips.
      OpeningHours const oh("Su 02:30-04:00");
      auto const info = oh.GetInfo(MakeTimestamp("2026-03-28 12:00"));
      TEST_EQUAL(FormatTime(info.nextTimeOpen, "%Y-%m-%d %H:%M"), "2026-03-29 03:00", ());
      TEST(oh.IsOpen(info.nextTimeOpen), ());
      TEST(!oh.IsOpen(info.nextTimeOpen - 60), ());
    }
    if (oldTz)
      setenv("TZ", oldTz, 1);
    else
      unsetenv("TZ");
    tzset();
  }
}

UNIT_TEST(OpeningHours_RulesCtorRoundTrip)
{
  using namespace osmoh;

  // Rebuilding from rules (the editor path) must preserve semantics for month
  // endpoints without a day and for open-ended spans.
  {
    // "Jan 05 - Feb": the day-less end month means through the end of
    // February: a day-less end month means through the end of that month.
    MonthDay from;
    from.SetMonth(MonthDay::Month::Jan);
    from.SetDayNum(5);
    MonthDay to;
    to.SetMonth(MonthDay::Month::Feb);
    MonthdayRange range;
    range.SetStart(from);
    range.SetEnd(to);
    Timespan span;
    span.SetStart(HourMinutes(8_h));
    span.SetEnd(HourMinutes(10_h));
    RuleSequence rule;
    rule.SetMonths({range});
    rule.SetTimes({span});

    OpeningHours const rebuilt(TRuleSequences{rule});
    TEST(rebuilt.IsValid(), ());
    TEST(IsOpen(rebuilt, "2026-02-20 8:30"), ());
    TEST(IsOpen(rebuilt, "2026-01-10 8:30"), ());
    TEST(IsClosed(rebuilt, "2026-03-02 8:30"), ());
  }
  {
    OpeningHours const source("10:00+");
    TEST(source.IsValid(), ());
    OpeningHours const rebuilt(source.GetRule());
    // An open-ended span has no explicit end in the canonical form.
    TEST_EQUAL(osmoh::ToString(rebuilt), "10:00+", ());
    TEST(IsOpen(rebuilt, "2026-07-06 15:00"), ());
  }
}

UNIT_TEST(OpeningHours_NightSpans)
{
  using namespace osmoh;

  // A span wrapping past midnight is not linearly mergeable with an adjacent
  // daytime span.
  {
    OpeningHours const oh("Fr 12:00-18:00,18:00-02:00");
    TEST(oh.IsValid(), ());
    TEST_EQUAL(osmoh::ToString(oh), "Fr 12:00-18:00, 18:00-02:00", ());
    TEST(IsOpen(oh, "2026-07-03 13:00"), ());  // Friday afternoon
    TEST(IsOpen(oh, "2026-07-03 20:00"), ());  // Friday evening
    TEST(IsOpen(oh, "2026-07-04 1:00"), ());   // Saturday night spill
    TEST(IsClosed(oh, "2026-07-04 3:00"), ());
  }
  // Fixed spans sort among themselves; sun-event spans keep their position.
  CheckRoundTrip("sunset-05:00,08:00-10:00,06:00-07:00", "sunset-05:00, 06:00-07:00, 08:00-10:00");
}

UNIT_TEST(OpeningHours_YearMonthRange)
{
  using namespace osmoh;

  // A year-prefixed month range covers the WHOLE end month; the upstream
  // reference evaluator stopped at the 1st of the end month.
  {
    OpeningHours const oh("2026 Apr-May 08:00-10:00");
    TEST(oh.IsValid(), ());
    TEST(IsOpen(oh, "2026-04-05 08:30"), ());
    TEST(IsOpen(oh, "2026-05-20 08:30"), ());  // deep inside the end month
    TEST(IsClosed(oh, "2026-06-02 08:30"), ());
    TEST(IsClosed(oh, "2027-04-05 08:30"), ());
  }
  {
    OpeningHours const oh("2021 Jan 10:00-12:00");
    TEST(oh.IsValid(), ());
    TEST(IsOpen(oh, "2021-01-05 11:00"), ());
    TEST(IsOpen(oh, "2021-01-31 11:00"), ());
    TEST(IsClosed(oh, "2021-02-05 11:00"), ());
  }
  {
    // Wrapping through the year end.
    OpeningHours const oh("2020 Nov-Feb 08:00-10:00");
    TEST(oh.IsValid(), ());
    TEST(IsOpen(oh, "2020-12-20 08:30"), ());
    TEST(IsOpen(oh, "2021-02-20 08:30"), ());
    TEST(IsClosed(oh, "2021-03-02 08:30"), ());
  }
}

UNIT_TEST(OpeningHours_CommentRules)
{
  using namespace osmoh;

  // A comment-only modifier means "unknown, see the comment" however the rule
  // is attached; it never blankets the other rules with open-24/7.
  {
    OpeningHours const oh("Su 10:00-16:00 || \"on request\"");
    TEST(oh.IsValid(), ());
    TEST_EQUAL(osmoh::ToString(oh), "Su 10:00-16:00 || \"on request\"", ());
    TEST(IsUnknown(oh, "2026-07-06 12:00"), ());  // Monday
    TEST(IsOpen(oh, "2026-07-05 12:00"), ());     // Sunday
  }
  {
    // The additive bare comment evaluates as a fallback and prints in the
    // explicit "||" form.
    OpeningHours const oh("Mo-Fr 10:00-18:00, \"by arrangement\"");
    TEST(oh.IsValid(), ());
    TEST_EQUAL(osmoh::ToString(oh), "Mo-Fr 10:00-18:00 || \"by arrangement\"", ());
    TEST(IsUnknown(oh, "2026-07-04 12:00"), ());  // Saturday
    TEST(IsOpen(oh, "2026-07-06 12:00"), ());     // Monday
    TEST(IsClosed(oh, "2026-07-06 9:00"), ());    // Monday, before opening
  }
  {
    // A selector'd comment-only rule is unknown on its days, not open.
    OpeningHours const oh("Mo-Fr \"call us\"");
    TEST(oh.IsValid(), ());
    TEST_EQUAL(osmoh::ToString(oh), "Mo-Fr \"call us\"", ());
    TEST(IsUnknown(oh, "2026-07-06 12:00"), ());  // Monday
    TEST(IsClosed(oh, "2026-07-04 12:00"), ());   // Saturday
  }
  {
    OpeningHours const oh("Mo-Fr 10:00-18:00 \"ring bell\"");
    TEST(oh.IsValid(), ());
    TEST_EQUAL(osmoh::ToString(oh), "Mo-Fr 10:00-18:00 \"ring bell\"", ());
    TEST(IsUnknown(oh, "2026-07-06 12:00"), ());  // Monday, within the hours
    TEST(IsClosed(oh, "2026-07-06 9:00"), ());
  }
}

}  // namespace osmoh_tests
