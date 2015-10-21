#pragma once

#include "osm_time_range.hpp"

//#define BOOST_SPIRIT_DEBUG 1
#define BOOST_SPIRIT_USE_PHOENIX_V3
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/repository/include/qi_subrule.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_fusion.hpp>
#include <boost/spirit/include/phoenix_stl.hpp>
#include <boost/spirit/include/phoenix_statement.hpp>
#include <boost/spirit/include/phoenix_bind.hpp>
#include <boost/spirit/include/phoenix_object.hpp>


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
namespace phx = boost::phoenix;

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

// inline boost::posix_time::time_period make_time_period(boost::gregorian::date const & d,
//                                                        osmoh::TimeSpan const & ts)
// {
//   using boost::posix_time::ptime;
//   using boost::posix_time::hours;
//   using boost::posix_time::minutes;
//   using boost::posix_time::time_period;

//   /// TODO(yershov@): Need create code for calculate real values
//   ptime sunrise(d, hours(6));
//   ptime sunset(d, hours(19));

//   ptime t1, t2;

//   if (ts.from.flags & osmoh::Time::eSunrise)
//     t1 = sunrise;
//   else if (ts.from.flags & osmoh::Time::eSunset)
//     t1 = sunset;
//   else
//     t1 = ptime(d, hours((ts.from.flags & osmoh::Time::eHours) ? ts.from.hours : 0) + minutes((ts.from.flags & osmoh::Time::eMinutes) ? ts.from.minutes : 0));

//   t2 = t1;

//   if (ts.to.flags & osmoh::Time::eSunrise)
//     t2 = sunrise;
//   else if (ts.to.flags & osmoh::Time::eSunset)
//     t2 = sunset;
//   else
//   {
//     t2 = ptime(d, hours((ts.to.flags & osmoh::Time::eHours) ? ts.to.hours : 24) + minutes((ts.to.flags & osmoh::Time::eMinutes) ? ts.to.minutes : 0));
//     if (t2 < t1)
//       t2 += hours(24);
//   }

//   return time_period(t1, t2);
// }

