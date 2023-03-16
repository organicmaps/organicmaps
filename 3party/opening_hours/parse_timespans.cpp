#include "parse_opening_hours.hpp"
#include "opening_hours_parsers.hpp"

#include <boost/phoenix/bind.hpp>
#include <boost/phoenix/operator.hpp>  // operator,

namespace osmoh
{
  namespace parsing
  {
    time_selector_parser::time_selector_parser() : time_selector_parser::base_type(main)
    {
      using qi::int_;
      using qi::_1;
      using qi::_2;
      using qi::_3;
      using qi::_a;
      using qi::_val;
      using qi::lit;
      using charset::char_;
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
  }

  bool Parse(std::string const & str, TTimespans & context)
  {
    return osmoh::ParseImpl<parsing::time_selector_parser>(str, context);
  }
} // namespace osmoh
