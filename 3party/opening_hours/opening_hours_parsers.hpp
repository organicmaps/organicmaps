#pragma once

#include "opening_hours.hpp"

// #define BOOST_SPIRIT_DEBUG
#include <boost/spirit/include/qi.hpp>

namespace osmoh
{
namespace phx = boost::phoenix;

namespace parsing
{
namespace qi = boost::spirit::qi;
namespace charset = boost::spirit::standard_wide;

using space_type = charset::space_type;

using Iterator = std::string::const_iterator;

struct dash_ : public qi::symbols<char> { dash_(); };
struct event_ : public qi::symbols<char, osmoh::TimeEvent::Event> { event_(); };
struct wdays_ : qi::symbols<char, osmoh::Weekday> { wdays_(); };
struct month_ : qi::symbols<char, osmoh::MonthDay::Month> { month_(); };
struct hours_ : qi::symbols<char, osmoh::Time::THours> { hours_(); };
struct exthours_ : qi::symbols<char, osmoh::Time::THours> { exthours_(); };
struct minutes_ : qi::symbols<char, osmoh::Time::TMinutes> { minutes_(); };
struct weeknum_ : qi::symbols<char, unsigned> { weeknum_(); };
struct daynum_ : qi::symbols<char, MonthDay::TDayNum> { daynum_(); };

extern dash_ dash;
extern event_ event;
extern wdays_ wdays;
extern month_ month;
extern hours_ hours;
extern exthours_ exthours;
extern minutes_ minutes;
extern weeknum_ weeknum;
extern daynum_ daynum;


class year_selector_parser : public qi::grammar<Iterator, osmoh::TYearRanges(), space_type>
{
protected:
  qi::rule<Iterator, osmoh::YearRange(), space_type> year_range;
  qi::rule<Iterator, osmoh::TYearRanges(), space_type> main;

public:
  year_selector_parser();
};

class week_selector_parser : public qi::grammar<Iterator, osmoh::TWeekRanges(), space_type>
{
protected:
  qi::rule<Iterator, osmoh::WeekRange(), space_type> week;
  qi::rule<Iterator, osmoh::TWeekRanges(), space_type> main;

public:
  week_selector_parser();
};

class month_selector_parser : public qi::grammar<Iterator, TMonthdayRanges(), space_type>
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
  month_selector_parser();
};

class weekday_selector_parser : public qi::grammar<Iterator, osmoh::Weekdays(), space_type>
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
  weekday_selector_parser();
};

class time_selector_parser : public qi::grammar<Iterator, osmoh::TTimespans(), space_type>
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
  time_selector_parser();
};

} // namespace parsing
} // namespace osmoh
