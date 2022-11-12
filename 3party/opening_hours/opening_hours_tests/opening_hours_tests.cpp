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

#include "parse_opening_hours.hpp"
#include "rules_evaluation.hpp"
#include "rules_evaluation_private.hpp"

#include <ctime>
#include <iostream>
#include <locale>
#include <sstream>

#define BOOST_TEST_MODULE OpeningHours

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wshorten-64-to-32"
#include <boost/test/included/unit_test.hpp>
#pragma clang diagnostic pop
#else
#include <boost/test/included/unit_test.hpp>
#endif

#include <boost/spirit/include/qi.hpp>

// Visual Studio doesn't have strptime
#ifdef _MSC_VER
#include <time.h>
#include <iomanip>
#include <sstream>
extern "C" char* strptime(const char* s, const char* f, struct tm* tm) {
  // Isn't the C++ standard lib nice? std::get_time is defined such that its
  // format parameters are the exact same as strptime. Of course, we have to
  // create a string stream first, and imbue it with the current C locale, and
  // we also have to make sure we return the right things if it fails, or
  // if it succeeds, but this is still far simpler an implementation than any
  // of the versions in any of the C standard libraries.
  std::istringstream input(s);
  input.imbue(std::locale(setlocale(LC_ALL, nullptr)));
  input >> std::get_time(tm, f);
  if (input.fail())
    return nullptr;
  return (char*)(s + input.tellg());
}
#endif


namespace
{
template <typename T>
std::string ToString(T const & t)
{
  std::stringstream sstr;
  sstr << t;
  return sstr.str();
}

template <typename Parser>
bool Test(std::string const & str, Parser const & p, bool full_match = true)
{
  // We don't care about the result of the "what" function.
  // We only care that all parsers have it:
  boost::spirit::qi::what(p);

  auto first = begin(str);
  auto last = end(str);
  return boost::spirit::qi::parse(first, last, p) && (!full_match || (first == last));
}

template <typename ParseResult>
std::string ParseAndUnparse(std::string const & str)
{
  ParseResult parseResult;
  if (!osmoh::Parse(str, parseResult))
    return ":CAN'T PARSE:";

  std::stringstream sstr;
  sstr << parseResult;

  return sstr.str();
}

bool GetTimeTuple(std::string const & strTime, std::string const & fmt, std::tm & tm)
{
  auto const rc = strptime(strTime.data(), fmt.data(), &tm);
  return rc != nullptr;
}

struct GetTimeError: std::exception
{
  GetTimeError(std::string const & message): m_message(message) {}
  char const * what() const noexcept override
  {
    return m_message.data();
  }
  std::string const m_message;
};

osmoh::RuleState GetRulesState(osmoh::TRuleSequences const & rules, std::string const & dateTime)
{
  static auto const & fmt = "%Y-%m-%d %H:%M";
  std::tm time = {};
  /// Parsing the format such as "%Y-%m-%d %H:%M" doesn't
  /// fill tm_wday field. It will be filled after time_t to tm convertion.
  if (!GetTimeTuple(dateTime, fmt, time))
    throw GetTimeError{"Can't parse " + dateTime + " against " + fmt};

  return osmoh::GetState(rules, mktime(&time));
}

bool IsOpen(osmoh::TRuleSequences const & rules, std::string const & dateTime)
{
  return GetRulesState(rules, dateTime) == osmoh::RuleState::Open;
}

std::string GetNextTimeOpen(osmoh::TRuleSequences const & rules, char const * fmt, std::string const & dateTime)
{
  std::tm time = {};
  BOOST_CHECK(GetTimeTuple(dateTime, fmt, time));

  time_t openingTime = osmoh::GetNextTimeOpen(rules, mktime(&time));
  tm openingTime_tm = *localtime(&openingTime);
  char buffer[30];
  std::strftime(buffer, sizeof(buffer)/sizeof(buffer[0]), fmt, &openingTime_tm);

  return std::string(buffer);
}

bool IsClosed(osmoh::TRuleSequences const & rules, std::string const & dateTime)
{
  return GetRulesState(rules, dateTime) == osmoh::RuleState::Closed;
}

std::string GetNextTimeClosed(osmoh::TRuleSequences const & rules, char const * fmt, std::string const & dateTime)
{
  std::tm time = {};
  BOOST_CHECK(GetTimeTuple(dateTime, fmt, time));

  time_t openingTime = osmoh::GetNextTimeClosed(rules, mktime(&time));
  tm openingTime_tm = *localtime(&openingTime);
  char buffer[30];
  std::strftime(buffer, sizeof(buffer)/sizeof(buffer[0]), fmt, &openingTime_tm);

  return std::string(buffer);
}

bool IsUnknown(osmoh::TRuleSequences const & rules, std::string const & dateTime)
{
  return GetRulesState(rules, dateTime) == osmoh::RuleState::Unknown;
}

bool IsActive(osmoh::RuleSequence const & rule, std::tm tm)
{
  return IsActive(rule, mktime(&tm));
}
} // namespace

BOOST_AUTO_TEST_CASE(OpeningHours_TestHourMinutes)
{
  using namespace osmoh;

  {
    BOOST_CHECK(HourMinutes().IsEmpty());
    BOOST_CHECK_EQUAL(ToString(HourMinutes()), "hh:mm");
  }
  {
    HourMinutes hm(10_min);
    BOOST_CHECK(!hm.IsEmpty());
    BOOST_CHECK_EQUAL(ToString(hm), "00:10");
  }
  {
    HourMinutes hm(100_min);
    BOOST_CHECK(!hm.IsEmpty());
    BOOST_CHECK_EQUAL(hm.GetHoursCount(), 1);
    BOOST_CHECK_EQUAL(hm.GetMinutesCount(), 40);

    BOOST_CHECK_EQUAL(ToString(hm), "01:40");
  }
  {
    HourMinutes hm;
    hm.SetHours(22_h);
    hm.SetMinutes(15_min);
    BOOST_CHECK(!hm.IsEmpty());
    BOOST_CHECK(!hm.IsExtended());

    BOOST_CHECK_EQUAL(hm.GetHoursCount(), 22);
    BOOST_CHECK_EQUAL(hm.GetMinutesCount(), 15);

    BOOST_CHECK_EQUAL(ToString(hm), "22:15");
  }
  {
    HourMinutes hm;
    hm.SetHours(39_h);
    hm.SetMinutes(15_min);
    BOOST_CHECK(!hm.IsEmpty());
    BOOST_CHECK(hm.IsExtended());

    BOOST_CHECK_EQUAL(hm.GetHoursCount(), 39);
    BOOST_CHECK_EQUAL(hm.GetMinutesCount(), 15);

    BOOST_CHECK_EQUAL(ToString(hm), "39:15");
  }
}

BOOST_AUTO_TEST_CASE(OpeningHours_TestTimeEvent)
{
  using namespace osmoh;

  {
    BOOST_CHECK(TimeEvent().IsEmpty());
    BOOST_CHECK(!TimeEvent().HasOffset());
  }
  {
    TimeEvent te(TimeEvent::Event::Sunrise);
    BOOST_CHECK(!te.IsEmpty());
    BOOST_CHECK(!te.HasOffset());

    BOOST_CHECK_EQUAL(ToString(te), "sunrise");
  }
  {
    TimeEvent te(TimeEvent::Event::Sunset);
    BOOST_CHECK(!te.IsEmpty());
    BOOST_CHECK(!te.HasOffset());

    BOOST_CHECK_EQUAL(ToString(te), "sunset");
  }
  {
    TimeEvent te(TimeEvent::Event::Sunrise);
    te.SetOffset(-HourMinutes(100_min));
    BOOST_CHECK(!te.IsEmpty());
    BOOST_CHECK(te.HasOffset());

    BOOST_CHECK_EQUAL(ToString(te), "(sunrise-01:40)");

    te.SetOffset(HourMinutes(100_min));
    BOOST_CHECK_EQUAL(ToString(te), "(sunrise+01:40)");
  }
}

BOOST_AUTO_TEST_CASE(OpeningHours_TestTime)
{
  using namespace osmoh;

  {
    BOOST_CHECK(Time().IsEmpty());
    BOOST_CHECK(!Time().IsHoursMinutes());
    BOOST_CHECK(!Time().IsTime());
    BOOST_CHECK(!Time().IsEvent());
  }
  {
    Time time;
    time.SetEvent(TimeEvent::Event::Sunrise);
    BOOST_CHECK(!time.IsEmpty());
    BOOST_CHECK(!time.IsHoursMinutes());
    BOOST_CHECK(time.IsTime());
    BOOST_CHECK(time.IsEvent());

    BOOST_CHECK_EQUAL(ToString(time), "sunrise");

    time.AddDuration(-90_min);
    BOOST_CHECK_EQUAL(ToString(time), "(sunrise-01:30)");
  }
  {
    Time time{};
    time.SetHourMinutes(HourMinutes(22_h + 5_min));
    BOOST_CHECK(!time.IsEmpty());
    BOOST_CHECK(time.IsHoursMinutes());
    BOOST_CHECK(time.IsTime());
    BOOST_CHECK(!time.IsEvent());

    BOOST_CHECK_EQUAL(ToString(time), "22:05");

    time.AddDuration(10_min);
    BOOST_CHECK_EQUAL(ToString(time), "22:15");
  }
  {
    Time time{};
    time.SetHourMinutes(HourMinutes(22_h + 5_min));

    time.SetEvent(TimeEvent::Event::Sunset);
    BOOST_CHECK_EQUAL(ToString(time), "sunset");
  }
  {
    Time time(HourMinutes(27_h + 30_min));
    BOOST_CHECK_EQUAL(ToString(time), "27:30");
  }
}

