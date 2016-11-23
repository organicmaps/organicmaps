#pragma once

#include "opening_hours.hpp"
#include <string>
#include <boost/spirit/include/qi.hpp>


namespace osmoh
{
template<typename Parser, typename Context>
bool ParseImpl(std::string const & str, Context & context)
{
  using boost::spirit::qi::phrase_parse;
  using boost::spirit::standard_wide::space;

  Parser parser;
#ifndef NDEBUG
  boost::spirit::qi::what(parser);
#endif

  auto first = begin(str);
  auto const last = end(str);
  auto parsed = phrase_parse(first, last, parser, space, context);

  if (!parsed || first != last)
    return false;

  return true;
}

bool Parse(std::string const &, TTimespans &);
bool Parse(std::string const &, Weekdays &);
bool Parse(std::string const &, TMonthdayRanges &);
bool Parse(std::string const &, TYearRanges &);
bool Parse(std::string const &, TWeekRanges &);
bool Parse(std::string const &, TRuleSequences &);
} // namespace osmoh
