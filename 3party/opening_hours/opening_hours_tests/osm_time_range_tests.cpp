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
#include "parse.hpp"
#include "rules_evaluation.hpp"

#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <locale>
#include <map>
#include <sstream>

#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/replace.hpp>

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
} // namespace

template <typename ParserResult>
bool CompareNormalized(std::string const & original, ParserResult const & pretendent)
{
  auto original_copy = original;
  auto pretendent_copy = ToString(pretendent);

  boost::to_lower(original_copy);
  boost::to_lower(pretendent_copy);

  boost::replace_all(original_copy, "off", "closed");

  boost::replace_all(original_copy, " ", "");
  boost::replace_all(pretendent_copy, " ", "");

  return pretendent_copy == original_copy;
}

BOOST_AUTO_TEST_CASE(OpeningHours_Locale)
{
  namespace charset = boost::spirit::standard_wide;
  namespace qi = boost::spirit::qi;

  class Alltime : public qi::symbols<wchar_t>
  {
  public:
    Alltime()
    {
      add
          (L"пн")(L"uu")(L"œæ")
      ;
    }
  } alltime;

  std::locale loc("en_US");
  std::locale prev = std::locale::global(loc);

  BOOST_CHECK(Test("TeSt", charset::no_case[qi::lit("test")]));
  BOOST_CHECK(Test("UU", charset::no_case[alltime]));

  std::locale::global(prev);
}

BOOST_AUTO_TEST_CASE(OpeningHours_TestTime)
{
  using namespace osmoh;

  {
    BOOST_CHECK(!Time{}.HasValue());
    BOOST_CHECK_EQUAL(ToString(Time{}), "hh:mm");
  }
  {
    Time time{10_min};
    BOOST_CHECK(time.HasValue());
    BOOST_CHECK(!time.IsHoursMinutes());
    BOOST_CHECK(!time.IsTime());
    BOOST_CHECK(time.IsMinutes());
    BOOST_CHECK(!time.IsEvent());
    BOOST_CHECK(!time.IsEventOffset());

    BOOST_CHECK_EQUAL(ToString(time), "10");
  }
  {
    Time time{100_min};
    BOOST_CHECK(time.HasValue());
    BOOST_CHECK(time.IsHoursMinutes());
    BOOST_CHECK(time.IsTime());
    BOOST_CHECK(!time.IsMinutes());
    BOOST_CHECK(!time.IsEvent());
    BOOST_CHECK(!time.IsEventOffset());

    BOOST_CHECK_EQUAL(time.GetHoursCount(), 1);
    BOOST_CHECK_EQUAL(time.GetMinutesCount(), 40);

    BOOST_CHECK_EQUAL(ToString(time), "01:40");
  }
  {
    Time time{};
    time.SetHours(22_h);
    time = time + 15_min;
    BOOST_CHECK(time.HasValue());
    BOOST_CHECK(time.IsHoursMinutes());
    BOOST_CHECK(time.IsTime());
    BOOST_CHECK(!time.IsMinutes());
    BOOST_CHECK(!time.IsEvent());
    BOOST_CHECK(!time.IsEventOffset());

    BOOST_CHECK_EQUAL(time.GetHoursCount(), 22);
    BOOST_CHECK_EQUAL(time.GetMinutesCount(), 15);

    BOOST_CHECK_EQUAL(ToString(time), "22:15");
  }
  {
    Time time{};
    time.SetEvent(Time::Event::Sunrise);
    BOOST_CHECK(time.HasValue());
    BOOST_CHECK(!time.IsHoursMinutes());
    BOOST_CHECK(time.IsTime());
    BOOST_CHECK(!time.IsMinutes());
    BOOST_CHECK(time.IsEvent());
    BOOST_CHECK(!time.IsEventOffset());

    BOOST_CHECK_EQUAL(ToString(time), "sunrise");

    time = time - 90_min;
    BOOST_CHECK(time.IsEventOffset());
    BOOST_CHECK_EQUAL(ToString(time), "(sunrise-01:30)");
  }
  {
    Time time{};
    time.SetEvent(Time::Event::Sunrise);
    time.SetHours(22_h);
    time.SetMinutes(15_min);
    BOOST_CHECK(time.HasValue());
    BOOST_CHECK(!time.IsHoursMinutes());
    BOOST_CHECK(time.IsTime());
    BOOST_CHECK(!time.IsMinutes());
    BOOST_CHECK(time.IsEvent());
    BOOST_CHECK(time.IsEventOffset());
  }
  {
    Time time{10_min};
    BOOST_CHECK_EQUAL((-time).GetMinutesCount(), -10);
    BOOST_CHECK_EQUAL(ToString(-time), "10");
  }
  {
    Time t1{2_h};
    Time t2{100_min};
    Time t3 = t1 - t2;
    BOOST_CHECK_EQUAL(t3.GetHoursCount(), 0);
    BOOST_CHECK_EQUAL(t3.GetMinutesCount(), 20);
  }
  {
    Time time {27_h + 30_min};
    BOOST_CHECK_EQUAL(ToString(time), "27:30");
  }
  // TODO(mgsergio): more tests with event and get hours/minutes
}