BOOST_AUTO_TEST_CASE(OpeningHours_TestTimespan)
{
  using namespace osmoh;

  {
    Timespan span;
    BOOST_CHECK(span.IsEmpty());
    BOOST_CHECK(!span.HasStart());
    BOOST_CHECK(!span.HasEnd());
    BOOST_CHECK(!span.HasExtendedHours());
    BOOST_CHECK_EQUAL(ToString(span), "hh:mm-hh:mm");

    span.SetStart(HourMinutes(10_h));
    BOOST_CHECK(span.HasStart());
    BOOST_CHECK(span.IsOpen());
    BOOST_CHECK(!span.HasExtendedHours());
    BOOST_CHECK_EQUAL(ToString(span), "10:00");

    span.SetEnd(HourMinutes(12_h));
    BOOST_CHECK(span.HasEnd());
    BOOST_CHECK(!span.IsOpen());
    BOOST_CHECK(!span.HasExtendedHours());
    BOOST_CHECK_EQUAL(ToString(span), "10:00-12:00");

    BOOST_CHECK(!span.HasPeriod());
    span.SetPeriod(10_min);
    BOOST_CHECK(span.HasPeriod());
    BOOST_CHECK_EQUAL(ToString(span), "10:00-12:00/10");
  }
  {
    Timespan span;

    span.SetStart(HourMinutes(10_h));
    span.SetEnd(HourMinutes(47_h));

    BOOST_CHECK(span.HasExtendedHours());
    BOOST_CHECK_EQUAL(ToString(span), "10:00-47:00");
  }
  {
    Timespan span;

    span.SetStart(HourMinutes(10_h));
    span.SetEnd(HourMinutes(06_h));

    BOOST_CHECK(span.HasExtendedHours());
    BOOST_CHECK_EQUAL(ToString(span), "10:00-06:00");
  }
  {
    Timespan span;

    span.SetStart(HourMinutes(10_h));
    span.SetEnd(HourMinutes(00_h));

    BOOST_CHECK(span.HasExtendedHours());
    BOOST_CHECK_EQUAL(ToString(span), "10:00-00:00");
  }

}

BOOST_AUTO_TEST_CASE(OpeningHours_TestNthWeekdayOfTheMonthEntry)
{
  using namespace osmoh;

  {
    NthWeekdayOfTheMonthEntry entry;
    BOOST_CHECK(entry.IsEmpty());
    BOOST_CHECK(!entry.HasStart());
    BOOST_CHECK(!entry.HasEnd());
    BOOST_CHECK_EQUAL(ToString(entry), "");

    entry.SetStart(NthWeekdayOfTheMonthEntry::NthDayOfTheMonth::Third);
    BOOST_CHECK(!entry.IsEmpty());
    BOOST_CHECK(entry.HasStart());
    BOOST_CHECK(!entry.HasEnd());
    BOOST_CHECK_EQUAL(ToString(entry), "3");

    entry.SetEnd(NthWeekdayOfTheMonthEntry::NthDayOfTheMonth::Fifth);
    BOOST_CHECK(!entry.IsEmpty());
    BOOST_CHECK(entry.HasStart());
    BOOST_CHECK(entry.HasEnd());
    BOOST_CHECK_EQUAL(ToString(entry), "3-5");

    entry.SetStart(NthWeekdayOfTheMonthEntry::NthDayOfTheMonth::None);
    BOOST_CHECK(!entry.IsEmpty());
    BOOST_CHECK(!entry.HasStart());
    BOOST_CHECK(entry.HasEnd());
    BOOST_CHECK_EQUAL(ToString(entry), "-5");
  }
}

BOOST_AUTO_TEST_CASE(OpeningHours_TestWeekdayRange)
{
  using namespace osmoh;

  {
    WeekdayRange range;
    BOOST_CHECK(range.IsEmpty());
    BOOST_CHECK(!range.HasEnd());
    BOOST_CHECK(!range.HasSunday());
    BOOST_CHECK(!range.HasWednesday());
    BOOST_CHECK(!range.HasSaturday());
    BOOST_CHECK(!range.HasNth());
  }
  {
    WeekdayRange range;
    BOOST_CHECK(!range.HasNth());

    range.SetStart(Weekday::Tuesday);
    BOOST_CHECK(!range.IsEmpty());
    BOOST_CHECK(!range.HasEnd());
    BOOST_CHECK(!range.HasSunday());
    BOOST_CHECK(!range.HasWednesday());
    BOOST_CHECK(range.HasTuesday());
    BOOST_CHECK(!range.HasSaturday());

    range.SetEnd(Weekday::Saturday);
    BOOST_CHECK(!range.IsEmpty());
    BOOST_CHECK(range.HasStart());
    BOOST_CHECK(range.HasEnd());
    BOOST_CHECK(!range.HasSunday());
    BOOST_CHECK(range.HasWednesday());
    BOOST_CHECK(range.HasTuesday());
    BOOST_CHECK(range.HasSaturday());
  }
  {
    WeekdayRange range;
    BOOST_CHECK(!range.HasNth());

    NthWeekdayOfTheMonthEntry entry;
    entry.SetStart(NthWeekdayOfTheMonthEntry::NthDayOfTheMonth::First);
    range.AddNth(entry);
    BOOST_CHECK(range.HasNth());
  }
  {
    WeekdayRange range;
    range.SetStart(Weekday::Monday);
    range.SetEnd(Weekday::Sunday);

    BOOST_CHECK(range.HasSunday());
    BOOST_CHECK(range.HasMonday());
    BOOST_CHECK(range.HasTuesday());
    BOOST_CHECK(range.HasWednesday());
    BOOST_CHECK(range.HasThursday());
    BOOST_CHECK(range.HasFriday());
    BOOST_CHECK(range.HasSaturday());
  }
}

BOOST_AUTO_TEST_CASE(OpeningHours_Holidays)
{
  using namespace osmoh;

  {
    Holiday h;
    BOOST_CHECK(!h.IsPlural());
    BOOST_CHECK_EQUAL(h.GetOffset(), 0);
    BOOST_CHECK_EQUAL(ToString(h), "SH");

    h.SetOffset(11);

    BOOST_CHECK_EQUAL(h.GetOffset(), 11);
    BOOST_CHECK_EQUAL(ToString(h), "SH +11 days");

    h.SetOffset(-1);
    BOOST_CHECK_EQUAL(ToString(h), "SH -1 day");

    h.SetPlural(true);
    BOOST_CHECK(h.IsPlural());
    BOOST_CHECK_EQUAL(ToString(h), "PH");
  }
}

BOOST_AUTO_TEST_CASE(OpeningHours_Weekdays)
{
  using namespace osmoh;

  {
    Weekdays w;
    BOOST_CHECK(w.IsEmpty());
    BOOST_CHECK(!w.HasWeekday());
    BOOST_CHECK(!w.HasHolidays());

    BOOST_CHECK_EQUAL(ToString(w), "");

    WeekdayRange r;
    r.SetStart(Weekday::Sunday);
    w.AddHoliday(Holiday{});
    w.AddWeekdayRange(r);

    BOOST_CHECK_EQUAL(ToString(w), "SH, Su");
  }
}

BOOST_AUTO_TEST_CASE(OpeningHours_DayOffset)
{
  using namespace osmoh;

  {
    DateOffset offset;
    BOOST_CHECK(offset.IsEmpty());
    BOOST_CHECK(!offset.HasWDayOffset());
    BOOST_CHECK(!offset.HasOffset());
    BOOST_CHECK_EQUAL(ToString(offset), "");

    offset.SetWDayOffset(Weekday::Monday);
    BOOST_CHECK(!offset.IsEmpty());
    BOOST_CHECK(offset.HasWDayOffset());
    BOOST_CHECK_EQUAL(ToString(offset), "+Mo");

    offset.SetOffset(11);
    BOOST_CHECK(offset.HasOffset());
    BOOST_CHECK_EQUAL(ToString(offset), "+Mo +11 days");

    BOOST_CHECK(offset.IsWDayOffsetPositive());
    offset.SetWDayOffsetPositive(false);
    BOOST_CHECK(!offset.IsWDayOffsetPositive());
    BOOST_CHECK_EQUAL(ToString(offset), "-Mo +11 days");
  }
}

BOOST_AUTO_TEST_CASE(OpeningHours_TestMonthDay)
{
  using namespace osmoh;

  {
    MonthDay md;
    BOOST_CHECK(md.IsEmpty());
    BOOST_CHECK(!md.HasYear());
    BOOST_CHECK(!md.HasMonth());
    BOOST_CHECK(!md.HasDayNum());
    BOOST_CHECK(!md.HasOffset());
    BOOST_CHECK(!md.IsVariable());
    BOOST_CHECK_EQUAL(ToString(md), "");
  }
  {
    MonthDay md;
    md.SetVariableDate(MonthDay::VariableDate::Easter);
    BOOST_CHECK(!md.IsEmpty());
    BOOST_CHECK_EQUAL(ToString(md), "easter");
  }
  {
    MonthDay md;
    md.SetMonth(MonthDay::Month::Jul);
    BOOST_CHECK(!md.IsEmpty());
    BOOST_CHECK(md.HasMonth());
    BOOST_CHECK_EQUAL(ToString(md), "Jul");

    md.SetYear(1990);
    BOOST_CHECK(!md.IsEmpty());
    BOOST_CHECK(md.HasYear());
    BOOST_CHECK(md.HasYear());
    BOOST_CHECK_EQUAL(ToString(md), "1990 Jul");

    md.SetDayNum(17);
    BOOST_CHECK(!md.IsEmpty());
    BOOST_CHECK(md.HasYear());
    BOOST_CHECK(md.HasMonth());
    BOOST_CHECK(md.HasDayNum());
    BOOST_CHECK_EQUAL(ToString(md), "1990 Jul 17");

    DateOffset offset;
    offset.SetWDayOffset(Weekday::Monday);
    md.SetOffset(offset);
    BOOST_CHECK(md.HasOffset());
    BOOST_CHECK_EQUAL(ToString(md), "1990 Jul 17 +Mo");
  }
}