namespace parsing
{
namespace qi = boost::spirit::qi;
namespace repo = boost::spirit::repository;

namespace charset = boost::spirit::standard_wide;
using space_type = charset::space_type;

class dash_ : public qi::symbols<wchar_t>
{
 public:
  dash_()
  {
    add
        (L"-")
        /* not standard */
        // (L"–")(L"—")(L"－")(L"~")(L"～")(L"〜")(L"to")(L"às")(L"ás")(L"as")(L"a")(L"ate")(L"bis")
        ;
  }
} dash;

class event_ : public qi::symbols<char,  osmoh::Time::EEvent>
{
 public:
  event_()
  {
    add
        ("dawn", osmoh::Time::EEvent::eSunrise)
        ("sunrise", osmoh::Time::EEvent::eSunrise)
        ("sunset", osmoh::Time::EEvent::eSunset)
        ("dusk", osmoh::Time::EEvent::eSunset)
        ;
  }
} event;

struct wdays_ : qi::symbols<char, osmoh::WeekdayRange::EWeekday>
{
  wdays_()
  {
    add
        ("su", 1_day)("mo", 2_day)("tu", 3_day)("we", 4_day)("th", 5_day)("fr", 6_day)("sa", 7_day) // en
        // (L"mon", 0)(L"tue", 1)(L"wed", 2)(L"thu", 3)(L"fri", 4)(L"sat", 5)(L"sun", 6) // en
        // (L"пн", 0)(L"вт", 1)(L"ср", 2)(L"чт", 3)(L"пт", 4)(L"сб", 5)(L"вс", 6) // ru
        // (L"пн.", 0)(L"вт.", 1)(L"ср.", 2)(L"чт.", 3)(L"пт.", 4)(L"сб.", 5)(L"вс.", 6) // ru
        // (L"lu", 0)(L"ma", 1)(L"me", 2)(L"je", 3)(L"ve", 4)(L"sa", 5)(L"di", 6) // fr
        // (L"lu", 0)(L"ma", 1)(L"me", 2)(L"gi", 3)(L"ve", 4)(L"sa", 5)(L"do", 6) // it
        // (L"lu", 0)(L"ma", 1)(L"mi", 2)(L"ju", 3)(L"vie", 4)(L"sá", 5)(L"do", 6) // sp
        // (L"週一", 0)(L"週二", 1)(L"週三", 2)(L"週四", 3)(L"週五", 4)(L"週六", 5)(L"週日", 6) // ch traditional
        // (L"senin", 0)(L"selasa", 1)(L"rabu", 2)(L"kamis", 3)(L"jum'at", 4)(L"sabtu", 5)(L"minggu", 6) // indonesian

        // (L"wd", 2)

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

struct hours_ : qi::symbols<char, osmoh::Time::THours>
{
  hours_()
  {
    add
        // ( "0",  0)( "1",  1)( "2",  2)( "3",  3)( "4",  4)( "5",  5)( "6",  6)( "7",  7)( "8",  8)( "9",  9) /* not standard */
        ("00",  0_h)("01",  1_h)("02",  2_h)("03",  3_h)("04",  4_h)("05",  5_h)("06",  6_h)("07",  7_h)("08",  8_h)("09",  9_h)
        ("10", 10_h)("11", 11_h)("12", 12_h)("13", 13_h)("14", 14_h)("15", 15_h)("16", 16_h)("17", 17_h)("18", 18_h)("19", 19_h)
        ("20", 20_h)("21", 21_h)("22", 22_h)("23", 23_h)("24", 24_h)
        ;
  }
} hours;

struct exthours_ : qi::symbols<char, osmoh::Time::THours>
{
  exthours_()
  {
    add
        // ( "0",  0)( "1",  1)( "2",  2)( "3",  3)( "4",  4)( "5",  5)( "6",  6)( "7",  7)( "8",  8)( "9",  9) /* not standard */
        ("00",  0_h)("01",  1_h)("02",  2_h)("03",  3_h)("04",  4_h)("05",  5_h)("06",  6_h)("07",  7_h)("08",  8_h)("09",  9_h)
        ("10", 10_h)("11", 11_h)("12", 12_h)("13", 13_h)("14", 14_h)("15", 15_h)("16", 16_h)("17", 17_h)("18", 18_h)("19", 19_h)
        ("20", 20_h)("21", 21_h)("22", 22_h)("23", 23_h)("24", 24_h)("25", 25_h)("26", 26_h)("27", 27_h)("28", 28_h)("29", 29_h)
        ("30", 30_h)("31", 31_h)("32", 32_h)("33", 33_h)("34", 34_h)("35", 35_h)("36", 36_h)("37", 37_h)("38", 38_h)("39", 39_h)
        ("40", 40_h)("41", 41_h)("42", 42_h)("43", 43_h)("44", 44_h)("45", 45_h)("46", 46_h)("47", 47_h)("48", 48_h)
        ;
  }
} exthours;

struct minutes_ : qi::symbols<char, osmoh::Time::TMinutes>
{
  minutes_()
  {
    add
        // ( "0",  0)( "1",  1)( "2",  2)( "3",  3)( "4",  4)( "5",  5)( "6",  6)( "7",  7)( "8",  8)( "9",  9) /* not standard */
        ("00",  0_min)("01",  1_min)("02",  2_min)("03",  3_min)("04",  4_min)("05",  5_min)("06",  6_min)("07",  7_min)("08",  8_min)("09",  9_min)
        ("10", 10_min)("11", 11_min)("12", 12_min)("13", 13_min)("14", 14_min)("15", 15_min)("16", 16_min)("17", 17_min)("18", 18_min)("19", 19_min)
        ("20", 20_min)("21", 21_min)("22", 22_min)("23", 23_min)("24", 24_min)("25", 25_min)("26", 26_min)("27", 27_min)("28", 28_min)("29", 29_min)
        ("30", 30_min)("31", 31_min)("32", 32_min)("33", 33_min)("34", 34_min)("35", 35_min)("36", 36_min)("37", 37_min)("38", 38_min)("39", 39_min)
        ("40", 40_min)("41", 41_min)("42", 42_min)("43", 43_min)("44", 44_min)("45", 45_min)("46", 46_min)("47", 47_min)("48", 48_min)("49", 49_min)
        ("50", 50_min)("51", 51_min)("52", 52_min)("53", 53_min)("54", 54_min)("55", 55_min)("56", 56_min)("57", 57_min)("58", 58_min)("59", 59_min)
        ;
  }
} minutes;

struct weeknum_ : qi::symbols<char, unsigned>
{
  weeknum_()
  {
    add
        // ( "1",  1)( "2",  2)( "3",  3)( "4",  4)( "5",  5)( "6",  6)( "7",  7)( "8",  8)( "9",  9)
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
    add
        // ("1",  1)("2",  2)("3",  3)("4",  4)("5",  5)("6",  6)("7",  7)("8",  8)("9",  9)
        ("01",  1)("02",  2)("03",  3)("04",  4)("05",  5)("06",  6)("07",  7)("08",  8)("09",  9)
        ("10", 10)("11", 11)("12", 12)("13", 13)("14", 14)("15", 15)("16", 16)("17", 17)("18", 18)("19", 19)
        ("20", 20)("21", 21)("22", 22)("23", 23)("24", 24)("25", 25)("26", 26)("27", 27)("28", 28)("29", 29)
        ("30", 30)("31", 31)
        ;
  }
} daynum;

// template <class Iterator>
// class year_selector : public qi::grammar<Iterator, space_type>
// {
//  protected:
//   qi::rule<Iterator, space_type> year;
//   qi::rule<Iterator, space_type> year_range;
//   qi::rule<Iterator, space_type> main;
//  public:
//   year_selector() : year_selector::base_type(main)
//   {
//     using qi::uint_;
//     using qi::lit;
//     using charset::char_;

//     static const qi::int_parser<unsigned, 10, 4, 4> _4digit = {};

//     year %= _4digit;
//     year_range %= (year >> dash >> year >> '/' >> uint_)
//         | (year >> dash >> year)
//         | year >> char_('+')
//         | year
//         ;
//     main %= year_range % ',';
//   }
// };

// template <typename Iterator>
// class week_selector : public qi::grammar<Iterator, space_type>
// {
//  protected:
//   qi::rule<Iterator, space_type> week;
//   qi::rule<Iterator, space_type> year_range;
//   qi::rule<Iterator, space_type> main;
//  public:
//   week_selector() : week_selector::base_type(main)
//   {
//     using qi::uint_;
//     using qi::lit;
//     using charset::char_;

//     week %= (weeknum >> dash >> weeknum >> '/' >> uint_)
//         | (weeknum >> dash >> weeknum)
//         | weeknum
//         ;

//     main %= charset::no_case[lit("week")] >> week % ',';
//   }
// };

// template <typename Iterator>
// class month_selector : public qi::grammar<Iterator, space_type>
// {
//  protected:
//   qi::rule<Iterator, space_type> date;
//   qi::rule<Iterator, space_type> day_offset;
//   qi::rule<Iterator, space_type> date_with_offsets;
//   qi::rule<Iterator, space_type> monthday_range;
//   qi::rule<Iterator, space_type> month_range;
//   qi::rule<Iterator, space_type> main;
//  public:
//   month_selector() : month_selector::base_type(main)
//   {
//     using qi::int_;
//     using qi::lit;
//     using qi::double_;
//     using qi::lexeme;
//     using charset::char_;

//     static const qi::int_parser<unsigned, 10, 4, 4> year = {};

//     day_offset %= (char_('+') | char_('-')) >> int_ >> charset::no_case[(lit("days") | lit("day"))];

//     date %= charset::no_case[(-year >> month >> daynum)]
//         | (-year >> charset::no_case[lit("easter")])
//         | daynum >> !(lit(':') >> qi::digit)
//         ;

//     date_with_offsets %= date >> -((char_('+') | char_('-')) >> charset::no_case[wdays] >> qi::no_skip[qi::space]) >> -day_offset;

//     monthday_range %= (date_with_offsets >> dash >> date_with_offsets)
//         | (date_with_offsets >> '+')
//         | date_with_offsets
//         | charset::no_case[(-year >> month >> dash >> month >> '/' >> int_)]
//         | charset::no_case[(-year >> month >> dash >> month)]
//         | charset::no_case[(-year >> month)]
//         ;

//     month_range %= charset::no_case[(month >> dash >> month)]
//         | charset::no_case[month]
//         ;

//     main %= (monthday_range % ',') | (month_range % ',');

//     BOOST_SPIRIT_DEBUG_NODE(main);
//     BOOST_SPIRIT_DEBUG_NODE(month_range);
//     BOOST_SPIRIT_DEBUG_NODE(monthday_range);
//     BOOST_SPIRIT_DEBUG_NODE(date_with_offsets);
//     BOOST_SPIRIT_DEBUG_NODE(date);
//     BOOST_SPIRIT_DEBUG_NODE(day_offset);

//   }
// };


template <typename Iterator>
//class weekday_selector : public qi::grammar<Iterator, osmoh::TWeekdayss(), space_type>
//class weekday_selector : public qi::grammar<Iterator, osmoh::THolidays(), space_type>
class weekday_selector : public qi::grammar<Iterator, osmoh::Weekdays(), space_type>
{
 protected:
  qi::rule<Iterator, osmoh::NthEntry::ENth(), space_type> nth;
  qi::rule<Iterator, osmoh::NthEntry(), space_type> nth_entry;
  qi::rule<Iterator, int32_t(), space_type, qi::locals<int8_t>> day_offset;
  qi::rule<Iterator, osmoh::WeekdayRange(), space_type> weekday_range;
  qi::rule<Iterator, osmoh::TWeekdayRanges(), space_type> weekday_sequence;
  qi::rule<Iterator, osmoh::Holiday(), space_type> holiday;
  qi::rule<Iterator, osmoh::THolidays(), space_type> holiday_sequence;
  qi::rule<Iterator, osmoh::Weekdays(), space_type> main;

 public:
  weekday_selector() : weekday_selector::base_type(main)
  {
    using qi::_a;
    using qi::_1;
    using qi::_2;
    using qi::_val;
    using qi::lit;
    using qi::ushort_;
    using boost::phoenix::bind;

    nth = ushort_(1)[_val = osmoh::NthEntry::ENth::First]
        | ushort_(2) [_val = osmoh::NthEntry::ENth::Second]
        | ushort_(3) [_val = osmoh::NthEntry::ENth::Third]
        | ushort_(4) [_val = osmoh::NthEntry::ENth::Fourth]
        | ushort_(5) [_val = osmoh::NthEntry::ENth::Fifth];

    nth_entry = (nth >> dash >> nth) [bind(&osmoh::NthEntry::SetStart, _val, _1),
                                      bind(&osmoh::NthEntry::SetEnd, _val, _2)]
        | (lit('-') >> nth) [bind(&osmoh::NthEntry::SetEnd, _val, _1)]
        | nth [bind(&osmoh::NthEntry::SetStart, _val, _1)]
        ;

    day_offset =
        (lit('+')[_a = 1] | lit('-') [_a = -1]) >>
        ushort_[_val = _1 * _a] >>
        charset::no_case[(lit(L"days") | lit(L"day"))];

    holiday = (charset::no_case[lit(L"SH")] [bind(&osmoh::Holiday::SetPlural, _val, false)]
               >> -day_offset               [bind(&osmoh::Holiday::SetOffset, _val, _1)])
        | charset::no_case[lit(L"PH")] [bind(&osmoh::Holiday::SetPlural, _val, true)]
        ;

    holiday_sequence %= (holiday % ',');

    weekday_range = (charset::no_case[wdays][bind(&osmoh::WeekdayRange::SetStart, _val, _1)]
                     >> L'[' >> nth_entry[bind(&osmoh::WeekdayRange::AddNth, _val, _1)] % L','
                     >> L']' >> day_offset[bind(&osmoh::WeekdayRange::SetOffset, _val, _1)])

        | (charset::no_case[wdays][bind(&osmoh::WeekdayRange::SetStart, _val, _1)]
           >> L'[' >> nth_entry[bind(&osmoh::WeekdayRange::AddNth, _val, _1)] % L','
           >> L']')

        | charset::no_case[(wdays >> dash >> wdays)][bind(&osmoh::WeekdayRange::SetStart, _val, _1),
                                                     bind(&osmoh::WeekdayRange::SetEnd, _val, _2)]

        | charset::no_case[wdays][bind(&osmoh::WeekdayRange::SetStart, _val, _1)]
        ;

    weekday_sequence %= (weekday_range % L',') >> !qi::no_skip[charset::alpha] >> -lit(L':');

    main = (holiday_sequence >> -lit(L',') >> weekday_sequence)
           [bind(&osmoh::Weekdays::SetHolidays, _val, _1),
            bind(&osmoh::Weekdays::SetWeekdayRanges, _val, _2)]
        | holiday_sequence [bind(&osmoh::Weekdays::SetHolidays, _val, _1)]
        | weekday_sequence [bind(&osmoh::Weekdays::SetWeekdayRanges, _val, _1)]
        ;

    BOOST_SPIRIT_DEBUG_NODE(main);
    BOOST_SPIRIT_DEBUG_NODE(weekday_sequence);
    BOOST_SPIRIT_DEBUG_NODE(weekday_range);
    BOOST_SPIRIT_DEBUG_NODE(holiday_sequence);
  }
};

template <typename Iterator>
class time_selector : public qi::grammar<Iterator, osmoh::TTimespans(), space_type>
{
 protected:
  qi::rule<Iterator, osmoh::Time(), space_type> hour_minutes;
  qi::rule<Iterator, osmoh::Time(), space_type> extended_hour_minutes;
  qi::rule<Iterator, osmoh::Time(), space_type> variable_time;
  qi::rule<Iterator, osmoh::Time(), space_type> extended_time;
  qi::rule<Iterator, osmoh::Time(), space_type> time;
  qi::rule<Iterator, osmoh::Timespan(), space_type> timespan;
  qi::rule<Iterator, osmoh::TTimespans(), space_type> main;

  // class validate_timespan_impl
  // {
  //  public:
  //   template <typename T>
  //   struct result { typedef bool type; };

  //   bool operator() (osmoh::TimeSpan const & ts) const
  //   {
  //     using boost::posix_time::ptime;
  //     using boost::posix_time::time_duration;
  //     using boost::posix_time::hours;
  //     using boost::posix_time::minutes;
  //     using boost::posix_time::time_period;

  //     bool result = true;
  //     if (ts.period.flags)
  //     {
  //       time_period tp = osmoh::make_time_period(boost::gregorian::day_clock::local_day(), ts);
  //       result = (tp.length() >= time_duration(ts.period.hours, ts.period.minutes, 0 /* seconds */));
  //     }

  //     return result;
  //   }
  // };

 public:
  time_selector() : time_selector::base_type(main)
  {
    using qi::int_;
    using qi::_1;
    using qi::_2;
    using qi::_3;
    using qi::_a;
    using qi::_val;
    using qi::lit;
    // using qi::_pass;
    using charset::char_;
    using boost::phoenix::bind;
    using boost::phoenix::construct;

    // phx::function<validate_timespan_impl> const validate_timespan = validate_timespan_impl();

    hour_minutes = (hours >> lit(':') >> minutes)[bind(&osmoh::Time::SetHours, _val, _1),
                                                  bind(&osmoh::Time::SetMinutes, _val, _2)]
        ;

    extended_hour_minutes = (exthours >> lit(':') >> minutes)[bind(&osmoh::Time::SetHours, _val, _1),
                                                              bind(&osmoh::Time::SetMinutes, _val, _2)]
        ;

    variable_time =
        (lit('(')
         >> charset::no_case[event][bind(&osmoh::Time::SetEvent, _val, _1)]
         >> (char_('+') | char_('-')[_val = -_val])
         >> hour_minutes
         >> lit(')')
         )
         | charset::no_case[event][bind(&osmoh::Time::SetEvent, _val, _1)]
        ;

    extended_time %= extended_hour_minutes | variable_time;

    time %= hour_minutes | variable_time;

    timespan =
        (time >> dash >> extended_time >> L'/' >> hour_minutes)
        [bind(&osmoh::Timespan::SetStart, _val, _1),
         bind(&osmoh::Timespan::SetEnd, _val, _2),
         bind(&osmoh::Timespan::SetPeriod, _val, _3)]

        | (time >> dash >> extended_time >> L'/' >> minutes)
          [bind(&osmoh::Timespan::SetStart, _val, _1),
           bind(&osmoh::Timespan::SetEnd, _val, _2),
           bind(&osmoh::Timespan::SetPeriod, _val, _3)]

        | (time >> dash >> extended_time >> char_(L'+'))
          [bind(&osmoh::Timespan::SetStart, _val, _1),
           bind(&osmoh::Timespan::SetEnd, _val, _2),
           bind(&osmoh::Timespan::SetPlus, _val, true)]

        | (time >> dash >> extended_time)
          [bind(&osmoh::Timespan::SetStart, _val, _1),
           bind(&osmoh::Timespan::SetEnd, _val, _2)]

        | (time >> char_(L'+'))
          [bind(&osmoh::Timespan::SetStart, _val, _1),
           bind(&osmoh::Timespan::SetPlus, _val, true)]

        | time[bind(&osmoh::Timespan::SetStart, _val, _1)]
        ;

    // main %= timespan[_pass = validate_timespan(_1)] % ',';
    main %= timespan % ',';

    BOOST_SPIRIT_DEBUG_NODE(main);
    BOOST_SPIRIT_DEBUG_NODE(timespan);
    BOOST_SPIRIT_DEBUG_NODE(time);
    BOOST_SPIRIT_DEBUG_NODE(extended_time);
    BOOST_SPIRIT_DEBUG_NODE(variable_time);
    BOOST_SPIRIT_DEBUG_NODE(extended_hour_minutes);
  }
};

// template <typename Iterator>
// class selectors : public qi::grammar<Iterator, osmoh::TimeRule(), space_type>
// {
//  protected:
//   weekday_selector<Iterator> weekday_selector;
//   time_selector<Iterator> time_selector;
//   year_selector<Iterator> year_selector;
//   month_selector<Iterator> month_selector;
//   week_selector<Iterator> week_selector;

//   qi::rule<Iterator, std::string(), space_type> comment;
//   qi::rule<Iterator, osmoh::TimeRule(), space_type> small_range_selectors;
//   qi::rule<Iterator, space_type> wide_range_selectors;
//   qi::rule<Iterator, osmoh::TimeRule(), space_type> main;
//  public:
//   selectors() : selectors::base_type(main)
//   {
//     using qi::_1;
//     using qi::_val;
//     using qi::lit;
//     using qi::lexeme;
//     using charset::char_;
//     using boost::phoenix::at_c;
//     using osmoh::State;

//     comment %= lexeme['"' >> +(char_ - '"') >> '"'];
//     wide_range_selectors = -year_selector >> -month_selector >> -week_selector >> -lit(':') | (comment >> ':');
//     small_range_selectors = -weekday_selector[at_c<0>(_val) = _1] >> -( lit("24/7") | time_selector[at_c<1>(_val) = _1]);

//     main =
//         (
//             lit(L"24/7")
//             | lit(L"24時間営業")
//             | lit(L"7/24")
//             | lit(L"24時間")
//             | charset::no_case[lit(L"daily 24/7")]
//             | charset::no_case[lit(L"24 hours")]
//             | charset::no_case[lit(L"24 horas")]
//             | charset::no_case[lit(L"круглосуточно")]
//             | charset::no_case[lit(L"24 часа")]
//             | charset::no_case[lit(L"24 hrs")]
//             | charset::no_case[lit(L"nonstop")]
//             | charset::no_case[lit(L"24hrs")]
//             | charset::no_case[lit(L"open 24 hours")]
//             | charset::no_case[lit(L"24 stunden")]
//          )[at_c<0>(at_c<2>(_val)) = State::eOpen]
//         | (-wide_range_selectors >> small_range_selectors[_val = _1, at_c<0>(at_c<2>(_val)) = State::eOpen])
//         ;
//     BOOST_SPIRIT_DEBUG_NODE(main);
//     BOOST_SPIRIT_DEBUG_NODE(small_range_selectors);
//     BOOST_SPIRIT_DEBUG_NODE(wide_range_selectors);
//   }
// };

// template <typename Iterator>
// class time_domain : public qi::grammar<Iterator, osmoh::TTimeRules(), space_type, qi::locals<qi::rule<Iterator, space_type>*>>
// {
// protected:
//   selectors<Iterator> selector_sequence;

//   qi::rule<Iterator, std::string(), space_type> comment;
//   qi::rule<Iterator, space_type> separator;
//   qi::rule<Iterator, space_type> base_separator;
//   qi::rule<Iterator, osmoh::TimeRule(), space_type> rule_sequence;
//   qi::rule<Iterator, osmoh::State(), space_type> rule_modifier;
//   qi::rule<Iterator, osmoh::TTimeRules(), space_type, qi::locals<qi::rule<Iterator, space_type>*>> main;

// public:
//   time_domain() : time_domain::base_type(main)
//   {
//     using qi::lit;
//     using qi::lexeme;
//     using qi::_1;
//     using qi::_a;
//     using qi::_val;
//     using charset::char_;
//     using boost::phoenix::at_c;
//     using qi::lazy;
//     using qi::eps;
//     using osmoh::State;

//     comment %= lexeme['"' >> +(char_ - '"') >> '"'] | lexeme['(' >> +(char_ - ')') >> ')'];
//     base_separator = lit(';') | lit("||");
//     separator = lit(';') | lit("||") | lit(',');

//     rule_modifier =
//         (charset::no_case[lit("open")][at_c<0>(_val) = State::eOpen] >> -comment[at_c<1>(_val) = _1])
//         | ((charset::no_case[lit("closed") | lit("off")])[at_c<0>(_val) = State::eClosed] >> -comment[at_c<1>(_val) = _1])
//         | (charset::no_case[lit("unknown")][at_c<0>(_val) = State::eUnknown] >> -comment[at_c<1>(_val) = _1])
//         | comment[at_c<0>(_val) = State::eUnknown, at_c<1>(_val) = _1]
//         ;

//     rule_sequence = selector_sequence[_val = _1]
//         >> -rule_modifier[at_c<2>(_val) = _1, at_c<3>(_val) = 1];

//     main %= -(lit("opening_hours") >> lit('='))
//         >> rule_sequence[_a = phx::val(&base_separator),
//                          phx::if_(at_c<3>(_1) || phx::size(at_c<1>(_1)))[_a = phx::val(&separator)]] % lazy(*_a);

//     BOOST_SPIRIT_DEBUG_NODE(main);
//     BOOST_SPIRIT_DEBUG_NODE(rule_sequence);
//     BOOST_SPIRIT_DEBUG_NODE(rule_modifier);
//   }
// };

// template <typename Iterator>
// inline bool parse_timerange(Iterator first, Iterator last, osmoh::TTimeRules & context)
// {
//   using qi::phrase_parse;
//   using charset::space;

//   time_domain<Iterator> time_domain;

//   bool r = phrase_parse(
//       first,       /* start iterator */
//       last,        /* end iterator */
//       time_domain, /* the parser */
//       space,       /* the skip-parser */
//       context      /* result storage */
//                         );

//   if (first != last) // fail if we did not get a full match
//     return false;
//   return r;
// }
} // namespace parsing
} // namespace osmoh