BOOST_AUTO_TEST_CASE(OpeningHours_TestTimespan)
{
  using namespace osmoh;

  {
    Timespan span;
    BOOST_CHECK(span.IsEmpty());
    BOOST_CHECK(!span.HasStart());
    BOOST_CHECK(!span.HasEnd());
    BOOST_CHECK_EQUAL(ToString(span), "hh:mm-hh:mm");

    span.SetStart(10_h);
    BOOST_CHECK(span.HasStart());
    BOOST_CHECK(span.IsOpen());
    BOOST_CHECK_EQUAL(ToString(span), "10:00");

    span.SetEnd(12_h);
    BOOST_CHECK(span.HasEnd());
    BOOST_CHECK(!span.IsOpen());
    BOOST_CHECK_EQUAL(ToString(span), "10:00-12:00");

    BOOST_CHECK(!span.HasPeriod());
    span.SetPeriod(10_min);
    BOOST_CHECK(span.HasPeriod());
    BOOST_CHECK_EQUAL(ToString(span), "10:00-12:00/10");
  }
}

BOOST_AUTO_TEST_CASE(OpeningHours_TestNthEntry)
{
  using namespace osmoh;

  {
    NthEntry entry;
    BOOST_CHECK(entry.IsEmpty());
    BOOST_CHECK(!entry.HasStart());
    BOOST_CHECK(!entry.HasEnd());
    BOOST_CHECK_EQUAL(ToString(entry), "");

    entry.SetStart(NthEntry::Nth::Third);
    BOOST_CHECK(!entry.IsEmpty());
    BOOST_CHECK(entry.HasStart());
    BOOST_CHECK(!entry.HasEnd());
    BOOST_CHECK_EQUAL(ToString(entry), "3");

    entry.SetEnd(NthEntry::Nth::Fifth);
    BOOST_CHECK(!entry.IsEmpty());
    BOOST_CHECK(entry.HasStart());
    BOOST_CHECK(entry.HasEnd());
    BOOST_CHECK_EQUAL(ToString(entry), "3-5");

    entry.SetStart(NthEntry::Nth::None);
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

    NthEntry entry;
    entry.SetStart(NthEntry::NthEntry::Nth::First);
    range.AddNth(entry);
    BOOST_CHECK(range.HasNth());
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
    auto const rule = "06:00";
    auto const parsedUnparsed = ParseAndUnparse<osmoh::TTimespans>(rule);
    BOOST_CHECK_EQUAL(parsedUnparsed, rule);
  }
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
    auto const rule = "dusk";
    auto const parsedUnparsed = ParseAndUnparse<osmoh::TTimespans>(rule);
  }
  {
    auto const rule = "dawn+";
    auto const parsedUnparsed = ParseAndUnparse<osmoh::TTimespans>(rule);
  }
  {
    auto const rule = "sunrise-sunset";
    auto const parsedUnparsed = ParseAndUnparse<osmoh::TTimespans>(rule);
  }
  {
    auto const rule = "(dusk-12:12)";
    auto const parsedUnparsed = ParseAndUnparse<osmoh::TTimespans>(rule);
  }
  {
    auto const rule = "(dusk-12:12)+";
    auto const parsedUnparsed = ParseAndUnparse<osmoh::TTimespans>(rule);
  }
  {
    auto const rule = "(dusk-12:12)-sunset";
    auto const parsedUnparsed = ParseAndUnparse<osmoh::TTimespans>(rule);
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
  {
    auto const rule = "1995";
    auto const parsedUnparsed = ParseAndUnparse<osmoh::TYearRanges>(rule);
    BOOST_CHECK_EQUAL(parsedUnparsed, rule);
  }
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
    auto const rule = "24/7";
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
    auto const rule = "06:00-02:00/21:03, 18:15";
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
    auto const rule = "Su-Th (sunset-24:00); Fr-Sa (sunrise+12:12)";
    auto const parsedUnparsed = ParseAndUnparse<osmoh::TRuleSequences>(rule);
    BOOST_CHECK_EQUAL(parsedUnparsed, rule);
  }
  {
    auto const rule = "2010 Apr 01-30: Mo-Su 17:00-24:00";
    auto const parsedUnparsed = ParseAndUnparse<osmoh::TRuleSequences>(rule);
    BOOST_CHECK_EQUAL(parsedUnparsed, rule);
  }
}

