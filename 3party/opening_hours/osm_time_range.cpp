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

#include <chrono>
#include <iomanip>
#include <ios>
#include <vector>
#include <codecvt>

//#define BOOST_SPIRIT_DEBUG 1
#define BOOST_SPIRIT_USE_PHOENIX_V3
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/repository/include/qi_subrule.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_fusion.hpp>
#include <boost/spirit/include/phoenix_stl.hpp>
#include <boost/spirit/include/phoenix_statement.hpp>
#include <boost/spirit/include/phoenix_bind.hpp>
#include <boost/fusion/adapted/struct/adapt_struct.hpp>

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wshorten-64-to-32"
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>
#pragma clang diagnostic pop
#else
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>
#endif


namespace osmoh
{

  std::ostream & operator << (std::ostream & s, Time const & t)
  {
    bool event = (t.flags & Time::eSunrise) || (t.flags & Time::eSunset);
    if (event)
      s << ((t.flags & Time::eSunrise) ? "sunrise" : "sunset") << " (";
    std::ios_base::fmtflags sf = s.flags();
    if (t.flags & (Time::ePlus | Time::eMinus))
      s << ((t.flags & Time::ePlus) ? "+" : "-");
    if (t.flags & Time::eHours)
      s << std::setw(2) << std::setfill('0') << (int)t.hours;
    if (t.flags & Time::eMinutes)
      s << ":" << std::setw(2) << std::setfill('0') << (int)t.minutes;
    s.flags(sf);
    if (event)
      s << ")";
    return s;
  }

  std::ostream & operator << (std::ostream & s, TimeSpan const & span)
  {
    s << span.from;
    if (span.to.flags)
      s << '-' << span.to;
    if (span.flags == Time::ePlus)
      s << "...";
    if (span.flags == Time::eExt)
      s << '/' << span.period;

    return s;
  }

  std::ostream & operator << (std::ostream & s, Weekdays const & w)
  {
    static char const * wdays[] = {"Mo", "Tu", "We", "Th", "Fr", "Sa", "Su"};
    static uint8_t const kDaysInWeek = 7;
    static uint8_t const kWeeksInMonth = 5;

    for (size_t i = 0; i < kDaysInWeek; ++i)
    {
      if (w.weekdays & (1 << i))
      {
        if (w.weekdays & ((1 << i) - 1))
          s << ',';
        s << wdays[i];
      }
    }

    if (w.nth)
    {
      s << "[";

      uint8_t a = w.nth & 0xFF;
      for (size_t i = 0; i < kWeeksInMonth; ++i)
      {
        if (a & (1 << i))
        {
          if (a & ((1 << i) - 1))
            s << ',';
          s << (i + 1);
        }
      }

      a = (w.nth >> 8) & 0xFF;
      for (size_t i = 0; i < kWeeksInMonth; ++i)
      {
        if (a & (1 << i))
        {
          if (a & ((1 << i) - 1))
            s << ',';
          s << '-' << (i + 1);
        }
      }

      s << "]";
    }

    if (w.offset)
      s << ' ' << w.offset << " day(s)";
    return s;
  }

  std::ostream & operator << (std::ostream & s, State const & w)
  {
    static char const * st[] = {"unknown", "closed", "open"};
    s << ' ' << st[w.state] << " " << w.comment;
    return s;
  }

  std::ostream & operator << (std::ostream & s, TimeRule const & w)
  {
    for (auto const & e : w.weekdays)
      s << e;
    if (!w.weekdays.empty() && !w.timespan.empty())
      s << ' ';
    for (auto const & e : w.timespan)
      s << e;

    return s << w.state;
  }

  boost::posix_time::time_period make_time_period(boost::gregorian::date const & d, osmoh::TimeSpan const & ts)
  {
    using boost::posix_time::ptime;
    using boost::posix_time::hours;
    using boost::posix_time::minutes;
    using boost::posix_time::time_period;

    /// TODO(yershov@): Need create code for calculate real values
    ptime sunrise(d, hours(6));
    ptime sunset(d, hours(19));

    ptime t1, t2;

    if (ts.from.flags & osmoh::Time::eSunrise)
      t1 = sunrise;
    else if (ts.from.flags & osmoh::Time::eSunset)
      t1 = sunset;
    else
      t1 = ptime(d, hours((ts.from.flags & osmoh::Time::eHours) ? ts.from.hours : 0) + minutes((ts.from.flags & osmoh::Time::eMinutes) ? ts.from.minutes : 0));

    t2 = t1;

    if (ts.to.flags & osmoh::Time::eSunrise)
      t2 = sunrise;
    else if (ts.to.flags & osmoh::Time::eSunset)
      t2 = sunset;
    else
    {
      t2 = ptime(d, hours((ts.to.flags & osmoh::Time::eHours) ? ts.to.hours : 24) + minutes((ts.to.flags & osmoh::Time::eMinutes) ? ts.to.minutes : 0));
      if (t2 < t1)
        t2 += hours(24);
    }

    return time_period(t1, t2);
  }

} // namespace osmoh