BOOST_AUTO_TEST_CASE(OpeningHours_TestMonthdayRange)
{
  using namespace osmoh;

  {
    MonthdayRange range;
    BOOST_CHECK(range.IsEmpty());
    BOOST_CHECK(!range.HasStart());
    BOOST_CHECK(!range.HasEnd());
    BOOST_CHECK(!range.HasPeriod());
    BOOST_CHECK(!range.HasPlus());
    BOOST_CHECK_EQUAL(ToString(range), "");
  }
  {
    MonthdayRange range;
    MonthDay md;

    md.SetYear(1990);
    md.SetMonth(MonthDay::Month::Sep);
    range.SetStart(md);

    BOOST_CHECK(!range.IsEmpty());
    BOOST_CHECK(range.HasStart());
    BOOST_CHECK(!range.HasEnd());
    BOOST_CHECK(!range.HasPeriod());
    BOOST_CHECK(!range.HasPlus());
    BOOST_CHECK_EQUAL(ToString(range), "1990 Sep");
  }
  {
    MonthdayRange range;
    MonthDay md;

    md.SetYear(1990);
    range.SetEnd(md);

    BOOST_CHECK(!range.IsEmpty());
    BOOST_CHECK(!range.HasStart());
    BOOST_CHECK(range.HasEnd());
    BOOST_CHECK(!range.HasPeriod());
    BOOST_CHECK(!range.HasPlus());
  }
  {
    MonthdayRange range;

    range.SetPlus(true);
    BOOST_CHECK(range.IsEmpty());
    BOOST_CHECK(!range.HasStart());
    BOOST_CHECK(!range.HasEnd());
    BOOST_CHECK(!range.HasPeriod());
    BOOST_CHECK(range.HasPlus());

    range.SetPeriod(7);
    BOOST_CHECK(range.IsEmpty());
    BOOST_CHECK(!range.HasStart());
    BOOST_CHECK(!range.HasEnd());
    BOOST_CHECK(range.HasPeriod());
    BOOST_CHECK(range.HasPlus());
  }
}

BOOST_AUTO_TEST_CASE(OpeningHours_YearRange)
{
  using namespace osmoh;

  {
    YearRange range;
    BOOST_CHECK(range.IsEmpty());
    BOOST_CHECK(!range.HasStart());
    BOOST_CHECK(!range.HasEnd());
    BOOST_CHECK(!range.HasPlus());
    BOOST_CHECK_EQUAL(ToString(range), "");

    range.SetStart(1812);
    BOOST_CHECK(range.HasStart());
    BOOST_CHECK(range.IsOpen());
    BOOST_CHECK_EQUAL(ToString(range), "1812");

    range.SetEnd(1815);
    BOOST_CHECK(range.HasEnd());
    BOOST_CHECK(!range.IsOpen());
    BOOST_CHECK_EQUAL(ToString(range), "1812-1815");

    BOOST_CHECK(!range.HasPeriod());
    range.SetPeriod(10);
    BOOST_CHECK(range.HasPeriod());
    BOOST_CHECK_EQUAL(ToString(range), "1812-1815/10");
  }
  {
    YearRange range;
    range.SetStart(1812);
    range.SetPlus(true);
    BOOST_CHECK(range.HasStart());
    BOOST_CHECK(range.IsOpen());
    BOOST_CHECK(range.HasPlus());
    BOOST_CHECK_EQUAL(ToString(range), "1812+");
  }
}

BOOST_AUTO_TEST_CASE(OpeningHours_WeekRange)
{
  using namespace osmoh;

  {
    WeekRange range;
    BOOST_CHECK(range.IsEmpty());
    BOOST_CHECK(!range.HasStart());
    BOOST_CHECK(!range.HasEnd());
    BOOST_CHECK_EQUAL(ToString(range), "");

    range.SetStart(18);
    BOOST_CHECK(range.HasStart());
    BOOST_CHECK(range.IsOpen());
    BOOST_CHECK_EQUAL(ToString(range), "18");

    range.SetEnd(42);
    BOOST_CHECK(range.HasEnd());
    BOOST_CHECK(!range.IsOpen());
    BOOST_CHECK_EQUAL(ToString(range), "18-42");

    BOOST_CHECK(!range.HasPeriod());
    range.SetPeriod(10);
    BOOST_CHECK(range.HasPeriod());
    BOOST_CHECK_EQUAL(ToString(range), "18-42/10");
  }
}

BOOST_AUTO_TEST_CASE(OpeningHours_RuleSequence)
{
  using namespace osmoh;

  {
    RuleSequence s;
  }
}

BOOST_AUTO_TEST_CASE(OpeningHoursTimerange_TestParseUnparse)
{
  {
    auto const rule = "06:00+";
    auto const parsedUnparsed = ParseAndUnparse<osmoh::TTimespans>(rule);
    BOOST_CHECK_EQUAL(parsedUnparsed, rule);
  }
  {
    auto const rule = "06:00-02:00";
    auto const parsedUnparsed = ParseAndUnparse<osmoh::TTimespans>(rule);
    BOOST_CHECK_EQUAL(parsedUnparsed, rule);
  }
  {
    auto const rule = "06:00-31:41";
    auto const parsedUnparsed = ParseAndUnparse<osmoh::TTimespans>(rule);
    BOOST_CHECK_EQUAL(parsedUnparsed, rule);
  }
  {
    auto const rule = "06:00-02:00+";
    auto const parsedUnparsed = ParseAndUnparse<osmoh::TTimespans>(rule);
    BOOST_CHECK_EQUAL(parsedUnparsed, rule);
  }
  {
    auto const rule = "06:00-02:00/03";
    auto const parsedUnparsed = ParseAndUnparse<osmoh::TTimespans>(rule);
    BOOST_CHECK_EQUAL(parsedUnparsed, rule);
  }
  {
    auto const rule = "06:00-02:00/21:03";
    auto const parsedUnparsed = ParseAndUnparse<osmoh::TTimespans>(rule);
    BOOST_CHECK_EQUAL(parsedUnparsed, rule);
  }
  {
    auto const rule = "dusk+";
    auto const parsedUnparsed = ParseAndUnparse<osmoh::TTimespans>(rule);
    BOOST_CHECK_EQUAL(parsedUnparsed, "sunset+");
  }
  {
    auto const rule = "dawn+";
    auto const parsedUnparsed = ParseAndUnparse<osmoh::TTimespans>(rule);
    BOOST_CHECK_EQUAL(parsedUnparsed, "sunrise+");
  }
  {
    auto const rule = "sunrise-sunset";
    auto const parsedUnparsed = ParseAndUnparse<osmoh::TTimespans>(rule);
    BOOST_CHECK_EQUAL(parsedUnparsed, rule);
  }
  {
    auto const rule = "(sunset-12:12)+";
    auto const parsedUnparsed = ParseAndUnparse<osmoh::TTimespans>(rule);
    BOOST_CHECK_EQUAL(parsedUnparsed, rule);
  }
  {
    auto const rule = "(dusk-12:12)+";
    auto const parsedUnparsed = ParseAndUnparse<osmoh::TTimespans>(rule);
    BOOST_CHECK_EQUAL(parsedUnparsed, "(sunset-12:12)+");
  }
  {
    auto const rule = "(sunrise-12:12)-sunset";
    auto const parsedUnparsed = ParseAndUnparse<osmoh::TTimespans>(rule);
    BOOST_CHECK_EQUAL(parsedUnparsed, rule);
  }
}

BOOST_AUTO_TEST_CASE(OpeningHoursWeekdays_TestParseUnparse)
{
  {
    auto const rule = "We[4] -2 days";
    auto const parsedUnparsed = ParseAndUnparse<osmoh::Weekdays>(rule);
    BOOST_CHECK_EQUAL(parsedUnparsed, rule);
  }
  {
    auto const rule = "Sa[4,5]";
    auto const parsedUnparsed = ParseAndUnparse<osmoh::Weekdays>(rule);
    BOOST_CHECK_EQUAL(parsedUnparsed, rule);
  }
  {
    auto const rule = "Mo[1,3]";
    auto const parsedUnparsed = ParseAndUnparse<osmoh::Weekdays>(rule);
    BOOST_CHECK_EQUAL(parsedUnparsed, rule);
  }
  {
    auto const rule = "Tu[4,5] +1 day";
    auto const parsedUnparsed = ParseAndUnparse<osmoh::Weekdays>(rule);
    BOOST_CHECK_EQUAL(parsedUnparsed, rule);
  }
  {
    auto const rule = "SH -2 days";
    auto const parsedUnparsed = ParseAndUnparse<osmoh::Weekdays>(rule);
    BOOST_CHECK_EQUAL(parsedUnparsed, rule);
  }
  {
    auto const rule = "SH +2 days";
    auto const parsedUnparsed = ParseAndUnparse<osmoh::Weekdays>(rule);
    BOOST_CHECK_EQUAL(parsedUnparsed, rule);
  }
  {
    auto const rule = "SH +1 day";
    auto const parsedUnparsed = ParseAndUnparse<osmoh::Weekdays>(rule);
    BOOST_CHECK_EQUAL(parsedUnparsed, rule);
  }
  {
    auto const rule = "PH";
    auto const parsedUnparsed = ParseAndUnparse<osmoh::Weekdays>(rule);
    BOOST_CHECK_EQUAL(parsedUnparsed, rule);
  }
  {
    auto const rule = "SH";
    auto const parsedUnparsed = ParseAndUnparse<osmoh::Weekdays>(rule);
    BOOST_CHECK_EQUAL(parsedUnparsed, rule);
  }
  {
    auto const rule = "Mo, We, Th, Fr";
    auto const parsedUnparsed = ParseAndUnparse<osmoh::Weekdays>(rule);
    BOOST_CHECK_EQUAL(parsedUnparsed, rule);
  }
  {
    auto const rule = "Fr-Sa";
    auto const parsedUnparsed = ParseAndUnparse<osmoh::Weekdays>(rule);
    BOOST_CHECK_EQUAL(parsedUnparsed, rule);
  }
  {
    auto const rule = "PH, Sa, Su";
    auto const parsedUnparsed = ParseAndUnparse<osmoh::Weekdays>(rule);
    BOOST_CHECK_EQUAL(parsedUnparsed, rule);
  }
  {
    auto const rule = "Sa";
    auto const parsedUnparsed = ParseAndUnparse<osmoh::Weekdays>(rule);
    BOOST_CHECK_EQUAL(parsedUnparsed, rule);
  }
}

