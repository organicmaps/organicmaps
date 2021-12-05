#include "parse_opening_hours.hpp"
#include "opening_hours_parsers.hpp"

#include <boost/phoenix/bind.hpp>
#include <boost/phoenix/operator.hpp>  // operator,

namespace osmoh
{
  namespace parsing
  {
    weekday_selector_parser::weekday_selector_parser() : weekday_selector_parser::base_type(main)
    {
      using qi::_a;
      using qi::_1;
      using qi::_2;
      using qi::_val;
      using qi::lit;
      using qi::ushort_;
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
  }

  bool Parse(std::string const & str, Weekdays & context)
  {
    return osmoh::ParseImpl<parsing::weekday_selector_parser>(str, context);
  }
} // namespace osmoh