BOOST_FUSION_ADAPT_STRUCT
(
 osmoh::Time,
 (uint8_t, hours)
 (uint8_t, minutes)
 (uint8_t, flags)
)

BOOST_FUSION_ADAPT_STRUCT
(
 osmoh::TimeSpan,
 (osmoh::Time, from)
 (osmoh::Time, to)
 (uint8_t, flags)
 (osmoh::Time, period)
)

BOOST_FUSION_ADAPT_STRUCT
(
 osmoh::Weekdays,
 (uint8_t, weekdays)
 (uint16_t, nth)
 (int32_t, offset)
)

BOOST_FUSION_ADAPT_STRUCT
(
 osmoh::State,
 (uint8_t, state)
 (std::string, comment)
)

BOOST_FUSION_ADAPT_STRUCT
(
 osmoh::TimeRule,
 (std::vector<osmoh::Weekdays>, weekdays)
 (std::vector<osmoh::TimeSpan>, timespan)
 (osmoh::State, state)
 (uint8_t, int_flags)
)

namespace
{

  namespace qi = boost::spirit::qi;
  namespace phx = boost::phoenix;
  namespace repo = boost::spirit::repository;

  namespace charset = boost::spirit::standard_wide;
  using space_type = charset::space_type;


  class test_impl
  {
  public:
    template <typename T>
    struct result { typedef void type; };

    template <typename Arg>
    void operator() (const Arg & a) const
    {
      std::cout << a << " \t(" << typeid(a).name() << ")" << std::endl;
    }
  };
  phx::function<test_impl> const test = test_impl();

  class dash_ : public qi::symbols<wchar_t>
  {
  public:
    dash_()
    {
      add
      (L"-")
      /* not standard */
      (L"–")(L"—")(L"－")(L"~")(L"～")(L"〜")(L"to")(L"às")(L"ás")(L"as")(L"a")(L"ate")(L"bis")
      ;
    }
  } dash;

  class event_ : public qi::symbols<wchar_t, uint8_t>
  {
  public:
    event_()
    {
      add
      (L"dawn", osmoh::Time::eSunrise)(L"sunrise", osmoh::Time::eSunrise)(L"sunset", osmoh::Time::eSunset)(L"dusk", osmoh::Time::eSunset)
      ;
    }
  } event;

  struct wdays_ : qi::symbols<wchar_t, unsigned>
  {
    wdays_()
    {
      add
      (L"mo", 0)(L"tu", 1)(L"we", 2)(L"th", 3)(L"fr", 4)(L"sa", 5)(L"su", 6) // en
      (L"mon", 0)(L"tue", 1)(L"wed", 2)(L"thu", 3)(L"fri", 4)(L"sat", 5)(L"sun", 6) // en
      (L"пн", 0)(L"вт", 1)(L"ср", 2)(L"чт", 3)(L"пт", 4)(L"сб", 5)(L"вс", 6) // ru
      (L"пн.", 0)(L"вт.", 1)(L"ср.", 2)(L"чт.", 3)(L"пт.", 4)(L"сб.", 5)(L"вс.", 6) // ru
      (L"lu", 0)(L"ma", 1)(L"me", 2)(L"je", 3)(L"ve", 4)(L"sa", 5)(L"di", 6) // fr
      (L"lu", 0)(L"ma", 1)(L"me", 2)(L"gi", 3)(L"ve", 4)(L"sa", 5)(L"do", 6) // it
      (L"lu", 0)(L"ma", 1)(L"mi", 2)(L"ju", 3)(L"vie", 4)(L"sá", 5)(L"do", 6) // sp
      (L"週一", 0)(L"週二", 1)(L"週三", 2)(L"週四", 3)(L"週五", 4)(L"週六", 5)(L"週日", 6) // ch traditional
      (L"senin", 0)(L"selasa", 1)(L"rabu", 2)(L"kamis", 3)(L"jum'at", 4)(L"sabtu", 5)(L"minggu", 6) // indonesian

      (L"wd", 2)

      ;
    }
  } wdays;

  struct month_ : qi::symbols<wchar_t, unsigned>
  {
    month_()
    {
      add
      (L"jan", 1)(L"feb", 2)(L"mar", 3)(L"apr",  4)(L"may",  5)(L"jun",  6)
      (L"jul", 7)(L"aug", 8)(L"sep", 9)(L"oct", 10)(L"nov", 11)(L"dec", 12)
      ;
    }
  } month;