BOOST_AUTO_TEST_CASE(OpeningHoursMonthdayRanges_TestParseUnparse)
{
  {
    auto const rule = "Jan";
    auto const parsedUnparsed = ParseAndUnparse<osmoh::TMonthdayRanges>(rule);
    BOOST_CHECK_EQUAL(parsedUnparsed, rule);
  }
  {
    auto const rule = "Mar 10+";
    auto const parsedUnparsed = ParseAndUnparse<osmoh::TMonthdayRanges>(rule);
    BOOST_CHECK_EQUAL(parsedUnparsed, rule);
  }
  {
    auto const rule = "Jan-Feb";
    auto const parsedUnparsed = ParseAndUnparse<osmoh::TMonthdayRanges>(rule);
    BOOST_CHECK_EQUAL(parsedUnparsed, rule);
  }
  {
    auto const rule = "easter -2 days+";
    auto const parsedUnparsed = ParseAndUnparse<osmoh::TMonthdayRanges>(rule);
    BOOST_CHECK_EQUAL(parsedUnparsed, rule);
  }
  {
    auto const rule = "Jan-Feb/10";
    auto const parsedUnparsed = ParseAndUnparse<osmoh::TMonthdayRanges>(rule);
    BOOST_CHECK_EQUAL(parsedUnparsed, rule);
  }
  {
    auto const rule = "Jan 11-Dec 10, Apr 01-Jun 02";
    auto const parsedUnparsed = ParseAndUnparse<osmoh::TMonthdayRanges>(rule);
    BOOST_CHECK_EQUAL(parsedUnparsed, rule);
  }
  {
    auto const rule = "2011 Jan";
    auto const parsedUnparsed = ParseAndUnparse<osmoh::TMonthdayRanges>(rule);
    BOOST_CHECK_EQUAL(parsedUnparsed, rule);
  }
  {
    auto const rule = "1989 Mar 10+";
    auto const parsedUnparsed = ParseAndUnparse<osmoh::TMonthdayRanges>(rule);
    BOOST_CHECK_EQUAL(parsedUnparsed, rule);
  }
  {
    auto const rule = "Jan 11 +Mo+";
    auto const parsedUnparsed = ParseAndUnparse<osmoh::TMonthdayRanges>(rule);
    BOOST_CHECK_EQUAL(parsedUnparsed, rule);
  }
  {
    auto const rule = "Jan 11 +3 days+";
    auto const parsedUnparsed = ParseAndUnparse<osmoh::TMonthdayRanges>(rule);
    BOOST_CHECK_EQUAL(parsedUnparsed, rule);
  }
  {
    auto const rule = "Feb 03 -Mo -2 days+";
    auto const parsedUnparsed = ParseAndUnparse<osmoh::TMonthdayRanges>(rule);
    BOOST_CHECK_EQUAL(parsedUnparsed, rule);
  }
  {
    auto const rule = "Feb 03 -Mo -2 days-Jan 11 +3 days";
    auto const parsedUnparsed = ParseAndUnparse<osmoh::TMonthdayRanges>(rule);
    BOOST_CHECK_EQUAL(parsedUnparsed, rule);
  }
  {
    auto const rule = "Feb 03 -Mo -2 days-Jan 11 +3 days, Mar, Apr";
    auto const parsedUnparsed = ParseAndUnparse<osmoh::TMonthdayRanges>(rule);
    BOOST_CHECK_EQUAL(parsedUnparsed, rule);
  }
  {
    auto const rule = "1941 Feb 03 -Mo -2 days-1945 Jan 11 +3 days, Mar, Apr";
    auto const parsedUnparsed = ParseAndUnparse<osmoh::TMonthdayRanges>(rule);
    BOOST_CHECK_EQUAL(parsedUnparsed, rule);
  }
}

BOOST_AUTO_TEST_CASE(OpeningHoursYearRanges_TestParseUnparse)
{
  /// @todo Single year was removed here:
  /// https://github.com/organicmaps/organicmaps/commit/ebe26a41da0744b3bc81d6b213406361f14d39b2
  /*
  {
    auto const rule = "1995";
    auto const parsedUnparsed = ParseAndUnparse<osmoh::TYearRanges>(rule);
    BOOST_CHECK_EQUAL(parsedUnparsed, rule);
  }
  */

  {
    auto const rule = "1997+";
    auto const parsedUnparsed = ParseAndUnparse<osmoh::TYearRanges>(rule);
    BOOST_CHECK_EQUAL(parsedUnparsed, rule);
  }
  {
    auto const rule = "2018-2019";
    auto const parsedUnparsed = ParseAndUnparse<osmoh::TYearRanges>(rule);
    BOOST_CHECK_EQUAL(parsedUnparsed, rule);
  }
  {
    auto const rule = "2018-2036/11";
    auto const parsedUnparsed = ParseAndUnparse<osmoh::TYearRanges>(rule);
    BOOST_CHECK_EQUAL(parsedUnparsed, rule);
  }
}

BOOST_AUTO_TEST_CASE(OpeningHoursWeekRanges_TestParseUnparse)
{
  {
    auto const rule = "week 15";
    auto const parsedUnparsed = ParseAndUnparse<osmoh::TWeekRanges>(rule);
    BOOST_CHECK_EQUAL(parsedUnparsed, rule);
  }
  {
    auto const rule = "week 19-31";
    auto const parsedUnparsed = ParseAndUnparse<osmoh::TWeekRanges>(rule);
    BOOST_CHECK_EQUAL(parsedUnparsed, rule);
  }
  {
    auto const rule = "week 18-36/3";
    auto const parsedUnparsed = ParseAndUnparse<osmoh::TWeekRanges>(rule);
    BOOST_CHECK_EQUAL(parsedUnparsed, rule);
  }
  {
    auto const rule = "week 18-36/3, 11";
    auto const parsedUnparsed = ParseAndUnparse<osmoh::TWeekRanges>(rule);
    BOOST_CHECK_EQUAL(parsedUnparsed, rule);
  }
}

