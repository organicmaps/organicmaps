#pragma once

#include "opening_hours.hpp"

// #define BOOST_SPIRIT_DEBUG
#define BOOST_SPIRIT_USE_PHOENIX_V3
#include <boost/spirit/include/phoenix_bind.hpp>
#include <boost/spirit/include/phoenix_fusion.hpp>
#include <boost/spirit/include/phoenix_object.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_statement.hpp>
#include <boost/spirit/include/phoenix_stl.hpp>
#include <boost/spirit/include/qi.hpp>

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

#include "opening_hours_parsers_terminals.hpp"

namespace osmoh
{
namespace phx = boost::phoenix;

namespace parsing
{
namespace qi = boost::spirit::qi;
namespace charset = boost::spirit::standard_wide;

using space_type = charset::space_type;

template <class Iterator>
class year_selector : public qi::grammar<Iterator, osmoh::TYearRanges(), space_type>
{
protected:
  qi::rule<Iterator, osmoh::YearRange(), space_type> year_range;
  qi::rule<Iterator, osmoh::TYearRanges(), space_type> main;

public:
  year_selector() : year_selector::base_type(main)
  {
    using qi::uint_;
    using qi::lit;
    using qi::_1;
    using qi::_2;
    using qi::_3;
    using qi::_val;
    using osmoh::YearRange;

    static const qi::int_parser<unsigned, 10, 4, 4> year = {};

    year_range = (year >> dash >> year >> '/' >> uint_) [bind(&YearRange::SetStart, _val, _1),
                                                         bind(&YearRange::SetEnd, _val, _2),
                                                         bind(&YearRange::SetPeriod, _val, _3)]
        | (year >> dash >> year) [bind(&YearRange::SetStart, _val, _1),
                                  bind(&YearRange::SetEnd, _val, _2)]
        | (year >> lit('+'))     [bind(&YearRange::SetStart, _val, _1),
                                  bind(&YearRange::SetPlus, _val, true)]
        | year                   [bind(&YearRange::SetStart, _val, _1)]
        ;

    main %= (year_range % ',');
  }
};

template <typename Iterator>
class week_selector : public qi::grammar<Iterator, osmoh::TWeekRanges(), space_type>
{
protected:
  qi::rule<Iterator, osmoh::WeekRange(), space_type> week;
  qi::rule<Iterator, osmoh::TWeekRanges(), space_type> main;

public:
  week_selector() : week_selector::base_type(main)
  {
    using qi::uint_;
    using qi::lit;
    using qi::_1;
    using qi::_2;
    using qi::_3;
    using qi::_val;
    using osmoh::WeekRange;

    week = (weeknum >> dash >> weeknum >> '/' >> uint_) [bind(&WeekRange::SetStart, _val, _1),
                                                         bind(&WeekRange::SetEnd, _val, _2),
                                                         bind(&WeekRange::SetPeriod, _val, _3)]
        | (weeknum >> dash >> weeknum) [bind(&WeekRange::SetStart, _val, _1),
                                        bind(&WeekRange::SetEnd, _val, _2)]
        | weeknum                      [bind(&WeekRange::SetStart, _val, _1)]
        ;

    main %= charset::no_case[lit("week")] >> (week % ',');
  }
};

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
    using qi::uint_;
    using qi::ushort_;
    using qi::lit;
    using qi::double_;
    using qi::lexeme;
    using osmoh::DateOffset;
    using osmoh::MonthDay;
    using osmoh::MonthdayRange;

    static const qi::int_parser<unsigned, 10, 4, 4> year = {};

    day_offset = ((lit('+')[_a = 1] | lit('-')[_a = -1]) >>
                  ushort_ >> charset::no_case[(lit("days") | lit("day"))]) [_val = _a * _1];

    date_offset = ((lit('+')[_a = true] | lit('-')[_a = false])
                   >> charset::no_case[wdays] >> day_offset)
                  [bind(&DateOffset::SetWDayOffset, _val, _1),
                   bind(&DateOffset::SetOffset, _val, _2),
                   bind(&DateOffset::SetWDayOffsetPositive, _val, _a)]
        | ((lit('+')[_a = true] | lit('-') [_a = false]) >> charset::no_case[wdays])
          [bind(&DateOffset::SetWDayOffset, _val, _1),
           bind(&DateOffset::SetWDayOffsetPositive, _val, _a)]
        | day_offset [bind(&DateOffset::SetOffset, _val, _1)]
        ;

    date_left = (year >> charset::no_case[month]) [bind(&MonthDay::SetYear, _val, _1),
                                                   bind(&MonthDay::SetMonth, _val, _2)]

