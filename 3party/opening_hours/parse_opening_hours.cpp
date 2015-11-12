#include "parse_opening_hours.hpp"
#include "opening_hours_parsers.hpp"

namespace
{
template <typename Context, typename Iterator>
struct context_parser;

template<typename Iterator> struct context_parser<osmoh::TTimespans, Iterator>
{
  using type = osmoh::parsing::time_selector<Iterator>;
};

template<typename Iterator> struct context_parser<osmoh::Weekdays, Iterator>
{
  using type = osmoh::parsing::weekday_selector<Iterator>;
};

template<typename Iterator> struct context_parser<osmoh::TMonthdayRanges, Iterator>
{
  using type = osmoh::parsing::month_selector<Iterator>;
};

template<typename Iterator> struct context_parser<osmoh::TYearRanges, Iterator>
{
  using type = osmoh::parsing::year_selector<Iterator>;
};

template<typename Iterator> struct context_parser<osmoh::TWeekRanges, Iterator>
{
  using type = osmoh::parsing::week_selector<Iterator>;
};

template<typename Iterator> struct context_parser<osmoh::TRuleSequences, Iterator>
{
  using type = osmoh::parsing::time_domain<Iterator>;
};

template <typename Context, typename Iterator>
using context_parser_t = typename context_parser<Context, Iterator>::type;

template <typename Context>
inline bool ParseImp(std::string const & str, Context & context)
{
  using boost::spirit::qi::phrase_parse;
  using boost::spirit::standard_wide::space;

  context_parser_t<Context, decltype(begin(str))> parser;
#ifndef NDEBUG
  boost::spirit::qi::what(parser);
#endif

  auto first = begin(str);
  auto const last = end(str);
  auto parsed = phrase_parse(first, last, parser,
                             space, context);

  if (!parsed || first != last)
    return false;

  return true;
}
} // namespace


namespace osmoh
{
bool Parse(std::string const & str, TTimespans & s)
{
  return ParseImp(str, s);
}

bool Parse(std::string const & str, Weekdays & w)
{
  return ParseImp(str, w);
}

bool Parse(std::string const & str, TMonthdayRanges & m)
{
  return ParseImp(str, m);
}

bool Parse(std::string const & str, TYearRanges & y)
{
  return ParseImp(str, y);
}

bool Parse(std::string const & str, TWeekRanges & w)
{
  return ParseImp(str, w);
}

bool Parse(std::string const & str, TRuleSequences & r)
{
  return ParseImp(str, r);
}
} // namespace osmoh