  struct hours_ : qi::symbols<char, uint8_t>
  {
    hours_()
    {
      add
      ( "0",  0)( "1",  1)( "2",  2)( "3",  3)( "4",  4)( "5",  5)( "6",  6)( "7",  7)( "8",  8)( "9",  9) /* not standard */
      ("00",  0)("01",  1)("02",  2)("03",  3)("04",  4)("05",  5)("06",  6)("07",  7)("08",  8)("09",  9)
      ("10", 10)("11", 11)("12", 12)("13", 13)("14", 14)("15", 15)("16", 16)("17", 17)("18", 18)("19", 19)
      ("20", 20)("21", 21)("22", 22)("23", 23)("24", 24)
      ;
    }
  } hours;

  struct exthours_ : qi::symbols<char, uint8_t>
  {
    exthours_()
    {
      add
      ( "0",  0)( "1",  1)( "2",  2)( "3",  3)( "4",  4)( "5",  5)( "6",  6)( "7",  7)( "8",  8)( "9",  9) /* not standard */
      ("00",  0)("01",  1)("02",  2)("03",  3)("04",  4)("05",  5)("06",  6)("07",  7)("08",  8)("09",  9)
      ("10", 10)("11", 11)("12", 12)("13", 13)("14", 14)("15", 15)("16", 16)("17", 17)("18", 18)("19", 19)
      ("20", 20)("21", 21)("22", 22)("23", 23)("24", 24)("25", 25)("26", 26)("27", 27)("28", 28)("29", 29)
      ("30", 30)("31", 31)("32", 32)("33", 33)("34", 34)("35", 35)("36", 36)("37", 37)("38", 38)("39", 39)
      ("40", 40)("41", 41)("42", 42)("43", 43)("44", 44)("45", 45)("46", 46)("47", 47)("48", 48)
      ;
    }
  } exthours;

  struct minutes_ : qi::symbols<char, uint8_t>
  {
    minutes_()
    {
      add
      ( "0",  0)( "1",  1)( "2",  2)( "3",  3)( "4",  4)( "5",  5)( "6",  6)( "7",  7)( "8",  8)( "9",  9) /* not standard */
      ("00",  0)("01",  1)("02",  2)("03",  3)("04",  4)("05",  5)("06",  6)("07",  7)("08",  8)("09",  9)
      ("10", 10)("11", 11)("12", 12)("13", 13)("14", 14)("15", 15)("16", 16)("17", 17)("18", 18)("19", 19)
      ("20", 20)("21", 21)("22", 22)("23", 23)("24", 24)("25", 25)("26", 26)("27", 27)("28", 28)("29", 29)
      ("30", 30)("31", 31)("32", 32)("33", 33)("34", 34)("35", 35)("36", 36)("37", 37)("38", 38)("39", 39)
      ("40", 40)("41", 41)("42", 42)("43", 43)("44", 44)("45", 45)("46", 46)("47", 47)("48", 48)("49", 49)
      ("50", 50)("51", 51)("52", 52)("53", 53)("54", 54)("55", 55)("56", 56)("57", 57)("58", 58)("59", 59)
      ;
    }
  } minutes;

  struct weeknum_ : qi::symbols<char, unsigned>
  {
    weeknum_()
    {
      add       ( "1",  1)( "2",  2)( "3",  3)( "4",  4)( "5",  5)( "6",  6)( "7",  7)( "8",  8)( "9",  9)
                ("01",  1)("02",  2)("03",  3)("04",  4)("05",  5)("06",  6)("07",  7)("08",  8)("09",  9)
      ("10", 10)("11", 11)("12", 12)("13", 13)("14", 14)("15", 15)("16", 16)("17", 17)("18", 18)("19", 19)
      ("20", 20)("21", 21)("22", 22)("23", 23)("24", 24)("25", 25)("26", 26)("27", 27)("28", 28)("29", 29)
      ("30", 30)("31", 31)("32", 32)("33", 33)("34", 34)("35", 35)("36", 36)("37", 37)("38", 38)("39", 39)
      ("40", 40)("41", 41)("42", 42)("43", 43)("44", 44)("45", 45)("46", 46)("47", 47)("48", 48)("49", 49)
      ("50", 50)("51", 51)("52", 52)("53", 53)
      ;
    }
  } weeknum;

  struct daynum_ : qi::symbols<char, unsigned>
  {
    daynum_()
    {
      add       ("1",  1)("2",  2)("3",  3)("4",  4)("5",  5)("6",  6)("7",  7)("8",  8)("9",  9)
      ("01",  1)("02",  2)("03",  3)("04",  4)("05",  5)("06",  6)("07",  7)("08",  8)("09",  9)
      ("10", 10)("11", 11)("12", 12)("13", 13)("14", 14)("15", 15)("16", 16)("17", 17)("18", 18)("19", 19)
      ("20", 20)("21", 21)("22", 22)("23", 23)("24", 24)("25", 25)("26", 26)("27", 27)("28", 28)("29", 29)
      ("30", 30)("31", 31)
      ;
    }
  } daynum;