BOOST_AUTO_TEST_CASE(OpeningHoursRuleSequence_TestParseUnparse)
{
  {
    auto const rule = "";
    auto const parsedUnparsed = ParseAndUnparse<osmoh::TRuleSequences>(rule);
    BOOST_CHECK_EQUAL(parsedUnparsed, rule);
  }
  {
    auto const rule = "24/7";
    auto const parsedUnparsed = ParseAndUnparse<osmoh::TRuleSequences>(rule);
    BOOST_CHECK_EQUAL(parsedUnparsed, rule);
  }
  {
    auto const rule = "06:00-09:00/03";
    auto const parsedUnparsed = ParseAndUnparse<osmoh::TRuleSequences>(rule);
    BOOST_CHECK_EQUAL(parsedUnparsed, rule);
  }
  {
    auto const rule = "Apr-Sep Su[1,3] 14:30-17:00";
    auto const parsedUnparsed = ParseAndUnparse<osmoh::TRuleSequences>(rule);
    BOOST_CHECK_EQUAL(parsedUnparsed, rule);
  }
  {
    auto const rule = "06:00+";
    auto const parsedUnparsed = ParseAndUnparse<osmoh::TRuleSequences>(rule);
    BOOST_CHECK_EQUAL(parsedUnparsed, rule);
  }
  {
    auto const rule = "06:00-07:00+";
    auto const parsedUnparsed = ParseAndUnparse<osmoh::TRuleSequences>(rule);
    BOOST_CHECK_EQUAL(parsedUnparsed, rule);
  }
  {
    auto const rule = "Mo-Su 08:00-23:00";
    auto const parsedUnparsed = ParseAndUnparse<osmoh::TRuleSequences>(rule);
    BOOST_CHECK_EQUAL(parsedUnparsed, rule);
  }
  {
    auto const rule = "Mo-Sa; PH closed";
    auto const parsedUnparsed = ParseAndUnparse<osmoh::TRuleSequences>(rule);
    BOOST_CHECK_EQUAL(parsedUnparsed, rule);
  }
  {
    auto const rule = "Jan-Mar 07:00-19:00; Apr-Sep 07:00-22:00; Oct-Dec 07:00-19:00";
    auto const parsedUnparsed = ParseAndUnparse<osmoh::TRuleSequences>(rule);
    BOOST_CHECK_EQUAL(parsedUnparsed, rule);
  }
  {
    auto const rule = "10:00-13:30, 17:00-20:30";
    auto const parsedUnparsed = ParseAndUnparse<osmoh::TRuleSequences>(rule);
    BOOST_CHECK_EQUAL(parsedUnparsed, rule);
  }
  {
    auto const rule = "Apr-Sep: Mo-Fr 09:00-13:00, 14:00-18:00; Apr-Sep: Sa 10:00-13:00";
    auto const parsedUnparsed = ParseAndUnparse<osmoh::TRuleSequences>(rule);
    BOOST_CHECK_EQUAL(parsedUnparsed, rule);
  }
  {
    auto const rule = "Tu-Su, PH 10:00-18:00";
    auto const parsedUnparsed = ParseAndUnparse<osmoh::TRuleSequences>(rule);
    BOOST_CHECK_EQUAL(parsedUnparsed, rule);
  }
  {
    auto const rule = "Mo, We, Th, Fr 12:00-18:00; Sa-Su 12:00-17:00";
    auto const parsedUnparsed = ParseAndUnparse<osmoh::TRuleSequences>(rule);
    BOOST_CHECK_EQUAL(parsedUnparsed, rule);
  }
  {
    auto const rule = "2016-2025";
    auto const parsedUnparsed = ParseAndUnparse<osmoh::TRuleSequences>(rule);
    BOOST_CHECK_EQUAL(parsedUnparsed, rule);
  }
  {
    auto const rule = "Feb 03 -Mo -2 days-Jan 11 +3 days";
    auto const parsedUnparsed = ParseAndUnparse<osmoh::TMonthdayRanges>(rule);
    BOOST_CHECK_EQUAL(parsedUnparsed, rule);
  }
  {
    auto const rule = "week 19-31";
    auto const parsedUnparsed = ParseAndUnparse<osmoh::TRuleSequences>(rule);
    BOOST_CHECK_EQUAL(parsedUnparsed, rule);
  }
  {
    auto const rule = "06:00-02:00/21:03, 18:15-sunset";
    auto const parsedUnparsed = ParseAndUnparse<osmoh::TRuleSequences>(rule);
    BOOST_CHECK_EQUAL(parsedUnparsed, rule);
  }
  {
    auto const rule = "06:13-15:00; 16:30+";
    auto const parsedUnparsed = ParseAndUnparse<osmoh::TRuleSequences>(rule);
    BOOST_CHECK_EQUAL(parsedUnparsed, rule);
  }
  {
    auto const rule = "We-Sa; Mo[1,3] closed";
    auto const parsedUnparsed = ParseAndUnparse<osmoh::TRuleSequences>(rule);
    BOOST_CHECK_EQUAL(parsedUnparsed, rule);
  }
  {
    auto const rule = "Mo-Fr 10:00-18:00, Sa 10:00-13:00";
    auto const parsedUnparsed = ParseAndUnparse<osmoh::TRuleSequences>(rule);
    BOOST_CHECK_EQUAL(parsedUnparsed, rule);
   }
  {
    auto const rule = ( "We-Sa; Mo[1,3] closed; Su[-1,-2] closed; "
                        "Fr[2] open; Fr[-2], Fr open; Su[-2] -2 days" );
    auto const parsedUnparsed = ParseAndUnparse<osmoh::TRuleSequences>(rule);
    BOOST_CHECK_EQUAL(parsedUnparsed, rule);
  }
  {
    auto const rule = "easter -2 days+: closed";
    auto const parsedUnparsed = ParseAndUnparse<osmoh::TRuleSequences>(rule);
    BOOST_CHECK_EQUAL(parsedUnparsed, rule);
  }
  {
    auto const rule = "easter: open";
    auto const parsedUnparsed = ParseAndUnparse<osmoh::TRuleSequences>(rule);
    BOOST_CHECK_EQUAL(parsedUnparsed, rule);
  }
  {
    auto const rule = ( "PH, Tu-Su 10:00-18:00; Sa[1] 10:00-18:00 open; "
                        "\"Eintritt ins gesamte Haus frei\"; "
                        "Jan 01, Dec 24, Dec 25, easter -2 days+: closed" );
    auto const parsedUnparsed = ParseAndUnparse<osmoh::TRuleSequences>(rule);
    BOOST_CHECK_EQUAL(parsedUnparsed, rule);
  }
  {
    auto const rule = "Su-Th sunrise-(sunset-24:00); Fr-Sa (sunrise+12:12)-sunset";
    auto const parsedUnparsed = ParseAndUnparse<osmoh::TRuleSequences>(rule);
    BOOST_CHECK_EQUAL(parsedUnparsed, rule);
  }
  {
    auto const rule = "2010 Apr 01-30: Mo-Su 17:00-24:00";
    auto const parsedUnparsed = ParseAndUnparse<osmoh::TRuleSequences>(rule);
    BOOST_CHECK_EQUAL(parsedUnparsed, rule);
  }
  {
    auto const rule = ("Mo-Th 14:00-22:00; Fr 14:00-24:00; "
                       "Sa 00:00-01:00, 14:00-24:00; "
                       "Su 00:00-01:00, 14:00-22:00");
    auto const parsedUnparsed = ParseAndUnparse<osmoh::TRuleSequences>(rule);
    BOOST_CHECK_EQUAL(parsedUnparsed, rule);
  }
  {
    auto const rule = "24/7 closed \"always closed\"";

    auto const parsedUnparsed = ParseAndUnparse<osmoh::TRuleSequences>(rule);
    BOOST_CHECK_EQUAL(parsedUnparsed, rule);
  }
  {
    auto const rule = "Mo-Fr closed \"always closed\"";

    auto const parsedUnparsed = ParseAndUnparse<osmoh::TRuleSequences>(rule);
    BOOST_CHECK_EQUAL(parsedUnparsed, rule);
  }
  {
    auto const rule = "Sa; Su";

    auto const parsedUnparsed = ParseAndUnparse<osmoh::TRuleSequences>(rule);
    BOOST_CHECK_EQUAL(parsedUnparsed, rule);
  }
  {
    auto const rule = "Sa-Su 00:00-24:00";

    auto const parsedUnparsed = ParseAndUnparse<osmoh::TRuleSequences>(rule);
    BOOST_CHECK_EQUAL(parsedUnparsed, rule);
  }
}

