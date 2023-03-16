#include "parse_opening_hours.hpp"
#include "opening_hours_parsers.hpp"

#include <boost/phoenix/bind.hpp>
#include <boost/phoenix/operator.hpp>  // operator,

namespace osmoh
{
  namespace parsing
  {
    month_selector_parser::month_selector_parser() : month_selector_parser::base_type(main)
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
  }

  bool Parse(std::string const & str, TMonthdayRanges & context)
  {
    return osmoh::ParseImpl<parsing::month_selector_parser>(str, context);
  }
} // namespace osmoh