  template <class Iterator>
  class year_selector_parser : public qi::grammar<Iterator, space_type>
  {
  protected:
    qi::rule<Iterator, space_type> year;
    qi::rule<Iterator, space_type> year_range;
    qi::rule<Iterator, space_type> main;
  public:
    year_selector_parser() : year_selector_parser::base_type(main)
    {
      using qi::uint_;
      using qi::lit;
      using charset::char_;

      static const qi::int_parser<unsigned, 10, 4, 4> _4digit = {};

      year %= _4digit;
      year_range %= (year >> dash >> year >> '/' >> uint_)
        | (year >> dash >> year)
        | year >> char_('+')
        | year
      ;
      main %= year_range % ',';
    }
  };

  template <typename Iterator>
  class week_selector_parser : public qi::grammar<Iterator, space_type>
  {
  protected:
    qi::rule<Iterator, space_type> week;
    qi::rule<Iterator, space_type> year_range;
    qi::rule<Iterator, space_type> main;
  public:
    week_selector_parser() : week_selector_parser::base_type(main)
    {
      using qi::uint_;
      using qi::lit;
      using charset::char_;

      week %= (weeknum >> dash >> weeknum >> '/' >> uint_)
        | (weeknum >> dash >> weeknum)
        | weeknum
      ;

      main %= charset::no_case[lit("week")] >> week % ',';
    }
  };

  template <typename Iterator>
  class month_selector_parser : public qi::grammar<Iterator, space_type>
  {
  protected:
    qi::rule<Iterator, space_type> date;
    qi::rule<Iterator, space_type> day_offset;
    qi::rule<Iterator, space_type> date_with_offsets;
    qi::rule<Iterator, space_type> monthday_range;
    qi::rule<Iterator, space_type> month_range;
    qi::rule<Iterator, space_type> main;
  public:
    month_selector_parser() : month_selector_parser::base_type(main)
    {
      using qi::int_;
      using qi::lit;
      using qi::double_;
      using qi::lexeme;
      using charset::char_;

      static const qi::int_parser<unsigned, 10, 4, 4> year = {};

      day_offset %= (char_('+') | char_('-')) >> int_ >> charset::no_case[(lit("days") | lit("day"))];

      date %= charset::no_case[(-year >> month >> daynum)]
        | (-year >> charset::no_case[lit("easter")])
        | daynum >> !(lit(':') >> qi::digit)
      ;

      date_with_offsets %= date >> -((char_('+') | char_('-')) >> charset::no_case[wdays] >> qi::no_skip[qi::space]) >> -day_offset;

      monthday_range %= (date_with_offsets >> dash >> date_with_offsets)
        | (date_with_offsets >> '+')
        | date_with_offsets
        | charset::no_case[(-year >> month >> dash >> month >> '/' >> int_)]
        | charset::no_case[(-year >> month >> dash >> month)]
        | charset::no_case[(-year >> month)]
      ;

      month_range %= charset::no_case[(month >> dash >> month)]
        | charset::no_case[month]
      ;

      main %= (monthday_range % ',') | (month_range % ',');

      BOOST_SPIRIT_DEBUG_NODE(main);
      BOOST_SPIRIT_DEBUG_NODE(month_range);
      BOOST_SPIRIT_DEBUG_NODE(monthday_range);
      BOOST_SPIRIT_DEBUG_NODE(date_with_offsets);
      BOOST_SPIRIT_DEBUG_NODE(date);
      BOOST_SPIRIT_DEBUG_NODE(day_offset);

    }
  };


