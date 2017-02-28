#include "parse_opening_hours.hpp"
#include "opening_hours_parsers.hpp"

#include <boost/spirit/include/phoenix_bind.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_stl.hpp>

namespace osmoh
{
namespace parsing
{
  dash_ dash;
  event_ event;
  wdays_ wdays;
  month_ month;
  hours_ hours;
  exthours_ exthours;
  minutes_ minutes;
  weeknum_ weeknum;
  daynum_ daynum;

  class time_domain : public qi::grammar<Iterator, osmoh::TRuleSequences(), space_type>
  {
  protected:
    weekday_selector_parser weekday_selector;
    time_selector_parser time_selector;
    year_selector_parser year_selector;
    month_selector_parser month_selector;
    week_selector_parser week_selector;

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
}  // namespace parsing

bool Parse(std::string const & str, TRuleSequences & context)
{
  return osmoh::ParseImpl<parsing::time_domain>(str, context);
}
} // namespace osmoh