        | charset::no_case[month]                 [bind(&MonthDay::SetMonth, _val, _1)]
        ;

    date_right = charset::no_case[month]          [bind(&MonthDay::SetMonth, _val, _1)]
        ;

    date_from = (date_left >> (daynum >> !(lit(':') >> qi::digit)))
                [_val = _1, bind(&MonthDay::SetDayNum, _val, _2)]
        | (year >> charset::no_case[lit("easter")]) [bind(&MonthDay::SetYear, _val, _1),
                                                     bind(&MonthDay::SetVariableDate, _val,
                                                          MonthDay::VariableDate::Easter)]
        | charset::no_case[lit("easter")]           [bind(&MonthDay::SetVariableDate, _val,
                                                          MonthDay::VariableDate::Easter)]
        ;

    date_to = date_from                        [_val = _1]
        | (daynum >> !(lit(':') >> qi::digit)) [bind(&MonthDay::SetDayNum, _val, _1)]
        ;

    date_from_with_offset = (date_from >> date_offset)
                            [_val = _1, bind(&MonthDay::SetOffset, _val, _2)]
        | date_from         [_val = _1]
        ;

    date_to_with_offset = (date_to >> date_offset)
                          [_val = _1, bind(&MonthDay::SetOffset, _val, _2)]
        | date_to         [_val = _1]
        ;

    monthday_range = (date_from_with_offset >> dash >> date_to_with_offset)
                     [bind(&MonthdayRange::SetStart, _val, _1),
                      bind(&MonthdayRange::SetEnd, _val, _2)]
        | (date_from_with_offset >> '+') [bind(&MonthdayRange::SetStart, _val, _1),
                                          bind(&MonthdayRange::SetPlus, _val, true)]
        | (date_left >> dash >> date_right >> '/' >> uint_)
          [bind(&MonthdayRange::SetStart, _val, _1),
           bind(&MonthdayRange::SetEnd, _val, _2),
           bind(&MonthdayRange::SetPeriod, _val, _3)]
        | (date_left >> lit("-") >> date_right) [bind(&MonthdayRange::SetStart, _val, _1),
                                                 bind(&MonthdayRange::SetEnd, _val, _2)]
        | date_from [bind(&MonthdayRange::SetStart, _val, _1)]
        | date_left [bind(&MonthdayRange::SetStart, _val, _1)]
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
  qi::rule<Iterator, osmoh::NthWeekdayOfTheMonthEntry::NthDayOfTheMonth(), space_type> nth;
  qi::rule<Iterator, osmoh::NthWeekdayOfTheMonthEntry(), space_type> nth_entry;
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
    using osmoh::NthWeekdayOfTheMonthEntry;
    using osmoh::Holiday;
    using osmoh::WeekdayRange;
    using osmoh::Weekdays;

    nth = ushort_(1)[_val = NthWeekdayOfTheMonthEntry::NthDayOfTheMonth::First]
        | ushort_(2) [_val = NthWeekdayOfTheMonthEntry::NthDayOfTheMonth::Second]
        | ushort_(3) [_val = NthWeekdayOfTheMonthEntry::NthDayOfTheMonth::Third]
        | ushort_(4) [_val = NthWeekdayOfTheMonthEntry::NthDayOfTheMonth::Fourth]
        | ushort_(5) [_val = NthWeekdayOfTheMonthEntry::NthDayOfTheMonth::Fifth];

    nth_entry = (nth >> dash >> nth) [bind(&NthWeekdayOfTheMonthEntry::SetStart, _val, _1),
                                      bind(&NthWeekdayOfTheMonthEntry::SetEnd, _val, _2)]
        | (lit('-') >> nth)          [bind(&NthWeekdayOfTheMonthEntry::SetEnd, _val, _1)]
        | nth [bind(&NthWeekdayOfTheMonthEntry::SetStart, _val, _1)]
        ;

    day_offset =
        ( (lit('+')[_a = 1] | lit('-') [_a = -1]) >>
          ushort_  [_val = _1 * _a] >>
          charset::no_case[(lit("days") | lit("day"))] )
        ;

    holiday = (charset::no_case[lit("SH")] [bind(&Holiday::SetPlural, _val, false)]
               >> -day_offset              [bind(&Holiday::SetOffset, _val, _1)])
        | charset::no_case[lit("PH")]      [bind(&Holiday::SetPlural, _val, true)]
        ;

    holiday_sequence %= (holiday % ',');