  template <typename Iterator>
  class weekday_selector_parser : public qi::grammar<Iterator, std::vector<osmoh::Weekdays>(), space_type>
  {
  protected:
    qi::rule<Iterator, uint8_t(), space_type> nth;
    qi::rule<Iterator, uint16_t(), space_type> nth_entry;
    qi::rule<Iterator, int32_t(), space_type, qi::locals<int8_t>> day_offset;
    qi::rule<Iterator, space_type> holyday;
    qi::rule<Iterator, space_type> holiday_sequence;
    qi::rule<Iterator, osmoh::Weekdays(), space_type> weekday_range;
    qi::rule<Iterator, std::vector<osmoh::Weekdays>(), space_type> weekday_sequence;
    qi::rule<Iterator, std::vector<osmoh::Weekdays>(), space_type> main;
  public:
    weekday_selector_parser() : weekday_selector_parser::base_type(main)
    {
      using qi::_a;
      using qi::_1;
      using qi::_2;
      using qi::_val;
      using qi::lit;
      using qi::ushort_;
      using boost::phoenix::at_c;

      nth %= ushort_(1) | ushort_(2) | ushort_(3) | ushort_(4) | ushort_(5);

      nth_entry = (nth >> dash >> nth) [_val |= ((2 << ((_2-1)-(_1-1))) - 1) << (_1-1)]
        | (lit('-') >> nth) [_val |= (0x0100 << (_1 - 1))]
        | nth [_val |= (1 << (_1 - 1))]
      ;

      day_offset = (lit('+')[_a = 1] | lit('-') [_a = -1]) >> ushort_[_val = _1*_a] >> charset::no_case[(lit(L"days") | lit(L"day"))];
      holyday %= (charset::no_case[lit(L"SH")] >> -day_offset) | charset::no_case[lit(L"PH")];
      holiday_sequence %= holyday % ',';
      weekday_range = (charset::no_case[wdays][at_c<0>(_val) |= (1<<_1)]
                       >> L'[' >> nth_entry[at_c<1>(_val) |= _1] % L','
                       >> L']' >> day_offset[at_c<2>(_val) = _1])
        | (charset::no_case[wdays][at_c<0>(_val) |= (1<<_1)] >> L'[' >> nth_entry[at_c<1>(_val) |= _1] % L',' >> L']')
        | charset::no_case[(wdays >> dash >> wdays)] [at_c<0>(_val) |= ((2 << ((_2)-(_1))) - 1) << (_1)]
        | charset::no_case[wdays][at_c<0>(_val) |= (1<<_1)]
      ;

      weekday_sequence %= (weekday_range % L',') >> !qi::no_skip[charset::alpha] >> -lit(L':');

      main = (holiday_sequence >> -lit(L',') >> weekday_sequence[_val = _1])
        | weekday_sequence[_val = _1] >> -(-lit(L',') >> holiday_sequence)
        | holiday_sequence
      ;

      BOOST_SPIRIT_DEBUG_NODE(main);
      BOOST_SPIRIT_DEBUG_NODE(weekday_sequence);
      BOOST_SPIRIT_DEBUG_NODE(weekday_range);
      BOOST_SPIRIT_DEBUG_NODE(holiday_sequence);

    }
  };

  template <typename Iterator>
  class time_selector_parser : public qi::grammar<Iterator, std::vector<osmoh::TimeSpan>(), space_type>
  {
  protected:
    qi::rule<Iterator, osmoh::Time(), space_type, qi::locals<uint8_t>> hour_minutes;
    qi::rule<Iterator, osmoh::Time(), space_type, qi::locals<uint8_t>> extended_hour_minutes;
    qi::rule<Iterator, osmoh::Time(), space_type> variable_time;
    qi::rule<Iterator, osmoh::Time(), space_type> extended_time;
    qi::rule<Iterator, osmoh::Time(), space_type> time;
    qi::rule<Iterator, osmoh::TimeSpan(), space_type> timespan;
    qi::rule<Iterator, std::vector<osmoh::TimeSpan>(), space_type> main;

    class validate_timespan_impl
    {
    public:
      template <typename T>
      struct result { typedef bool type; };

      bool operator() (osmoh::TimeSpan const & ts) const
      {
        using boost::posix_time::ptime;
        using boost::posix_time::time_duration;
        using boost::posix_time::hours;
        using boost::posix_time::minutes;
        using boost::posix_time::time_period;

        bool result = true;
        if (ts.period.flags)
        {
          time_period tp = osmoh::make_time_period(boost::gregorian::day_clock::local_day(), ts);
          result = (tp.length() >= time_duration(ts.period.hours, ts.period.minutes, 0 /* seconds */));
        }

        return result;
      }
    };

