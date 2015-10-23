#pragma once

#include "osm_time_range.hpp"

// #define BOOST_SPIRIT_DEBUG 1
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

#include "osm_parsers_terminals.hpp"

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
namespace charset = boost::spirit::standard_wide;

using space_type = charset::space_type;

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

template <typename Iterator>
class month_selector : public qi::grammar<Iterator, TMonthdayRanges(), space_type>
{
 protected:
  qi::rule<Iterator, int32_t(), space_type, qi::locals<int32_t>> day_offset;
  qi::rule<Iterator, DateOffset(), space_type, qi::locals<bool>> date_offset;

  qi::rule<Iterator, MonthDay(), space_type> date_left;
  qi::rule<Iterator, MonthDay(), space_type> date_right;
  qi::rule<Iterator, MonthDay(), space_type> date_from;
  qi::rule<Iterator, MonthDay(), space_type> date_to;
  qi::rule<Iterator, MonthDay(), space_type> date_from_with_offset;
  qi::rule<Iterator, MonthDay(), space_type> date_to_with_offset;

  qi::rule<Iterator, MonthdayRange(), space_type> monthday_range;
  qi::rule<Iterator, TMonthdayRanges(), space_type> main;

 public:
  month_selector() : month_selector::base_type(main)
  {
    using qi::_1;
    using qi::_2;
    using qi::_3;
    using qi::_a;
    using qi::_val;
    using qi::int_;
    using qi::uint_;
    using qi::lit;
    using qi::double_;
    using qi::lexeme;
    using charset::char_;

    static const qi::int_parser<unsigned, 10, 4, 4> year = {};

    day_offset = ((lit('+')[_a = 1] | lit('-')[_a = -1]) >>
                  int_ >> charset::no_case[(lit("days") | lit("day"))]) [_val = _a * _1];

    date_offset = ((lit('+')[_a = true] | lit('-')[_a = false]) >> wdays >> day_offset)
                  [bind(&osmoh::DateOffset::SetWDayOffset, _val, _1),
                   bind(&osmoh::DateOffset::SetOffset, _val, _2),
                   bind(&osmoh::DateOffset::SetWDayOffsetPositive, _val, _a)]
        | ((lit('+')[_a = true] | lit('-') [_a = false]) >> wdays)
          [bind(&osmoh::DateOffset::SetWDayOffset, _val, _1),
           bind(&osmoh::DateOffset::SetWDayOffsetPositive, _val, _a)]
        | day_offset [bind(&osmoh::DateOffset::SetOffset, _val, _1)]
        ;

    date_left = (year >> charset::no_case[month]) [bind(&osmoh::MonthDay::SetYear, _val, _1),
                                                   bind(&osmoh::MonthDay::SetMonth, _val, _2)]

        | charset::no_case[month]                 [bind(&osmoh::MonthDay::SetMonth, _val, _1)]
        ;

    date_right = charset::no_case[month]          [bind(&osmoh::MonthDay::SetMonth, _val, _1)]
        ;

    date_from = (date_left >> (daynum >> !(lit(':') >> qi::digit)))
                [_val = _1, bind(&osmoh::MonthDay::SetDayNum, _val, _2)]
        | (year >> charset::no_case[lit("easter")]) [bind(&osmoh::MonthDay::SetYear, _val, _1),
                                                     bind(&osmoh::MonthDay::SetVariableDate, _val,
                                                          MonthDay::EVariableDate::Easter)]
        | charset::no_case[lit("easter")]           [bind(&osmoh::MonthDay::SetVariableDate, _val,
                                                          MonthDay::EVariableDate::Easter)]
        ;

    date_to = date_from                        [_val = _1]
        | (daynum >> !(lit(':') >> qi::digit)) [bind(&osmoh::MonthDay::SetDayNum, _val, _1)]
        ;

    date_from_with_offset = (date_from >> date_offset)
                            [_val = _1, bind(&osmoh::MonthDay::SetOffset, _val, _2)]
        | date_from         [_val = _1]
        ;

    date_to_with_offset = (date_to >> date_offset)
                          [_val = _1, bind(&osmoh::MonthDay::SetOffset, _val, _2)]
        | date_to         [_val = _1]
        ;

    monthday_range = (date_from_with_offset >> dash >> date_to_with_offset)
                     [bind(&osmoh::MonthdayRange::SetStart, _val, _1),
                      bind(&osmoh::MonthdayRange::SetEnd, _val, _2)]
        | (date_from_with_offset >> '+') [bind(&osmoh::MonthdayRange::SetStart, _val, _1),
                                          bind(&osmoh::MonthdayRange::SetPlus, _val, true)]
        | (date_left >> dash >> date_right >> '/' >> uint_)
          [bind(&osmoh::MonthdayRange::SetStart, _val, _1),
           bind(&osmoh::MonthdayRange::SetEnd, _val, _2),
           bind(&osmoh::MonthdayRange::SetPeriod, _val, _3)]
        | (date_left >> lit("-") >> date_right) [bind(&osmoh::MonthdayRange::SetStart, _val, _1),
                                                 bind(&osmoh::MonthdayRange::SetEnd, _val, _2)]
        | date_from [bind(&osmoh::MonthdayRange::SetStart, _val, _1)]
        | date_left [bind(&osmoh::MonthdayRange::SetStart, _val, _1)]
        ;

    main %= (monthday_range % ',');

    BOOST_SPIRIT_DEBUG_NODE(main);
    BOOST_SPIRIT_DEBUG_NODE(monthday_range);
    BOOST_SPIRIT_DEBUG_NODE(day_offset);
    BOOST_SPIRIT_DEBUG_NODE(date_offset);
    BOOST_SPIRIT_DEBUG_NODE(date_left);
    BOOST_SPIRIT_DEBUG_NODE(date_right);
    BOOST_SPIRIT_DEBUG_NODE(date_from);
    BOOST_SPIRIT_DEBUG_NODE(date_to);
    BOOST_SPIRIT_DEBUG_NODE(date_from_with_offset);
    BOOST_SPIRIT_DEBUG_NODE(date_to_with_offset);
  }
};


template <typename Iterator>
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