    weekday_range =
        ( charset::no_case[wdays]  [bind(&WeekdayRange::SetStart, _val, _1)] >>
          '[' >> (nth_entry        [bind(&WeekdayRange::AddNth, _val, _1)]) % ',') >> ']' >>
          -(day_offset             [bind(&WeekdayRange::SetOffset, _val, _1)])
        | charset::no_case[(wdays >> dash >> wdays)]  [bind(&WeekdayRange::SetStart, _val, _1),
                                                       bind(&WeekdayRange::SetEnd, _val, _2)]
        | charset::no_case[wdays]  [bind(&WeekdayRange::SetStart, _val, _1)]
        ;

    weekday_sequence %= (weekday_range % ',') >> !qi::no_skip[charset::alpha]
        ;

    main = (holiday_sequence >> -lit(',') >> weekday_sequence)
           [bind(&Weekdays::SetHolidays, _val, _1),
            bind(&Weekdays::SetWeekdayRanges, _val, _2)]
        | holiday_sequence [bind(&Weekdays::SetHolidays, _val, _1)]
        | weekday_sequence [bind(&Weekdays::SetWeekdayRanges, _val, _1)]
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
  qi::rule<Iterator, osmoh::HourMinutes(), space_type> hour_minutes;
  qi::rule<Iterator, osmoh::HourMinutes(), space_type> extended_hour_minutes;
  qi::rule<Iterator, osmoh::TimeEvent(), space_type> variable_time;
  qi::rule<Iterator, osmoh::Time(), space_type> extended_time;
  qi::rule<Iterator, osmoh::Time(), space_type> time;
  qi::rule<Iterator, osmoh::Timespan(), space_type> timespan;
  qi::rule<Iterator, osmoh::TTimespans(), space_type> main;

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
    using charset::char_;
    using boost::phoenix::bind;
    using boost::phoenix::construct;
    using osmoh::HourMinutes;
    using osmoh::TimeEvent;
    using osmoh::Time;
    using osmoh::Timespan;

    hour_minutes =
        (hours >> lit(':') >> minutes) [bind(&HourMinutes::AddDuration, _val, _1),
                                        bind(&HourMinutes::AddDuration, _val, _2)]
        ;

    extended_hour_minutes =
        (exthours >> lit(':') >> minutes)[bind(&HourMinutes::AddDuration, _val, _1),
                                          bind(&HourMinutes::AddDuration, _val, _2)]
        ;

    variable_time =
        ( lit('(')
          >> charset::no_case[event]         [bind(&TimeEvent::SetEvent, _val, _1)]
          >> ( (lit('+') >> hour_minutes)    [bind(&TimeEvent::SetOffset, _val, _1)]
               | (lit('-') >> hour_minutes)  [bind(&TimeEvent::SetOffset, _val, -_1)] )
          >> lit(')')
          )
        | charset::no_case[event][bind(&TimeEvent::SetEvent, _val, _1)]
        ;

    extended_time = extended_hour_minutes [bind(&Time::SetHourMinutes, _val, _1)]
        | variable_time                   [bind(&Time::SetEvent, _val, _1)]
        ;

    time = hour_minutes [bind(&Time::SetHourMinutes, _val, _1)]
        | variable_time [bind(&Time::SetEvent, _val, _1)]
        ;

    timespan =
        (time >> dash >> extended_time >> '/' >> hour_minutes)
        [bind(&Timespan::SetStart, _val, _1),
         bind(&Timespan::SetEnd, _val, _2),
         bind(&Timespan::SetPeriod, _val, _3)]

        | (time >> dash >> extended_time >> '/' >> minutes)
          [bind(&Timespan::SetStart, _val, _1),
           bind(&Timespan::SetEnd, _val, _2),
           bind(&Timespan::SetPeriod, _val, _3)]

        | (time >> dash >> extended_time >> '+')
          [bind(&Timespan::SetStart, _val, _1),
           bind(&Timespan::SetEnd, _val, _2),
           bind(&Timespan::SetPlus, _val, true)]

        | (time >> dash >> extended_time)
          [bind(&Timespan::SetStart, _val, _1),
           bind(&Timespan::SetEnd, _val, _2)]

        | (time >> '+')
          [bind(&Timespan::SetStart, _val, _1),
           bind(&Timespan::SetPlus, _val, true)]

        // This rule is only used for collection_times tag wish is not in our interest.
        // | time[bind(&Timespan::SetStart, _val, _1)]
        ;

    main %= timespan % ',';