  public:
    time_selector_parser() : time_selector_parser::base_type(main)
    {
      using qi::int_;
      using qi::_1;
      using qi::_2;
      using qi::_3;
      using qi::_a;
      using qi::_val;
      using qi::lit;
      using qi::_pass;
      using charset::char_;
      using boost::phoenix::at_c;

      phx::function<validate_timespan_impl> const validate_timespan = validate_timespan_impl();

      hour_minutes = hours[at_c<0>(_val) = _1, at_c<2>(_val) |= osmoh::Time::eHours]
        || (((lit(':') | lit("：") | lit('.')) >> minutes[at_c<1>(_val) = _1,
                                                         at_c<2>(_val) |= osmoh::Time::eMinutes])
            ^ charset::no_case[lit('h') | lit("hs") | lit("hrs") | lit("uhr")]
            ^ (charset::no_case[lit("am")][_a = 0] | charset::no_case[lit("pm")][_a = 1])
                [phx::if_(at_c<0>(_val) <= 12)[at_c<0>(_val) += (12 * _a)]])
      ;

      extended_hour_minutes = exthours[at_c<0>(_val) = _1, at_c<2>(_val) |= osmoh::Time::eHours]
        || (((lit(':') | lit("：") | lit('.')) >> minutes[at_c<1>(_val) = _1,
                                                         at_c<2>(_val) |= osmoh::Time::eMinutes])
            ^ charset::no_case[lit('h') | lit("hs") | lit("hrs") | lit("uhr")]
            ^ (charset::no_case[lit("am")][_a = 0] | charset::no_case[lit("pm")][_a = 1])
                [phx::if_(at_c<0>(_val) <= 12)[at_c<0>(_val) += (12 * _a)]])
      ;

      variable_time =
         (lit('(')
          >> charset::no_case[event][at_c<2>(_val) |= _1]
           >> (
                 char_('+')[at_c<2>(_val) |= osmoh::Time::ePlus]
               | char_('-')[at_c<2>(_val) |= osmoh::Time::eMinus]
              )
           >> hour_minutes[at_c<2>(_1) |= at_c<2>(_val), _val = _1]
           >> lit(')')
         )
        | charset::no_case[event][at_c<2>(_val) |= _1]
      ;

      extended_time %= extended_hour_minutes | variable_time;

      time %= hour_minutes | variable_time;

      timespan =
          (time >> dash >> extended_time >> L'/' >> hour_minutes)
            [at_c<0>(_val) = _1, at_c<1>(_val) = _2, at_c<2>(_val) |= osmoh::Time::eExt,
             at_c<3>(_val) = _3]
        | (time >> dash >> extended_time >> L'/' >> minutes)
            [at_c<0>(_val) = _1, at_c<1>(_val) = _2, at_c<2>(_val) |= osmoh::Time::eExt,
             at_c<1>(at_c<3>(_val)) = _3, at_c<2>(at_c<3>(_val)) = osmoh::Time::eMinutes]
        | (time >> dash >> extended_time >> char_(L'+'))
            [at_c<0>(_val) = _1, at_c<1>(_val) = _2, at_c<2>(_val) |= osmoh::Time::ePlus]
        | (time >> dash >> extended_time)
            [at_c<0>(_val) = _1, at_c<1>(_val) = _2]
        | (time >> char_(L'+'))
            [at_c<0>(_val) = _1, at_c<2>(_val) |= osmoh::Time::ePlus]
        | time [at_c<0>(_val) = _1]
      ;

      main %= timespan[_pass = validate_timespan(_1)] % ',';

      BOOST_SPIRIT_DEBUG_NODE(main);
      BOOST_SPIRIT_DEBUG_NODE(timespan);
      BOOST_SPIRIT_DEBUG_NODE(time);
      BOOST_SPIRIT_DEBUG_NODE(extended_time);
      BOOST_SPIRIT_DEBUG_NODE(variable_time);
      BOOST_SPIRIT_DEBUG_NODE(extended_hour_minutes);
    }
  };

  template <typename Iterator>
  class selectors_parser : public qi::grammar<Iterator, osmoh::TimeRule(), space_type>
  {
  protected:
    weekday_selector_parser<Iterator> weekday_selector;
    time_selector_parser<Iterator> time_selector;
    year_selector_parser<Iterator> year_selector;
    month_selector_parser<Iterator> month_selector;
    week_selector_parser<Iterator> week_selector;

    qi::rule<Iterator, std::string(), space_type> comment;
    qi::rule<Iterator, osmoh::TimeRule(), space_type> small_range_selectors;
    qi::rule<Iterator, space_type> wide_range_selectors;
    qi::rule<Iterator, osmoh::TimeRule(), space_type> main;
  public:
    selectors_parser() : selectors_parser::base_type(main)
    {
      using qi::_1;
      using qi::_val;
      using qi::lit;
      using qi::lexeme;
      using charset::char_;
      using boost::phoenix::at_c;
      using osmoh::State;

      comment %= lexeme['"' >> +(char_ - '"') >> '"'];
      wide_range_selectors = -year_selector >> -month_selector >> -week_selector >> -lit(':') | (comment >> ':');
      small_range_selectors = -weekday_selector[at_c<0>(_val) = _1] >> -( lit("24/7") | time_selector[at_c<1>(_val) = _1]);

      main =
        (
           lit(L"24/7")
         | lit(L"24時間営業")
         | lit(L"7/24")
         | lit(L"24時間")
         | charset::no_case[lit(L"daily 24/7")]
         | charset::no_case[lit(L"24 hours")]
         | charset::no_case[lit(L"24 horas")]
         | charset::no_case[lit(L"круглосуточно")]
         | charset::no_case[lit(L"24 часа")]
         | charset::no_case[lit(L"24 hrs")]
         | charset::no_case[lit(L"nonstop")]
         | charset::no_case[lit(L"24hrs")]
         | charset::no_case[lit(L"open 24 hours")]
         | charset::no_case[lit(L"24 stunden")]
         )[at_c<0>(at_c<2>(_val)) = State::eOpen]
        | (-wide_range_selectors >> small_range_selectors[_val = _1, at_c<0>(at_c<2>(_val)) = State::eOpen])
      ;
      BOOST_SPIRIT_DEBUG_NODE(main);
      BOOST_SPIRIT_DEBUG_NODE(small_range_selectors);
      BOOST_SPIRIT_DEBUG_NODE(wide_range_selectors);
    }
  };