BOOST_AUTO_TEST_CASE(OpeningHours_TestIsActive)
{
   using namespace osmoh;

   {
    TTimespans spans;
    BOOST_CHECK(Parse("10:15-13:49", spans));

    std::tm time{};
    auto const fmt = "%H:%M";
    BOOST_CHECK(GetTimeTuple("12:41", fmt, time));
    BOOST_CHECK(IsActive(spans[0], time));

    BOOST_CHECK(GetTimeTuple("10:15", fmt, time));
    BOOST_CHECK(IsActive(spans[0], time));

    BOOST_CHECK(GetTimeTuple("10:14", fmt, time));
    BOOST_CHECK(!IsActive(spans[0], time));

    BOOST_CHECK(GetTimeTuple("13:49", fmt, time));
    BOOST_CHECK(IsActive(spans[0], time));

    BOOST_CHECK(GetTimeTuple("13:50", fmt, time));
    BOOST_CHECK(!IsActive(spans[0], time));
  }
  {
    Weekdays range;
    BOOST_CHECK(Parse("Su-Sa", range));

    std::tm time{};
    auto const fmt = "%w";
    BOOST_CHECK(GetTimeTuple("4", fmt, time));
    BOOST_CHECK(IsActive(range, time));

    BOOST_CHECK(GetTimeTuple("0", fmt, time));
    BOOST_CHECK(IsActive(range, time));

    BOOST_CHECK(GetTimeTuple("6", fmt, time));
    BOOST_CHECK(IsActive(range, time));


    BOOST_CHECK(Parse("Mo-Tu", range));
    BOOST_CHECK(GetTimeTuple("0", fmt, time));
    BOOST_CHECK(!IsActive(range, time));

    BOOST_CHECK(GetTimeTuple("5", fmt, time));
    BOOST_CHECK(!IsActive(range, time));


    BOOST_CHECK(Parse("Mo", range));
    BOOST_CHECK(GetTimeTuple("1", fmt, time));
    BOOST_CHECK(IsActive(range, time));

    BOOST_CHECK(GetTimeTuple("5", fmt, time));
    BOOST_CHECK(!IsActive(range, time));
  }
  {
    TMonthdayRanges ranges;
    BOOST_CHECK(Parse("2015 Sep-Oct", ranges));

    std::tm time{};
    auto const fmt = "%Y-%m";
    BOOST_CHECK(GetTimeTuple("2015-10", fmt, time));
    BOOST_CHECK(IsActive(ranges[0], time));

    BOOST_CHECK(GetTimeTuple("2015-09", fmt, time));
    BOOST_CHECK(IsActive(ranges[0], time));

    BOOST_CHECK(GetTimeTuple("2015-08", fmt, time));
    BOOST_CHECK(!IsActive(ranges[0], time));

    BOOST_CHECK(GetTimeTuple("2014-10", fmt, time));
    BOOST_CHECK(!IsActive(ranges[0], time));

    BOOST_CHECK(GetTimeTuple("2016-10", fmt, time));
    BOOST_CHECK(!IsActive(ranges[0], time));
  }
  {
    TMonthdayRanges ranges;
    BOOST_CHECK(Parse("2015 Sep", ranges));

    std::tm time{};
    auto const fmt = "%Y-%m";
    BOOST_CHECK(GetTimeTuple("2015-10", fmt, time));
    BOOST_CHECK(!IsActive(ranges[0], time));

    BOOST_CHECK(GetTimeTuple("2015-09", fmt, time));
    BOOST_CHECK(IsActive(ranges[0], time));

    BOOST_CHECK(GetTimeTuple("2015-08", fmt, time));
    BOOST_CHECK(!IsActive(ranges[0], time));

    BOOST_CHECK(GetTimeTuple("2014-10", fmt, time));
    BOOST_CHECK(!IsActive(ranges[0], time));

    BOOST_CHECK(GetTimeTuple("2016-10", fmt, time));
    BOOST_CHECK(!IsActive(ranges[0], time));
  }
  {
    TMonthdayRanges ranges;
    BOOST_CHECK(Parse("Sep-Nov", ranges));

    std::tm time{};
    auto const fmt = "%Y-%m";
    BOOST_CHECK(GetTimeTuple("2015-10", fmt, time));
    BOOST_CHECK(IsActive(ranges[0], time));

    BOOST_CHECK(GetTimeTuple("2015-09", fmt, time));
    BOOST_CHECK(IsActive(ranges[0], time));

    BOOST_CHECK(GetTimeTuple("2014-11", fmt, time));
    BOOST_CHECK(IsActive(ranges[0], time));

    BOOST_CHECK(GetTimeTuple("2016-10", fmt, time));
    BOOST_CHECK(IsActive(ranges[0], time));

    BOOST_CHECK(GetTimeTuple("2015-08", fmt, time));
    BOOST_CHECK(!IsActive(ranges[0], time));
  }
  {
    TMonthdayRanges ranges;
    BOOST_CHECK(Parse("Sep", ranges));

    std::tm time{};
    auto const fmt = "%Y-%m";
    BOOST_CHECK(GetTimeTuple("2015-09", fmt, time));
    BOOST_CHECK(IsActive(ranges[0], time));

    BOOST_CHECK(GetTimeTuple("2014-11", fmt, time));
    BOOST_CHECK(!IsActive(ranges[0], time));

    BOOST_CHECK(GetTimeTuple("2016-10", fmt, time));
    BOOST_CHECK(!IsActive(ranges[0], time));

    BOOST_CHECK(GetTimeTuple("2015-08", fmt, time));
    BOOST_CHECK(!IsActive(ranges[0], time));
  }
  {
    TMonthdayRanges ranges;
    BOOST_CHECK(Parse("Sep 01", ranges));

    std::tm time{};
    auto const fmt = "%Y-%m-%d";

    BOOST_CHECK(GetTimeTuple("2015-09-01", fmt, time));
    BOOST_CHECK(IsActive(ranges[0], time));

    BOOST_CHECK(GetTimeTuple("2014-09-02", fmt, time));
    BOOST_CHECK(!IsActive(ranges[0], time));
  }
  {
    TMonthdayRanges ranges;
    BOOST_CHECK(Parse("2015 Sep 01", ranges));

    std::tm time{};
    auto const fmt = "%Y-%m-%d";

    BOOST_CHECK(GetTimeTuple("2015-09-01", fmt, time));
    BOOST_CHECK(IsActive(ranges[0], time));

    BOOST_CHECK(GetTimeTuple("2014-09-01", fmt, time));
    BOOST_CHECK(!IsActive(ranges[0], time));
  }
  {
    TYearRanges ranges;
    BOOST_CHECK(Parse("2011-2014", ranges));

    std::tm time{};
    auto const fmt = "%Y";
    BOOST_CHECK(GetTimeTuple("2011", fmt, time));
    BOOST_CHECK(IsActive(ranges[0], time));

    BOOST_CHECK(GetTimeTuple("2012", fmt, time));
    BOOST_CHECK(IsActive(ranges[0], time));

    BOOST_CHECK(GetTimeTuple("2014", fmt, time));
    BOOST_CHECK(IsActive(ranges[0], time));

    BOOST_CHECK(GetTimeTuple("2010", fmt, time));
    BOOST_CHECK(!IsActive(ranges[0], time));

    BOOST_CHECK(GetTimeTuple("2015", fmt, time));
    BOOST_CHECK(!IsActive(ranges[0], time));
  }

  /// @todo Single year was removed here:
  /// https://github.com/organicmaps/organicmaps/commit/ebe26a41da0744b3bc81d6b213406361f14d39b2
  /*
  {
    TYearRanges ranges;
    BOOST_REQUIRE(Parse("2011", ranges));

    std::tm time;
    auto const fmt = "%Y";
    BOOST_CHECK(GetTimeTuple("2011", fmt, time));
    BOOST_CHECK(IsActive(ranges[0], time));

    BOOST_CHECK(GetTimeTuple("2012", fmt, time));
    BOOST_CHECK(!IsActive(ranges[0], time));
  }
  */

  /// See https://en.wikipedia.org/wiki/ISO_week_date#First_week
  {
    TWeekRanges ranges;
    BOOST_CHECK(Parse("week 01-02", ranges));

    std::tm time{};
    auto const fmt = "%Y %j %w";
    BOOST_CHECK(GetTimeTuple("2015 01 4", fmt, time));
    BOOST_CHECK(IsActive(ranges[0], time));

    BOOST_CHECK(GetTimeTuple("2015 08 4", fmt, time));
    BOOST_CHECK(IsActive(ranges[0], time));

    BOOST_CHECK(GetTimeTuple("2015 14 3", fmt, time));
    BOOST_CHECK(!IsActive(ranges[0], time));
  }
  {
    TWeekRanges ranges;
    BOOST_CHECK(Parse("week 02", ranges));

    std::tm time{};
    auto const fmt = "%Y %j %w";
    BOOST_CHECK(GetTimeTuple("2015 08 4", fmt, time));
    BOOST_CHECK(IsActive(ranges[0], time));

    BOOST_CHECK(GetTimeTuple("2015 05 1", fmt, time));
    BOOST_CHECK(IsActive(ranges[0], time));

    BOOST_CHECK(GetTimeTuple("2015 04 0", fmt, time));
    BOOST_CHECK(!IsActive(ranges[0], time));

    BOOST_CHECK(GetTimeTuple("2015 14 3", fmt, time));
    BOOST_CHECK(!IsActive(ranges[0], time));
  }
  {
    TRuleSequences rules;
    BOOST_CHECK(Parse("09:00-14:00", rules));

    std::tm time{};
    auto const fmt = "%Y %H:%M";
    BOOST_CHECK(GetTimeTuple("2088 11:35", fmt, time));
    BOOST_CHECK(IsActive(rules[0], time));

    BOOST_CHECK(GetTimeTuple("3000 15:35", fmt, time));
    BOOST_CHECK(!IsActive(rules[0], time));
  }
  {
    TRuleSequences rules;
    BOOST_CHECK(Parse("Mo-We 17:00-18:00, Th,Fr 15:00-16:00", rules));

    std::tm time{};
    auto const fmt = "%Y-%m-%d %H:%M";
    BOOST_CHECK(GetTimeTuple("2015-10-6 17:35", fmt, time));
    BOOST_CHECK(IsActive(rules[0], time));

    BOOST_CHECK(GetTimeTuple("2015-10-8 15:35", fmt, time));
    BOOST_CHECK(IsActive(rules[1], time));
  }

  auto const kDateTimeFmt = "%Y-%m-%d %H:%M";

  {
    TRuleSequences rules;
    BOOST_CHECK(Parse("2010 Apr 01-30: Mo-Su 17:00-24:00", rules));

    std::tm time{};
    BOOST_CHECK(GetTimeTuple("2010-4-20 18:15", kDateTimeFmt, time));
    BOOST_CHECK(IsActive(rules[0], time));
  }
  {
    TRuleSequences rules;
    BOOST_CHECK(Parse("2010 Apr 02 - May 21: Mo-Su 17:00-24:00", rules));

    std::tm time{};
    BOOST_CHECK(GetTimeTuple("2010-4-20 18:15", kDateTimeFmt, time));
    BOOST_CHECK(IsActive(rules[0], time));

    BOOST_CHECK(GetTimeTuple("2010-5-20 18:15", kDateTimeFmt, time));
    BOOST_CHECK(IsActive(rules[0], time));

    BOOST_CHECK(GetTimeTuple("2010-4-01 18:15", kDateTimeFmt, time));
    BOOST_CHECK(!IsActive(rules[0], time));

    BOOST_CHECK(GetTimeTuple("2010-5-22 18:15", kDateTimeFmt, time));
    BOOST_CHECK(!IsActive(rules[0], time));

    BOOST_CHECK(GetTimeTuple("2010-6-01 18:15", kDateTimeFmt, time));
    BOOST_CHECK(!IsActive(rules[0], time));
  }
  {
    TRuleSequences rules;
    BOOST_CHECK(Parse("Apr-Sep Su 14:30-17:00", rules));

    std::tm time{};
    BOOST_CHECK(GetTimeTuple("2015-04-12 15:35", kDateTimeFmt, time));
    BOOST_CHECK(IsActive(rules[0], time));

    BOOST_CHECK(GetTimeTuple("2015-04-13 15:35", kDateTimeFmt, time));
    BOOST_CHECK(!IsActive(rules[0], time));

    BOOST_CHECK(GetTimeTuple("2015-10-11 15:35", kDateTimeFmt, time));
    BOOST_CHECK(!IsActive(rules[0], time));
  }
  {
    TRuleSequences rules;
    BOOST_CHECK(Parse("Mo 09:00-06:00", rules));

    std::tm time{};
    BOOST_CHECK(GetTimeTuple("2015-11-10 05:35", kDateTimeFmt, time));
    BOOST_CHECK(IsActive(rules[0], time));

    BOOST_CHECK(GetTimeTuple("2015-11-11 05:35", kDateTimeFmt, time));
    BOOST_CHECK(!IsActive(rules[0], time));

    BOOST_CHECK(GetTimeTuple("2015-11-11 06:01", kDateTimeFmt, time));
    BOOST_CHECK(!IsActive(rules[0], time));
  }
  {
    TRuleSequences rules;
    BOOST_CHECK(Parse("Mo 09:00-32:00", rules));

    std::tm time{};
    BOOST_CHECK(GetTimeTuple("2015-11-10 05:35", kDateTimeFmt, time));
    BOOST_CHECK(IsActive(rules[0], time));

    BOOST_CHECK(GetTimeTuple("2015-11-11 05:35", kDateTimeFmt, time));
    BOOST_CHECK(!IsActive(rules[0], time));

    BOOST_CHECK(GetTimeTuple("2015-11-11 06:01", kDateTimeFmt, time));
    BOOST_CHECK(!IsActive(rules[0], time));
  }
  {
    TRuleSequences rules;
    BOOST_CHECK(Parse("Mo 09:00-48:00", rules));

    std::tm time{};
    BOOST_CHECK(GetTimeTuple("2015-11-10 05:35", kDateTimeFmt, time));
    BOOST_CHECK(IsActive(rules[0], time));

    BOOST_CHECK(GetTimeTuple("2015-11-10 15:35", kDateTimeFmt, time));
    BOOST_CHECK(IsActive(rules[0], time));

    BOOST_CHECK(GetTimeTuple("2015-11-10 23:35", kDateTimeFmt, time));
    BOOST_CHECK(IsActive(rules[0], time));

    BOOST_CHECK(GetTimeTuple("2015-11-11 05:35", kDateTimeFmt, time));
    BOOST_CHECK(!IsActive(rules[0], time));

    BOOST_CHECK(GetTimeTuple("2015-11-11 06:01", kDateTimeFmt, time));
    BOOST_CHECK(!IsActive(rules[0], time));
  }
  {
    TRuleSequences rules;
    BOOST_CHECK(Parse("Mo-We 00:00-24:00", rules));

    std::tm time{};
    BOOST_CHECK(GetTimeTuple("2016-10-03 05:35", kDateTimeFmt, time));
    BOOST_CHECK(IsActive(rules[0], time));

    BOOST_CHECK(GetTimeTuple("2017-01-17 15:35", kDateTimeFmt, time));
    BOOST_CHECK(IsActive(rules[0], time));

    BOOST_CHECK(GetTimeTuple("2017-05-31 23:35", kDateTimeFmt, time));
    BOOST_CHECK(IsActive(rules[0], time));

    BOOST_CHECK(GetTimeTuple("2017-02-10 05:35", kDateTimeFmt, time));
    BOOST_CHECK(!IsActive(rules[0], time));

    BOOST_CHECK(GetTimeTuple("2017-05-21 06:01", kDateTimeFmt, time));
    BOOST_CHECK(!IsActive(rules[0], time));
  }
  {
    TRuleSequences rules;
    BOOST_CHECK(Parse("Mo-We 00:00-24:00 off", rules));

    std::tm time{};
    BOOST_CHECK(GetTimeTuple("2016-10-03 05:35", kDateTimeFmt, time));
    BOOST_CHECK(IsActive(rules[0], time));

    BOOST_CHECK(GetTimeTuple("2017-01-17 15:35", kDateTimeFmt, time));
    BOOST_CHECK(IsActive(rules[0], time));

    BOOST_CHECK(GetTimeTuple("2017-05-31 23:35", kDateTimeFmt, time));
    BOOST_CHECK(IsActive(rules[0], time));

    BOOST_CHECK(GetTimeTuple("2017-02-10 05:35", kDateTimeFmt, time));
    BOOST_CHECK(!IsActive(rules[0], time));

    BOOST_CHECK(GetTimeTuple("2017-05-21 06:01", kDateTimeFmt, time));
    BOOST_CHECK(!IsActive(rules[0], time));
  }
}