    BOOST_SPIRIT_DEBUG_NODE(main);
    BOOST_SPIRIT_DEBUG_NODE(timespan);
    BOOST_SPIRIT_DEBUG_NODE(time);
    BOOST_SPIRIT_DEBUG_NODE(extended_time);
    BOOST_SPIRIT_DEBUG_NODE(variable_time);
    BOOST_SPIRIT_DEBUG_NODE(extended_hour_minutes);
  }
};

template <typename Iterator>
class time_domain : public qi::grammar<Iterator, osmoh::TRuleSequences(), space_type>
{
protected:
  weekday_selector<Iterator> weekday_selector;
  time_selector<Iterator> time_selector;
  year_selector<Iterator> year_selector;
  month_selector<Iterator> month_selector;
  week_selector<Iterator> week_selector;

  qi::rule<Iterator, std::string()> comment;
  qi::rule<Iterator, std::string(), space_type> separator;

  qi::rule<Iterator, qi::unused_type(osmoh::RuleSequence &), space_type> small_range_selectors;
  qi::rule<Iterator, qi::unused_type(osmoh::RuleSequence &), space_type> wide_range_selectors;
  qi::rule<Iterator, qi::unused_type(osmoh::RuleSequence &), space_type> rule_modifier;

  qi::rule<Iterator, osmoh::RuleSequence(), space_type> rule_sequence;
  qi::rule<Iterator, osmoh::TRuleSequences(), space_type> main;

public:
  time_domain() : time_domain::base_type(main)
  {
    using qi::lit;
    using qi::lexeme;
    using qi::_1;
    using qi::_a;
    using qi::_r1;
    using qi::_val;
    using charset::char_;
    using qi::eps;
    using qi::lazy;
    using phx::back;
    using phx::push_back;
    using phx::construct;
    using osmoh::RuleSequence;

    using Modifier = RuleSequence::Modifier;

    comment %= '"' >> +(char_ - '"') >> '"'
        ;

    separator %= charset::string(";")
        | charset::string("||")
        | charset::string(",")
        ;

    wide_range_selectors =
        ( -(year_selector    [bind(&RuleSequence::SetYears, _r1, _1)]) >>
          -(month_selector   [bind(&RuleSequence::SetMonths, _r1, _1)]) >>
          -(week_selector    [bind(&RuleSequence::SetWeeks, _r1, _1)]) >>
          -(lit(':')         [bind(&RuleSequence::SetSeparatorForReadability, _r1, true)]))
        | (comment >> ':')   [bind(&RuleSequence::SetComment, _r1, _1)]
        ;

    small_range_selectors =
        ( -(weekday_selector [bind(&RuleSequence::SetWeekdays, _r1, _1)]) >>
          -(time_selector    [bind(&RuleSequence::SetTimes, _r1, _1)]))
        ;

    rule_modifier =
        (charset::no_case[lit("open")]
           [bind(&RuleSequence::SetModifier, _r1, Modifier::Open)] >>
           -(comment [bind(&RuleSequence::SetModifierComment, _r1, _1)]))

        | ((charset::no_case[lit("closed") | lit("off")])
           [bind(&RuleSequence::SetModifier, _r1, Modifier::Closed)] >>
           -(comment [bind(&RuleSequence::SetModifierComment, _r1, _1)]))

        | (charset::no_case[lit("unknown")]
           [bind(&RuleSequence::SetModifier, _r1, Modifier::Unknown)] >>
           -(comment [bind(&RuleSequence::SetModifierComment, _r1, _1)]))

        | comment    [bind(&RuleSequence::SetModifier, _r1, Modifier::Comment),
                      bind(&RuleSequence::SetModifierComment, _r1, _1)]
        ;

    rule_sequence =
        ( lit("24/7") [bind(&RuleSequence::SetTwentyFourHours, _val, true)]
          | ( -wide_range_selectors(_val) >>
              -small_range_selectors(_val) )) >>
        -rule_modifier(_val)
        ;

    main = ( -(lit("opening_hours") >> lit('=')) >>
             (rule_sequence [push_back(_val, _1)] %
              (separator    [phx::bind(&RuleSequence::SetAnySeparator, back(_val), _1)])))
        ;

    BOOST_SPIRIT_DEBUG_NODE(main);
    BOOST_SPIRIT_DEBUG_NODE(rule_sequence);
    BOOST_SPIRIT_DEBUG_NODE(rule_modifier);
    BOOST_SPIRIT_DEBUG_NODE(small_range_selectors);
    BOOST_SPIRIT_DEBUG_NODE(wide_range_selectors);
  }
};
} // namespace parsing
} // namespace osmoh
#undef BOOST_SPIRIT_USE_PHOENIX_V3