  template <typename Iterator>
  class time_domain_parser : public qi::grammar<Iterator, std::vector<osmoh::TimeRule>(), space_type, qi::locals<qi::rule<Iterator, space_type>*>>
  {
  protected:
    selectors_parser<Iterator> selector_sequence;

    qi::rule<Iterator, std::string(), space_type> comment;
    qi::rule<Iterator, space_type> separator;
    qi::rule<Iterator, space_type> base_separator;
    qi::rule<Iterator, osmoh::TimeRule(), space_type> rule_sequence;
    qi::rule<Iterator, osmoh::State(), space_type> rule_modifier;
    qi::rule<Iterator, std::vector<osmoh::TimeRule>(), space_type, qi::locals<qi::rule<Iterator, space_type>*>> main;

  public:
    time_domain_parser() : time_domain_parser::base_type(main)
    {
      using qi::lit;
      using qi::lexeme;
      using qi::_1;
      using qi::_a;
      using qi::_val;
      using charset::char_;
      using boost::phoenix::at_c;
      using qi::lazy;
      using qi::eps;
      using osmoh::State;

      comment %= lexeme['"' >> +(char_ - '"') >> '"'] | lexeme['(' >> +(char_ - ')') >> ')'];
      base_separator = lit(';') | lit("||");
      separator = lit(';') | lit("||") | lit(',');

      rule_modifier =
          (charset::no_case[lit("open")][at_c<0>(_val) = State::eOpen] >> -comment[at_c<1>(_val) = _1])
        | ((charset::no_case[lit("closed") | lit("off")])[at_c<0>(_val) = State::eClosed] >> -comment[at_c<1>(_val) = _1])
        | (charset::no_case[lit("unknown")][at_c<0>(_val) = State::eUnknown] >> -comment[at_c<1>(_val) = _1])
        | comment[at_c<0>(_val) = State::eUnknown, at_c<1>(_val) = _1]
      ;

      rule_sequence = selector_sequence[_val = _1]
        >> -rule_modifier[at_c<2>(_val) = _1, at_c<3>(_val) = 1];

      main %= -(lit("opening_hours") >> lit('='))
        >> rule_sequence[_a = phx::val(&base_separator),
                              phx::if_(at_c<3>(_1) || phx::size(at_c<1>(_1)))[_a = phx::val(&separator)]] % lazy(*_a);

      BOOST_SPIRIT_DEBUG_NODE(main);
      BOOST_SPIRIT_DEBUG_NODE(rule_sequence);
      BOOST_SPIRIT_DEBUG_NODE(rule_modifier);
    }
  };

  template <typename Iterator>
  bool parse_timerange(Iterator first, Iterator last, std::vector<osmoh::TimeRule> & context)
  {
    using qi::double_;
    using qi::phrase_parse;
    using charset::space;

    time_domain_parser<Iterator> time_domain;

    bool r = phrase_parse(
                          first,       /* start iterator */
                          last,        /* end iterator */
                          time_domain, /* the parser */
                          space,       /* the skip-parser */
                          context      /* result storage */
                          );

    if (first != last) // fail if we did not get a full match
      return false;
    return r;
  }

  bool check_weekday(osmoh::Weekdays const & wd, boost::gregorian::date const & d)
  {
    using namespace boost::gregorian;

    bool hit = false;
    typedef nth_day_of_the_week_in_month nth_dow;
    if (wd.nth)
    {
      for (uint8_t i = 0; (wd.weekdays & (0xFF ^ ((1 << i) - 1))); ++i)
      {
        if (!(wd.weekdays & (1 << i)))
          continue;

        uint8_t a = wd.nth & 0xFF;
        for (size_t j = 0; (a & (0xFF ^ ((1 << j) - 1))); ++j)
        {
          if (a & (1 << j))
          {
            nth_dow ndm(nth_dow::week_num(j + 1), nth_dow::day_of_week_type((i + 1 == 7) ? 0 : (i + 1)), d.month());
            hit |= (d == ndm.get_date(d.year()));
          }
        }
        a = (wd.nth >> 8) & 0xFF;
        for (size_t j = 0; (a & (0xFF ^ ((1 << j) - 1))); ++j)
        {
          if (a & (1 << j))
          {
            last_day_of_the_week_in_month lwdm(nth_dow::day_of_week_type((i + 1 == 7) ? 0 : (i + 1)), d.month());
            hit |= (d == ((lwdm.get_date(d.year()) - weeks(j)) + days(wd.offset)));
          }
        }
      }
    }
    else
    {
      for (uint8_t i = 0; (wd.weekdays & (0xFF ^ ((1 << i) - 1))); ++i)
      {
        if (!(wd.weekdays & (1 << i)))
          continue;
        hit |= (d.day_of_week() == ((i + 1 == 7) ? 0 : (i + 1)));
      }
    }
    /* very useful in debug */
//    std::cout << d.day_of_week() << " " <<  d << " --> " << wd << (hit ? " hit" : " miss") << std::endl;
    return hit;
  }