BOOST_AUTO_TEST_CASE(OpenigHours_TestIsActive)
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
  {
    TYearRanges ranges;
    BOOST_CHECK(Parse("2011", ranges));

    std::tm time;
    auto const fmt = "%Y";
    BOOST_CHECK(GetTimeTuple("2011", fmt, time));
    BOOST_CHECK(IsActive(ranges[0], time));

    BOOST_CHECK(GetTimeTuple("2012", fmt, time));
    BOOST_CHECK(!IsActive(ranges[0], time));
  }
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
    BOOST_CHECK(Parse("Mo-We 17:00-18:00, Th,Fr 15:00-16:00", rules));

    std::tm time{};
    auto const fmt = "%w %H:%M";
    BOOST_CHECK(GetTimeTuple("2 17:35", fmt, time));
    BOOST_CHECK(IsActive(rules[0], time));

    BOOST_CHECK(GetTimeTuple("4 15:35", fmt, time));
    BOOST_CHECK(IsActive(rules[1], time));
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
    BOOST_CHECK(Parse("Apr-Sep Su 14:30-17:00", rules));

    std::tm time{};
    auto const fmt = "%m-%w %H:%M";
    BOOST_CHECK(GetTimeTuple("5-0 15:35", fmt, time));
    BOOST_CHECK(IsActive(rules[0], time));

    BOOST_CHECK(GetTimeTuple("5-1 15:35", fmt, time));
    BOOST_CHECK(!IsActive(rules[0], time));

    BOOST_CHECK(GetTimeTuple("10-0 15:35", fmt, time));
    BOOST_CHECK(!IsActive(rules[0], time));
  }
  {
    TRuleSequences rules;
    BOOST_CHECK(Parse("2010 Apr 01-30: Mo-Su 17:00-24:00", rules));

    std::tm time{};
    auto const fmt = "%Y-%m-%d-%w %H:%M";
    BOOST_CHECK(GetTimeTuple("2010-4-20-0 18:15", fmt, time));
    BOOST_CHECK(IsActive(rules[0], time));
  }
}

/// How to run:
/// 1. copy opening-count.lst to where the binary is
/// 2. run with --log_level=message
BOOST_AUTO_TEST_CASE(OpeningHours_CountFailed)
{
  std::ifstream datalist("opening-count.lst");
  BOOST_REQUIRE_MESSAGE(datalist.is_open(),
                        "Can't open ./opening-count.lst: " << std::strerror(errno));

  std::string line;

  size_t line_num = 0;
  size_t num_failed = 0;
  size_t num_total = 0;

  std::map<size_t, size_t> hist;

  while (std::getline(datalist, line))
  {
    size_t count = 1;
    std::string datastr;

    auto d = line.find('|');
    if (d == std::string::npos)
    {
      BOOST_WARN_MESSAGE((d != std::string::npos),
                         "Incorrect line " << line_num << " format: " << line);
      datastr = line;
    }
    else
    {
      count = std::stol(line.substr(0,d));
      datastr = line.substr(d+1);
    }

    line_num++;

    osmoh::TRuleSequences rules;
    auto const isParsed = Parse(datastr, rules);
    if (!isParsed) {
      num_failed += count;
      hist[count]++;
      BOOST_TEST_MESSAGE("-- " << count << " :[" << datastr << "]");
    }
    else if (!CompareNormalized(datastr, rules))
    {
      num_failed += count;
      hist[count]++;
      BOOST_TEST_MESSAGE("- " << count << " :[" << datastr << "]");
      BOOST_TEST_MESSAGE("+ " << count << " :[" << ToString(rules) << "]");
    }
    num_total += count;
  }

  BOOST_CHECK_MESSAGE((num_failed == 0),
                      "Failed " << num_failed <<
                      " of " << num_total <<
                      " (" << double(num_failed)/(double(num_total)/100) << "%)");

  std::stringstream desc_message;
  for (auto const & e : hist)
    desc_message << "Weight: " << e.first << " Count: " << e.second << std::endl;

  BOOST_TEST_MESSAGE(desc_message.str());
}
