#include "parse_opening_hours.hpp"
#include "opening_hours_parsers.hpp"

#include <boost/phoenix/bind.hpp>
#include <boost/phoenix/operator.hpp>  // operator,

namespace osmoh
{
  namespace parsing
  {
    week_selector_parser::week_selector_parser() : week_selector_parser::base_type(main)
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
  }

  bool Parse(std::string const & str, TWeekRanges & context)
  {
    return osmoh::ParseImpl<parsing::week_selector_parser>(str, context);
  }
} // namespace osmoh