  bool check_timespan(osmoh::TimeSpan const &ts, boost::gregorian::date const & d, boost::posix_time::ptime const & p)
  {
    using boost::gregorian::days;
    using boost::posix_time::ptime;
    using boost::posix_time::hours;
    using boost::posix_time::minutes;
    using boost::posix_time::time_period;

    time_period tp1 = osmoh::make_time_period(d-days(1), ts);
    time_period tp2 = osmoh::make_time_period(d, ts);
    /* very useful in debug */
//    std::cout << ts << "\t" << tp1 << "(" << p << ")" << (tp1.contains(p) ? " hit" : " miss") << std::endl;
//    std::cout << ts << "\t" << tp2 << "(" << p << ")" << (tp2.contains(p) ? " hit" : " miss") << std::endl;
    return tp1.contains(p) || tp2.contains(p);
  }

  bool check_rule(osmoh::TimeRule const & r, std::tm const & stm, std::ostream * hitcontext = nullptr)
  {
    bool next = false;

    // check 24/7
    if (r.weekdays.empty() && r.timespan.empty() && r.state.state == osmoh::State::eOpen)
      return true;

    boost::gregorian::date date = boost::gregorian::date_from_tm(stm);
    boost::posix_time::ptime pt = boost::posix_time::ptime_from_tm(stm);

    next = r.weekdays.empty();
    for (auto const & wd : r.weekdays)
    {
      if (check_weekday(wd, date))
      {
        if (hitcontext)
          *hitcontext << wd << " ";
        next = true;
      }
    }
    if (!next)
      return next;

    next = r.timespan.empty();
    for (auto const & ts : r.timespan)
    {
      if (check_timespan(ts, date, pt))
      {
        if (hitcontext)
          *hitcontext << ts << " ";
        next = true;
      }
    }
    return next && !(r.timespan.empty() && r.weekdays.empty());
  }


} // anonymouse namespace

OSMTimeRange::OSMTimeRange(std::string const & rules)
: m_sourceString(rules)
, m_valid(false)
, m_state(osmoh::State::eUnknown)
{
  parse();
}

void OSMTimeRange::parse()
{
  std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter; // could not work on android
  std::wstring src = converter.from_bytes(m_sourceString); // m_sourceString should be wstring
  m_valid = parse_timerange(src.begin(), src.end(), m_rules);
}

OSMTimeRange & OSMTimeRange::operator () (time_t timestamp)
{
  std::tm stm = *localtime(&timestamp);

  osmoh::State::EState true_state[3][3] = {
    {osmoh::State::eUnknown, osmoh::State::eClosed, osmoh::State::eOpen},
    {osmoh::State::eClosed , osmoh::State::eClosed, osmoh::State::eOpen},
    {osmoh::State::eOpen   , osmoh::State::eClosed, osmoh::State::eOpen}
  };

  osmoh::State::EState false_state[3][3] = {
    {osmoh::State::eUnknown, osmoh::State::eOpen   , osmoh::State::eClosed},
    {osmoh::State::eClosed , osmoh::State::eClosed , osmoh::State::eClosed},
    {osmoh::State::eOpen   , osmoh::State::eOpen   , osmoh::State::eOpen}
  };

  m_state = osmoh::State::eUnknown;
  m_comment = std::string();

  for (auto const & el : m_rules)
  {
    bool hit = false;
    if ((hit = check_rule(el, stm)))
    {
      m_state = true_state[m_state][el.state.state];
      m_comment = el.state.comment;
    }
    else
    {
      m_state = false_state[m_state][el.state.state];
    }
    /* very useful in debug */
//    char const * st[] = {"unknown", "closed", "open"};
//    std::cout << "-[" << hit << "]-------------------[" << el << "]: " << st[m_state] << "--------------------" << std::endl;
  }
  return *this;
}

OSMTimeRange & OSMTimeRange::operator () (std::string const & timestr, char const * timefmt)
{
  std::tm when = {};
  std::stringstream ss(timestr);
  ss >> std::get_time(&when, timefmt);
  return this->operator()(std::mktime(&when));
}