BOOST_AUTO_TEST_CASE(OpeningHours_TestIsOpen)
{
  using namespace osmoh;

  {
    TRuleSequences rules;
    BOOST_CHECK(Parse("2010 Apr 01-30: Mo-Su 17:00-24:00", rules));

    BOOST_CHECK(IsOpen(rules, "2010-04-12 19:15"));
    BOOST_CHECK(IsClosed(rules, "2010-04-12 14:15"));
    BOOST_CHECK(IsClosed(rules, "2011-04-12 20:15"));
  }
  {
    TRuleSequences rules;
    BOOST_CHECK(Parse("Mo-Th 14:00-22:00; Fr 14:00-16:00;"
                      "Sa 00:00-01:00, 14:00-24:00 closed; "
                      "Su 00:00-01:00, 14:00-22:00", rules));

    BOOST_CHECK(IsOpen(rules, "2010-05-05 19:15"));
    BOOST_CHECK(IsClosed(rules, "2010-05-05 12:15"));

    BOOST_CHECK(IsClosed(rules, "2010-04-10 15:15"));   // Saturday
    /// If no selectors with `open' modifier match than state is closed.
    BOOST_CHECK(IsClosed(rules, "2010-04-10 11:15"));

    BOOST_CHECK(IsOpen(rules, "2010-04-11 14:15"));
    BOOST_CHECK(IsClosed(rules, "2010-04-11 23:45"));
  }
  {
    TRuleSequences rules;
    BOOST_CHECK(Parse("Mo-Tu 15:00-18:00; We off; "
                      "Th-Fr 15:00-18:00; Sa 10:00-12:00", rules));

    BOOST_CHECK(IsClosed(rules, "2015-11-04 16:00"));
    BOOST_CHECK(IsOpen(rules, "2015-11-02 16:00"));
  }
  {
    TRuleSequences rules;
    BOOST_CHECK(Parse("Su 11:00-17:00; \"Wochentags auf Anfrage\"", rules));

    BOOST_CHECK(IsOpen(rules, "2015-11-08 12:30"));
    BOOST_CHECK(IsUnknown(rules, "2015-11-09 12:30"));
  }
  {
    TRuleSequences rules;
    BOOST_CHECK(Parse("PH open", rules));
    BOOST_CHECK(Parse("PH closed", rules));
    BOOST_CHECK(Parse("PH on", rules));
    BOOST_CHECK(Parse("PH off", rules));

    /// @todo Single PH entries are not supported yet, always closed?!
    BOOST_CHECK(IsClosed(rules, "2021-05-07 11:23"));   // Friday
    BOOST_CHECK(IsClosed(rules, "2015-11-08 12:30"));   // Sunday
  }
  {
    TRuleSequences rules;
    BOOST_CHECK(Parse("Mo-Sa 08:00-20:00; Dec Mo-Sa 08:00-14:00; Dec 25 off", rules));

    BOOST_CHECK(IsClosed(rules, "2020-12-25 11:11"));

    BOOST_CHECK(IsOpen(rules, "2020-12-24 13:50"));     // Thursday
    BOOST_CHECK(IsClosed(rules, "2020-12-24 14:10"));   // Thursday
    BOOST_CHECK(IsClosed(rules, "2020-12-27 12:00"));   // Sunday

    BOOST_CHECK(IsOpen(rules, "2021-05-07 13:50"));     // Friday
    BOOST_CHECK(IsOpen(rules, "2021-05-08 19:40"));     // Saturdaya
    BOOST_CHECK(IsClosed(rules, "2021-05-09 12:00"));   // Sunday
  }

  /// @todo Synthetic example with ill-formed OH, but documented behaviour.
  /// @see rules_evaluation.cpp/IsR1IncludesR2
  /*
  {
    TRuleSequences rules;
    BOOST_CHECK(Parse("Mo-Sa 08:00-20:00; Fr 08:00-14:00", rules));

    BOOST_CHECK(IsOpen(rules, "2021-05-07 13:50"));     // Friday
    BOOST_CHECK(IsClosed(rules, "2021-05-07 14:10"));   // Friday
  }
  */

  {
    TRuleSequences rules;
    BOOST_CHECK(Parse("Mo-Sa 08:00-20:00; Dec 24 Mo-Sa 08:00-14:00; PH off", rules));

    BOOST_CHECK(IsClosed(rules, "2020-12-24 14:10"));   // Thursday

    BOOST_CHECK(IsOpen(rules, "2021-05-07 11:12"));     // Friday
    BOOST_CHECK(IsOpen(rules, "2021-05-08 13:14"));     // Saturday
    BOOST_CHECK(IsClosed(rules, "2021-05-09 15:16"));   // Sunday
  }
  {
    TRuleSequences rules;
    BOOST_CHECK(Parse("Apr 01-Sep 30 11:00-15:00, "
                      "Mo off, Fr off; "
                      "week 27-32 11:00-17:00", rules));

    BOOST_CHECK(IsClosed(rules, "2015-11-9 12:20"));
    BOOST_CHECK(IsClosed(rules, "2015-11-13 12:20"));

    BOOST_CHECK(IsOpen(rules, "2015-04-08 12:20"));
    BOOST_CHECK(IsOpen(rules, "2015-09-15 12:20"));

    /// week 28th of 2015, Tu
    BOOST_CHECK(IsOpen(rules, "2015-07-09 16:50"));
    BOOST_CHECK(IsClosed(rules, "2015-08-14 12:00"));
  }
  {
    TRuleSequences rules;
    BOOST_CHECK(Parse("06:13-15:00; 16:30+", rules));

    BOOST_CHECK(IsOpen(rules, "2013-12-12 7:00"));
    BOOST_CHECK(IsOpen(rules, "2013-12-12 20:00"));
    BOOST_CHECK(IsClosed(rules, "2013-12-12 16:00"));
  }
  {
    TRuleSequences rules;
    BOOST_CHECK(Parse("10:00-12:00", rules));

    BOOST_CHECK(IsOpen(rules, "2013-12-12 10:01"));
    BOOST_CHECK(IsClosed(rules, "2013-12-12 12:01"));
  }
  {
    TRuleSequences rules;
    BOOST_CHECK(Parse("24/7; Mo 15:00-16:00 off", rules));

    BOOST_CHECK(IsOpen(rules, "2012-10-08 00:01"));
    BOOST_CHECK(IsClosed(rules, "2012-10-08 15:59"));
  }
  {
    TRuleSequences rules;
    BOOST_CHECK(Parse("Mo-Su 12:00-23:00", rules));

    BOOST_CHECK(IsOpen(rules, "2015-11-06 18:40"));
    BOOST_CHECK(!IsClosed(rules, "2015-11-06 18:40"));
  }
  {
    TRuleSequences rules;
    BOOST_CHECK(Parse("2015 Apr 01-30: Mo-Fr 17:00-08:00", rules));

    BOOST_CHECK(IsOpen(rules, "2015-04-10 07:15"));
    BOOST_CHECK(IsOpen(rules, "2015-05-01 07:15"));
    BOOST_CHECK(IsOpen(rules, "2015-04-11 07:15"));
    BOOST_CHECK(IsClosed(rules, "2015-04-12 14:15"));
    BOOST_CHECK(IsClosed(rules, "2016-04-12 20:15"));
  }
  {
    TRuleSequences rules;
    BOOST_CHECK(Parse("Mo-Th 15:00+; Fr-Su 13:00+", rules));

    BOOST_CHECK(!IsOpen(rules, "2016-06-06 13:14"));
    BOOST_CHECK(IsOpen(rules, "2016-06-06 17:06"));
    BOOST_CHECK(IsOpen(rules, "2016-06-05 13:06"));
    BOOST_CHECK(IsOpen(rules, "2016-05-31 18:28"));
  }
  {
    TRuleSequences rules;
    BOOST_CHECK(Parse("Mo-We 00:00-24:00", rules));

    BOOST_CHECK(IsOpen(rules, "2016-10-03 05:35"));
    BOOST_CHECK(IsOpen(rules, "2017-01-17 15:35"));
    BOOST_CHECK(IsOpen(rules, "2017-05-31 23:35"));
    BOOST_CHECK(!IsOpen(rules, "2017-02-10 05:35"));
    BOOST_CHECK(!IsOpen(rules, "2017-05-21 06:01"));
  } 
  {
    TRuleSequences rules;
    BOOST_CHECK(Parse("Mo-Su 00:00-24:00; Mo-We 00:00-24:00 off", rules));

    BOOST_CHECK(!IsOpen(rules, "2016-10-03 05:35"));
    BOOST_CHECK(!IsOpen(rules, "2017-01-17 15:35"));
    BOOST_CHECK(!IsOpen(rules, "2017-05-31 23:35"));
    BOOST_CHECK(IsOpen(rules, "2017-02-10 05:35"));
    BOOST_CHECK(IsOpen(rules, "2017-05-21 06:01"));
  }
}

