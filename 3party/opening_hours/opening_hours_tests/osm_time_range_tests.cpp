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

bool GetTimeTuple(std::string const & strTime, std::tm & tm)
{
  std::stringstream sstr(strTime);
  sstr >> std::get_time(&tm, "%H:%M");
  return !sstr.fail();
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
                        //"\"Eintritt ins gesamte Haus frei\"; "
                        "Jan 01, Dec 24, Dec 25, easter -2 days+: closed" );
    auto const parsedUnparsed = ParseAndUnparse<osmoh::TRuleSequences>(rule);
    BOOST_CHECK_EQUAL(parsedUnparsed, rule);
  }
  {
    auto const rule = "Su-Th (sunset-24:00); Fr-Sa (sunrise+12:12)";
    auto const parsedUnparsed = ParseAndUnparse<osmoh::TRuleSequences>(rule);
    BOOST_CHECK_EQUAL(parsedUnparsed, rule);
  }
}

BOOST_AUTO_TEST_CASE(OpenigHours_TestIsActiveTime)
{
  using namespace osmoh;

  {
    Timespan span;
    span.SetStart(10_h + 15_min);
    span.SetEnd(13_h + 49_min);

    std::tm time;
    BOOST_CHECK(GetTimeTuple("12:41", time));
    BOOST_CHECK(IsActive(span, time));

    BOOST_CHECK(GetTimeTuple("10:15", time));
    BOOST_CHECK(IsActive(span, time));

    BOOST_CHECK(GetTimeTuple("10:14", time));
    BOOST_CHECK(!IsActive(span, time));

    BOOST_CHECK(GetTimeTuple("13:49", time));
    BOOST_CHECK(IsActive(span, time));

    BOOST_CHECK(GetTimeTuple("13:50", time));
    BOOST_CHECK(!IsActive(span, time));
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


// BOOST_AUTO_TEST_CASE(OpeningHours_TimeHit)
// {
//   {
//     OSMTimeRange oh = OSMTimeRange::FromString("06:13-15:00; 16:30+");
//     BOOST_CHECK(oh.IsValid());
//     BOOST_CHECK(oh.UpdateState("12-12-2013 7:00").IsOpen());
//     BOOST_CHECK(oh.UpdateState("12-12-2013 16:00").IsClosed());
//     BOOST_CHECK(oh.UpdateState("12-12-2013 20:00").IsOpen());
//   }
//   {
//     OSMTimeRange oh = OSMTimeRange::FromString("We-Fr; Mo[1,3] closed; Su[-1,-2] closed");
//     BOOST_CHECK(oh.IsValid());
//     BOOST_CHECK(oh.UpdateState("20-03-2015 18:00").IsOpen());
//     BOOST_CHECK(oh.UpdateState("17-03-2015 18:00").IsClosed());
//   }
//   {
//     OSMTimeRange oh = OSMTimeRange::FromString("We-Fr; Mo[1,3] +1 day closed; Su[-1,-2] -3 days closed");
//     BOOST_CHECK(oh.IsValid());
//     BOOST_CHECK(oh.UpdateState("20-03-2015 18:00").IsOpen());
//     BOOST_CHECK(oh.UpdateState("17-03-2015 18:00").IsClosed());
//   }
//   {
//     OSMTimeRange oh = OSMTimeRange::FromString("Mo-Su 14:30-17:00; Mo[1] closed; Su[-1] closed");
//     BOOST_CHECK(oh.IsValid());
//     BOOST_CHECK(oh.UpdateState("09-03-2015 16:00").IsOpen());
//     BOOST_CHECK(oh.UpdateState("02-03-2015 16:00").IsClosed());
//     BOOST_CHECK(oh.UpdateState("22-03-2015 16:00").IsOpen());
//     BOOST_CHECK(oh.UpdateState("29-03-2015 16:00").IsClosed());
//   }
//   {
//     OSMTimeRange oh = OSMTimeRange::FromString("PH,Tu-Su 10:00-18:00; Sa[1] 10:00-18:00 open \"Eintritt ins gesamte Haus frei\"; Jan 1,Dec 24,Dec 25,easter -2 days: closed");
//     BOOST_CHECK(oh.IsValid());
//     BOOST_CHECK(oh.UpdateState("03-03-2015 16:00").IsOpen());
//     BOOST_CHECK(oh.Comment().empty());
//     BOOST_CHECK(oh.UpdateState("07-03-2015 16:00").IsOpen());
//     BOOST_CHECK(oh.Comment().empty() == false);
//   }
//   {
//     OSMTimeRange oh = OSMTimeRange::FromString("Mo-Su 11:00+; Mo [1,3] off");
//     BOOST_CHECK(oh.IsValid());
//     BOOST_CHECK(oh.UpdateState("04-03-2015 16:00").IsOpen());
//     BOOST_CHECK(oh.UpdateState("09-03-2015 16:00").IsOpen());
//     BOOST_CHECK(oh.UpdateState("02-03-2015 16:00").IsClosed());
//     BOOST_CHECK(oh.UpdateState("16-03-2015 16:00").IsClosed());
//   }
//   {
//     OSMTimeRange oh = OSMTimeRange::FromString("08:00-16:00 open, 16:00-03:00 open \"public room\"");
//     BOOST_CHECK(oh.IsValid());
//     BOOST_CHECK(oh.UpdateState("01-03-2015 20:00").IsOpen());
//     BOOST_CHECK(oh.UpdateState("01-03-2015 20:00").Comment() == "public room");
//   }
//   {
//     OSMTimeRange oh = OSMTimeRange::FromString("9:00－02:00");
//     BOOST_CHECK(oh.IsValid());
//     BOOST_CHECK(oh.UpdateState("01-01-2000 07:00").IsClosed());
//     BOOST_CHECK(oh.UpdateState("01-01-2000 09:00").IsOpen());
//     BOOST_CHECK(oh.UpdateState("01-01-2000 12:00").IsOpen());
//     BOOST_CHECK(oh.UpdateState("01-01-2000 20:00").IsOpen());
//     BOOST_CHECK(oh.UpdateState("01-01-2000 24:00").IsOpen());
//     BOOST_CHECK(oh.UpdateState("01-01-2000 00:00").IsOpen());
//     BOOST_CHECK(oh.UpdateState("01-01-2000 01:00").IsOpen());
//     BOOST_CHECK(oh.UpdateState("01-01-2000 01:59").IsOpen());
//     BOOST_CHECK(oh.UpdateState("01-01-2000 02:00").IsClosed());
//   }
//   {
//     OSMTimeRange oh = OSMTimeRange::FromString("09:00am-19:00pm"); // hours > 12, ignore pm
//     BOOST_CHECK(oh.IsValid());
//     BOOST_CHECK(oh.UpdateState("01-01-2000 20:00").IsClosed());
//     BOOST_CHECK(oh.UpdateState("01-01-2000 8:00").IsClosed());
//     BOOST_CHECK(oh.UpdateState("01-01-2000 14:00").IsOpen());
//   }
//   {
//     OSMTimeRange oh = OSMTimeRange::FromString("09:00h-7:00 pm"); // symbols case
//     BOOST_CHECK(oh.IsValid());
//     BOOST_CHECK(oh.UpdateState("01-01-2000 20:00").IsClosed());
//     BOOST_CHECK(oh.UpdateState("01-01-2000 8:00").IsClosed());
//     BOOST_CHECK(oh.UpdateState("01-01-2000 14:00").IsOpen());
//   }
//   {
//     OSMTimeRange oh = OSMTimeRange::FromString("Mo-Fr: 11-19 Uhr;Sa: 10-18 Uhr");
//     BOOST_CHECK(oh.IsValid());
//     BOOST_CHECK_EQUAL(oh.UpdateState("08-03-2015 20:00").IsClosed(), true);
//     BOOST_CHECK_EQUAL(oh.UpdateState("18-03-2015 12:00").IsClosed(), false);
//     BOOST_CHECK_EQUAL(oh.UpdateState("16-03-2015 10:00").IsOpen(), false);
//     BOOST_CHECK(oh.UpdateState("14-03-2015 10:00").IsOpen());
//     BOOST_CHECK(oh.UpdateState("16-03-2015 11:00").IsOpen());
//     BOOST_CHECK(oh.UpdateState("01-01-2000 14:00").IsOpen());
//   }
//   {
//     OSMTimeRange oh = OSMTimeRange::FromString("Apr 9-19");
//     BOOST_CHECK(oh.IsValid());
//     BOOST_CHECK_EQUAL(oh.UpdateState("01-01-2000 20:00").IsClosed(), false);
//     BOOST_CHECK_EQUAL(oh.UpdateState("01-01-2000 8:00").IsClosed(), false);
//     BOOST_CHECK(oh.UpdateState("01-01-2000 14:00").IsOpen());
//   }
//   {
//     OSMTimeRange oh = OSMTimeRange::FromString("9-19"); // it's no time, it's days of month
//     BOOST_CHECK(oh.IsValid());
//     BOOST_CHECK_EQUAL(oh.UpdateState("01-01-2000 20:00").IsClosed(), false);
//     BOOST_CHECK_EQUAL(oh.UpdateState("01-01-2000 8:00").IsClosed(), false);
//     BOOST_CHECK(oh.UpdateState("01-01-2000 14:00").IsOpen());
//   }
// }

// BOOST_AUTO_TEST_CASE(OpeningHours_StaticSet)
// {
//   {
//     // TODO(mgsergio) move validation from parsing
//     //  OSMTimeRange oh = OSMTimeRange::FromString("06:00-02:00/21:03");
//     //  BOOST_CHECK(oh.IsValid());
//     auto const rule = "06:00-02:00/21:03";
//     BOOST_CHECK_EQUAL(ParseAndUnparse<osmoh::parsing::time_domain>(rule), rule);
//   }

//   {
//     // BOOST_CHECK(test_hard<osmoh::parsing::time_domain>("06:00-09:00/03"));
//     //BOOST_CHECK(oh.IsValid());
//   }
//   {
//     OSMTimeRange oh = OSMTimeRange::FromString("06:00-07:00/03");
//     BOOST_CHECK(oh.IsValid() == false);
//   }
//   {
//     OSMTimeRange oh = OSMTimeRange::FromString("sunrise-sunset");
//     BOOST_CHECK(oh.IsValid());
//   }
//   {
//     OSMTimeRange oh = OSMTimeRange::FromString("Su-Th sunset-24:00, 04:00-sunrise; Fr-Sa sunset-sunrise");
//     BOOST_CHECK(oh.IsValid());
//   }
//   {
//     OSMTimeRange oh = OSMTimeRange::FromString("Apr-Sep Su [1,3] 14:30-17:00");
//     BOOST_CHECK(oh.IsValid());
//   }
//   {
//     OSMTimeRange oh = OSMTimeRange::FromString("06:00+");
//     BOOST_CHECK(oh.IsValid());
//   }
//   {
//     OSMTimeRange oh = OSMTimeRange::FromString("06:00-07:00+");
//     BOOST_CHECK(oh.IsValid());
//   }
//   {
//     OSMTimeRange oh = OSMTimeRange::FromString("24/7");
//     BOOST_CHECK(oh.IsValid());
//   }
//   {
//     OSMTimeRange oh = OSMTimeRange::FromString("06:13-15:00");
//     BOOST_CHECK(oh.IsValid());
//   }
//   {
//     OSMTimeRange oh = OSMTimeRange::FromString("Mo-Su 08:00-23:00");
//     BOOST_CHECK(oh.IsValid());
//   }
//   {
//     OSMTimeRange oh = OSMTimeRange::FromString("(sunrise+02:00)-(sunset-04:12)");
//     BOOST_CHECK(oh.IsValid());
//   }
//   {
//     OSMTimeRange oh = OSMTimeRange::FromString("Mo-Sa; PH off");
//     BOOST_CHECK(oh.IsValid());
//   }
//   {
//     OSMTimeRange oh = OSMTimeRange::FromString("Jan-Mar 07:00-19:00;Apr-Sep 07:00-22:00;Oct-Dec 07:00-19:00");
//     BOOST_CHECK(oh.IsValid());
//   }
//   {
//     OSMTimeRange oh = OSMTimeRange::FromString("Mo closed");
//     BOOST_CHECK(oh.IsValid());
//   }
//   {
//     OSMTimeRange oh = OSMTimeRange::FromString("06:00-23:00 open \"Dining in\"");
//     BOOST_CHECK(oh.IsValid());
//   }
//   {
//     OSMTimeRange oh = OSMTimeRange::FromString("06:00-23:00 open \"Dining in\" || 00:00-24:00 open \"Drive-through\"");
//     BOOST_CHECK(oh.IsValid());
//   }
//   {
//     OSMTimeRange oh = OSMTimeRange::FromString("Tu-Th 20:00-03:00 open \"Club and bar\"; Fr-Sa 20:00-04:00 open \"Club and bar\" || Su-Mo 18:00-02:00 open \"bar\" || Tu-Th 18:00-03:00 open \"bar\" || Fr-Sa 18:00-04:00 open \"bar\"");
//     BOOST_CHECK(oh.IsValid());
//   }
//   {
//     OSMTimeRange oh = OSMTimeRange::FromString("09:00-21:00 \"call us\"");
//     BOOST_CHECK(oh.IsValid());
//   }
//   {
//     OSMTimeRange oh = OSMTimeRange::FromString("10:00-13:30,17:00-20:30");
//     BOOST_CHECK(oh.IsValid());
//   }
//   {
//     OSMTimeRange oh = OSMTimeRange::FromString("Apr-Sep: Mo-Fr 09:00-13:00,14:00-18:00; Apr-Sep: Sa 10:00-13:00");
//     BOOST_CHECK(oh.IsValid());
//   }
//   {
//     OSMTimeRange oh = OSMTimeRange::FromString("Mo,We,Th,Fr 12:00-18:00; Sa-Su 12:00-17:00");
//     BOOST_CHECK(oh.IsValid());
//   }
//   {
//     OSMTimeRange oh = OSMTimeRange::FromString("Su-Th 11:00-03:00, Fr-Sa 11:00-05:00");
//     BOOST_CHECK(oh.IsValid());
//   }
//   {
//     OSMTimeRange oh = OSMTimeRange::FromString("Mo-We 17:00-01:00, Th,Fr 15:00-01:00; PH off");
//     BOOST_CHECK(oh.IsValid());
//   }
//   {
//     OSMTimeRange oh = OSMTimeRange::FromString("Tu-Su 10:00-18:00, Mo 12:00-17:00");
//     BOOST_CHECK(oh.IsValid());
//   }
//   {
//     OSMTimeRange oh = OSMTimeRange::FromString("sunset-sunrise");
//     BOOST_CHECK(oh.IsValid());
//   }
//   {
//     OSMTimeRange oh = OSMTimeRange::FromString("9:00－22:00");
//     BOOST_CHECK(oh.IsValid());
//   }
//   {
//     OSMTimeRange oh = OSMTimeRange::FromString("jun 16-mar 14 sunrise-sunset");
//     BOOST_CHECK(oh.IsValid());
//   }
//   {
//     OSMTimeRange oh = OSMTimeRange::FromString("Sa-Su; PH");
//     BOOST_CHECK(oh.IsValid());
//   }
//   {
//     OSMTimeRange oh = OSMTimeRange::FromString("Su; PH");
//     BOOST_CHECK(oh.IsValid());
//   }
//   {
//     OSMTimeRange oh = OSMTimeRange::FromString("Mo-Sa; PH off");
//     BOOST_CHECK(oh.IsValid());
//   }
//   {
//     OSMTimeRange oh = OSMTimeRange::FromString("Mo-Sa; PH off");
//     BOOST_CHECK(oh.IsValid());
//   }
//   {
//     // TODO(mgsergio) Check if we need locale. // пн -> Пн
//     OSMTimeRange oh = OSMTimeRange::FromString("пн. — пт.: 09:00 — 21:00; сб.: 09:00 — 19:00");
//     BOOST_CHECK(oh.IsValid());
//   }
//   {
//     OSMTimeRange oh = OSMTimeRange::FromString("May 15-Sep 23 10:00-18:00; Sep 24 - May 14 \"by appointment\"");
//     BOOST_CHECK(oh.IsValid());
//   }
//   {
//     OSMTimeRange oh = OSMTimeRange::FromString("May-Aug: Mo-Sa 14:30-Sunset; Su 10:30-Sunset; Sep-Apr off");
//     BOOST_CHECK(oh.IsValid());
//   }
//   {
//     OSMTimeRange oh = OSMTimeRange::FromString("May-Oct; Nov-Apr off");
//     BOOST_CHECK(oh.IsValid());
//   }
//   {
//     OSMTimeRange oh = OSMTimeRange::FromString("Fr-Sa");
//     BOOST_CHECK(oh.IsValid());
//   }
//   {
//     OSMTimeRange oh = OSMTimeRange::FromString("Apr 01-Oct 03: Mo-Th 07:00-20:00; Apr 01-Oct 03: Fr-Su 07:00-21:00");
//     BOOST_CHECK(oh.IsValid());
//   }
//   {
//     OSMTimeRange oh = OSMTimeRange::FromString("Apr 01-Oct 14 07:00-13:00, 15:00-22:00");
//     BOOST_CHECK(oh.IsValid());
//   }
//   {
//     OSMTimeRange oh = OSMTimeRange::FromString("06:00-08:30; 15:30-16:30");
//     BOOST_CHECK(oh.IsValid());
//   }
//   {
//     OSMTimeRange oh = OSMTimeRange::FromString("Apr-Sep: sunrise-sunset; Dec 1-20: dusk-dawn off");
//     BOOST_CHECK(oh.IsValid());
//   }
//   {
//     OSMTimeRange oh = OSMTimeRange::FromString("Apr-Sep: sunrise-sunset; Jan 1 off; Dec 25-26 off");
//     BOOST_CHECK(oh.IsValid());
//   }
//   {
//     OSMTimeRange oh = OSMTimeRange::FromString("Apr-Sep: sunrise-sunset; Jan 1: off; Dec 25-26: off");
//     BOOST_CHECK(oh.IsValid());
//   }
//   {
//     OSMTimeRange oh = OSMTimeRange::FromString("Mo-Fr: 09:00-18:00");
//     BOOST_CHECK(oh.IsValid());
//   }
//   {
//     OSMTimeRange oh = OSMTimeRange::FromString("Apr-Sep sunrise-sunset; Dec 1-20 dusk-dawn off");
//     BOOST_CHECK(oh.IsValid());
//   }
//   {
//     OSMTimeRange oh = OSMTimeRange::FromString("Mo off; Tu-Sa 09:30-19:00; Su 10:00-14:30; Jan 1 off; Dec 25-26 off");
//     BOOST_CHECK(oh.IsValid());
//   }
//   {
//     OSMTimeRange oh = OSMTimeRange::FromString("Mo-Th 08:00-19:00, Fr 08:00-17:00, Su[-2] 08:00-15:00");
//     BOOST_CHECK(oh.IsValid());
//   }
//   {
//     OSMTimeRange oh = OSMTimeRange::FromString("Lu-Di 10:00-18:00"); // symbols case
//     BOOST_CHECK(oh.IsValid());
//   }
//   {
//     OSMTimeRange oh = OSMTimeRange::FromString("sunset-sunrise; Sa 09:00-18:00;TU,su,pH OFF"); // symbols case
//     BOOST_CHECK(oh.IsValid());
//   }
//   {
//     OSMTimeRange oh = OSMTimeRange::FromString("09:00h-19:00 h"); // symbols case
//     BOOST_CHECK(oh.IsValid());
//   }
//   {
//     OSMTimeRange oh = OSMTimeRange::FromString("09:00h to 19:00 h"); // symbols case
//     BOOST_CHECK(oh.IsValid());
//   }
//   {
//     OSMTimeRange oh = OSMTimeRange::FromString("09h to 19:00"); // symbols case
//     BOOST_CHECK(oh.IsValid());
//   }
//   {
//     OSMTimeRange oh = OSMTimeRange::FromString("9h-19h"); // symbols case
//     BOOST_CHECK(oh.IsValid());
//   }
//   {
//     OSMTimeRange oh = OSMTimeRange::FromString("9am-9pm"); // symbols case
//     BOOST_CHECK(oh.IsValid());
//   }
//   {
//     OSMTimeRange oh = OSMTimeRange::FromString("09:00H-19:00 h"); // symbols case
//     BOOST_CHECK(oh.IsValid());
//   }
//   {
//     OSMTimeRange oh = OSMTimeRange::FromString("09:00am-19:00"); // symbols case
//     BOOST_CHECK(oh.IsValid());
//   }
//   {
//     OSMTimeRange oh = OSMTimeRange::FromString("09:00-19:00;Sa 09:00-18:00;Tu,Su,PH OFF"); // symbols case
//     BOOST_CHECK(oh.IsValid());
//   }
//   {
//     OSMTimeRange oh = OSMTimeRange::FromString("09:00-19:00;Sa 09:00-18:00;Tu,Su,ph Off"); // symbols case
//     BOOST_CHECK(oh.IsValid());
//   }
//   {
//     OSMTimeRange oh = OSMTimeRange::FromString("05:00 – 22:00"); // long dash instead minus
//     BOOST_CHECK(oh.IsValid());
//   }
//   {
//     OSMTimeRange oh = OSMTimeRange::FromString("05:00 - 22:00"); // minus
//     BOOST_CHECK(oh.IsValid());
//   }
//   {
//     OSMTimeRange oh = OSMTimeRange::FromString("09:00-20:00 open \"Bei schönem Wetter. Falls unklar kann angerufen werden\""); // charset
//     BOOST_CHECK(oh.IsValid());
//   }
//   {
//     OSMTimeRange oh = OSMTimeRange::FromString("09:00-22:00; Tu off; dec 31 off; Jan 1 off"); // symbols case
//     BOOST_CHECK(oh.IsValid());
//   }
//   {
//     OSMTimeRange oh = OSMTimeRange::FromString("9:00-22:00"); // leading zeros
//     BOOST_CHECK(oh.IsValid());
//   }
//   {
//     OSMTimeRange oh = OSMTimeRange::FromString("09:00-9:30"); // leading zeros
//     BOOST_CHECK(oh.IsValid());
//   }
//   {
//     OSMTimeRange oh = OSMTimeRange::FromString("Mo 08:00-11:00,14:00-17:00; Tu 08:00-11:00, 14:00-17:00; We 08:00-11:00; Th 08:00-11:00, 14:00-16:00; Fr 08:00-11:00");
//     BOOST_CHECK(oh.IsValid());
//   }
// }