BOOST_AUTO_TEST_CASE(OpeningHours_GetNextTimeOpen)
{
  using namespace osmoh;

  TRuleSequences rules;

  char constexpr fmt[] = "%Y-%m-%d %H:%M";

  BOOST_CHECK(Parse("Mo-Tu 15:00-18:00; We off; Th on; Fr 15:00-18:00; Sa 10:00-12:00", rules));
  BOOST_CHECK(GetNextTimeOpen(rules, fmt, "2022-01-03 09:00") == "2022-01-03 15:00");    // Mo
  BOOST_CHECK(GetNextTimeClosed(rules, fmt, "2022-01-03 16:00") == "2022-01-03 18:00");  // Mo
  BOOST_CHECK(GetNextTimeOpen(rules, fmt, "2022-01-04 09:00") == "2022-01-04 15:00");    // Tu
  BOOST_CHECK(GetNextTimeClosed(rules, fmt, "2022-01-04 16:00") == "2022-01-04 18:00");  // Tu
  BOOST_CHECK(GetNextTimeOpen(rules, fmt, "2022-01-05 09:00") == "2022-01-06 00:00");    // We
  BOOST_CHECK(GetNextTimeClosed(rules, fmt, "2022-01-06 16:00") == "2022-01-07 00:00");  // Th
  BOOST_CHECK(GetNextTimeOpen(rules, fmt, "2022-01-07 09:00") == "2022-01-07 15:00");    // Fr
  BOOST_CHECK(GetNextTimeClosed(rules, fmt, "2022-01-07 16:00") == "2022-01-07 18:00");  // Fr
  BOOST_CHECK(GetNextTimeOpen(rules, fmt, "2022-01-08 09:00") == "2022-01-08 10:00");    // Sa
  BOOST_CHECK(GetNextTimeClosed(rules, fmt, "2022-01-08 11:00") == "2022-01-08 12:00");  // Sa
  BOOST_CHECK(GetNextTimeOpen(rules, fmt, "2022-01-09 09:00") == "2022-01-10 15:00");    // Su

  BOOST_CHECK(Parse("Mo-Fr 09:00-12:00, 13:00-20:00; We 10:00-11:00 off; Fr off", rules));
  BOOST_CHECK(GetNextTimeOpen(rules, fmt, "2022-01-03 07:00") == "2022-01-03 09:00");    // Mo morning
  BOOST_CHECK(GetNextTimeClosed(rules, fmt, "2022-01-03 10:00") == "2022-01-03 12:00");  // Mo morning
  BOOST_CHECK(GetNextTimeOpen(rules, fmt, "2022-01-03 12:30") == "2022-01-03 13:00");    // Mo afternoon
  BOOST_CHECK(GetNextTimeClosed(rules, fmt, "2022-01-03 13:30") == "2022-01-03 20:00");  // Mo afternoon
  BOOST_CHECK(GetNextTimeOpen(rules, fmt, "2022-01-03 21:00") == "2022-01-04 09:00");    // Mo night
  BOOST_CHECK(GetNextTimeClosed(rules, fmt, "2022-01-05 09:30") == "2022-01-05 10:00");  // We off
  BOOST_CHECK(GetNextTimeOpen(rules, fmt, "2022-01-05 10:30") == "2022-01-05 11:00");    // We off
  BOOST_CHECK(GetNextTimeOpen(rules, fmt, "2022-01-07 07:30") == "2022-01-10 09:00");    // Fr off

  BOOST_CHECK(Parse("Mo-Sa 08:00-20:00; Feb Mo-Sa 09:00-14:00; Jan 06 off", rules));
  BOOST_CHECK(GetNextTimeOpen(rules, fmt, "2022-01-04 07:00") == "2022-01-04 08:00");    // Tu Jan
  BOOST_CHECK(GetNextTimeClosed(rules, fmt, "2022-01-04 09:00") == "2022-01-04 20:00");  // Tu Jan
  BOOST_CHECK(GetNextTimeOpen(rules, fmt, "2022-02-08 07:00") == "2022-02-08 09:00");    // Tu Feb
  BOOST_CHECK(GetNextTimeClosed(rules, fmt, "2022-02-08 09:00") == "2022-02-08 14:00");  // Tu Feb
  BOOST_CHECK(GetNextTimeOpen(rules, fmt, "2020-01-06 07:00") == "2020-01-07 08:00");    // Jan 06

  BOOST_CHECK(Parse("24/7; Mo 15:00-16:00 off", rules));
  BOOST_CHECK(GetNextTimeOpen(rules, fmt, "2022-01-03 15:30") == "2022-01-03 16:00");    // Mo
  BOOST_CHECK(GetNextTimeClosed(rules, fmt, "2022-01-01 15:30") == "2022-01-03 15:00");  // Sa

  BOOST_CHECK(Parse("Mo-Th 15:00+; Fr-Su 13:00+", rules));
  BOOST_CHECK(GetNextTimeOpen(rules, fmt, "2022-01-03 07:30") == "2022-01-03 15:00");    // Mo
  BOOST_CHECK(GetNextTimeClosed(rules, fmt, "2022-01-03 15:30") == "2022-01-04 00:00");  // Mo
  BOOST_CHECK(GetNextTimeOpen(rules, fmt, "2022-01-08 07:30") == "2022-01-08 13:00");    // Sa
  BOOST_CHECK(GetNextTimeClosed(rules, fmt, "2022-01-08 15:30") == "2022-01-09 00:00");  // Sa

  BOOST_CHECK(Parse("Mo-Su 00:00-24:00; Mo-We 00:00-24:00 off", rules));
  BOOST_CHECK(GetNextTimeOpen(rules, fmt, "2022-01-03 15:30") == "2022-01-06 00:00");    // Mo
  BOOST_CHECK(GetNextTimeClosed(rules, fmt, "2022-01-01 15:30") == "2022-01-03 00:00");  // Sa
}


BOOST_AUTO_TEST_CASE(OpeningHours_TestOpeningHours)
{
  // OpeningHours is just a wrapper. So a couple of tests is
  // enough to check if it works.

  static auto const & fmt = "%Y-%m-%d %H:%M";

  using namespace osmoh;

  {
    OpeningHours oh("Su 11:00-17:00; \"Wochentags auf Anfrage\"; Tu off");
    BOOST_CHECK(oh.IsValid());

    std::tm time = {};
    BOOST_CHECK(GetTimeTuple("2015-11-08 12:30", fmt, time));
    BOOST_CHECK(oh.IsOpen(mktime(&time)));

    BOOST_CHECK(GetTimeTuple("2015-11-09 12:30", fmt, time));
    BOOST_CHECK(oh.IsUnknown(mktime(&time)));

    BOOST_CHECK(GetTimeTuple("2015-11-10 12:30", fmt, time));
    BOOST_CHECK(oh.IsClosed(mktime(&time)));
  }
  {
    OpeningHours oh("Nov +1");
    BOOST_CHECK(!oh.IsValid());
  }
  {
    OpeningHours oh("Mo-Th 15:00+; Fr-Su 13:00+");
    BOOST_CHECK(oh.IsValid());

    std::tm time = {};
    BOOST_CHECK(GetTimeTuple("2016-05-31 18:28", fmt, time));
    BOOST_CHECK(oh.IsOpen(mktime(&time)));

    BOOST_CHECK(GetTimeTuple("2016-05-31 22:28", fmt, time));
    BOOST_CHECK(oh.IsOpen(mktime(&time)));

    BOOST_CHECK(GetTimeTuple("2016-05-31 10:30", fmt, time));
    BOOST_CHECK(oh.IsClosed(mktime(&time)));
  }
}
