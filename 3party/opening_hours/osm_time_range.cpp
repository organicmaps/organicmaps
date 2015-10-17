/*
  The MIT License (MIT)

  Copyright (c) 2015 Mail.Ru Group

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.
*/

#include "osm_parsers.hpp"

#include <iomanip>
#include <ios>
#include <codecvt>

namespace osmoh
{

std::ostream & operator << (std::ostream & s, Time const & t)
{
  bool event = (t.flags & Time::eSunrise) || (t.flags & Time::eSunset);
  if (event)
    s << ((t.flags & Time::eSunrise) ? "sunrise" : "sunset") << " (";
  std::ios_base::fmtflags sf = s.flags();
  if (t.flags & (Time::ePlus | Time::eMinus))
    s << ((t.flags & Time::ePlus) ? "+" : "-");
  if (t.flags & Time::eHours)
    s << std::setw(2) << std::setfill('0') << (int)t.hours;
  if (t.flags & Time::eMinutes)
    s << ":" << std::setw(2) << std::setfill('0') << (int)t.minutes;
  s.flags(sf);
  if (event)
    s << ")";
  return s;
}

std::ostream & operator << (std::ostream & s, TimeSpan const & span)
{
  s << span.from;
  if (span.to.flags)
    s << '-' << span.to;
  if (span.flags == Time::ePlus)
    s << "...";
  if (span.flags == Time::eExt)
    s << '/' << span.period;

  return s;
}

std::ostream & operator << (std::ostream & s, Weekday const & w)
{
  static char const * wdays[] = {"Mo", "Tu", "We", "Th", "Fr", "Sa", "Su"};
  static uint8_t const kDaysInWeek = 7;
  static uint8_t const kWeeksInMonth = 5;

  for (size_t i = 0; i < kDaysInWeek; ++i)
  {
    if (w.weekdays & (1 << i))
    {
      if (w.weekdays & ((1 << i) - 1))
        s << ',';
      s << wdays[i];
    }
  }

  if (w.nth)
  {
    s << "[";

    uint8_t a = w.nth & 0xFF;
    for (size_t i = 0; i < kWeeksInMonth; ++i)
    {
      if (a & (1 << i))
      {
        if (a & ((1 << i) - 1))
          s << ',';
        s << (i + 1);
      }
    }

    a = (w.nth >> 8) & 0xFF;
    for (size_t i = 0; i < kWeeksInMonth; ++i)
    {
      if (a & (1 << i))
      {
        if (a & ((1 << i) - 1))
          s << ',';
        s << '-' << (i + 1);
      }
    }

    s << "]";
  }

  if (w.offset)
    s << ' ' << w.offset << " day(s)";
  return s;
}

std::ostream & operator << (std::ostream & s, State const & w)
{
  static char const * st[] = {"unknown", "closed", "open"};
  s << ' ' << st[w.state] << " " << w.comment;
  return s;
}

std::ostream & operator << (std::ostream & s, TimeRule const & w)
{
  for (auto const & e : w.weekdays)
    s << e;
  if (!w.weekdays.empty() && !w.timespan.empty())
    s << ' ';
  for (auto const & e : w.timespan)
    s << e;

  std::cout << "Weekdays size " << w.weekdays.size() <<
      " Timespan size " << w.timespan.size() << std::endl;
  return s << w.state;
}
} // namespace osmoh


namespace
{
bool check_timespan(osmoh::TimeSpan const &ts, boost::gregorian::date const & d, boost::posix_time::ptime const & p)
{
  using boost::gregorian::days;
  using boost::posix_time::ptime;
  using boost::posix_time::hours;
  using boost::posix_time::minutes;
  using boost::posix_time::time_period;

  time_period tp1 = osmoh::make_time_period(d-days(1), ts);
  time_period tp2 = osmoh::make_time_period(d, ts);
  /* very useful in debug */
  //    std::cout << ts << "\t" << tp1 << "(" << p << ")" << (tp1.contains(p) ? " hit" : " miss") << std::endl;
  //    std::cout << ts << "\t" << tp2 << "(" << p << ")" << (tp2.contains(p) ? " hit" : " miss") << std::endl;
  return tp1.contains(p) || tp2.contains(p);
}

bool check_weekday(osmoh::Weekday const & wd, boost::gregorian::date const & d)
{
  using namespace boost::gregorian;

  bool hit = false;
  typedef nth_day_of_the_week_in_month nth_dow;
  if (wd.nth)
  {
    for (uint8_t i = 0; (wd.weekdays & (0xFF ^ ((1 << i) - 1))); ++i)
    {
      if (!(wd.weekdays & (1 << i)))
        continue;

      uint8_t a = wd.nth & 0xFF;
      for (size_t j = 0; (a & (0xFF ^ ((1 << j) - 1))); ++j)
      {
        if (a & (1 << j))
        {
          nth_dow ndm(nth_dow::week_num(j + 1), nth_dow::day_of_week_type((i + 1 == 7) ? 0 : (i + 1)), d.month());
          hit |= (d == ndm.get_date(d.year()));
        }
      }
      a = (wd.nth >> 8) & 0xFF;
      for (size_t j = 0; (a & (0xFF ^ ((1 << j) - 1))); ++j)
      {
        if (a & (1 << j))
        {
          last_day_of_the_week_in_month lwdm(nth_dow::day_of_week_type((i + 1 == 7) ? 0 : (i + 1)), d.month());
          hit |= (d == ((lwdm.get_date(d.year()) - weeks(j)) + days(wd.offset)));
        }
      }
    }
  }
  else
  {
    for (uint8_t i = 0; (wd.weekdays & (0xFF ^ ((1 << i) - 1))); ++i)
    {
      if (!(wd.weekdays & (1 << i)))
        continue;
      hit |= (d.day_of_week() == ((i + 1 == 7) ? 0 : (i + 1)));
    }
  }
  /* very useful in debug */
  //    std::cout << d.day_of_week() << " " <<  d << " --> " << wd << (hit ? " hit" : " miss") << std::endl;
  return hit;
}

bool check_rule(osmoh::TimeRule const & r, std::tm const & stm,
                std::ostream * hitcontext = nullptr)
{
  bool next = false;

  // check 24/7
  if (r.weekdays.empty() && r.timespan.empty() && r.state.state == osmoh::State::eOpen)
    return true;

  boost::gregorian::date date = boost::gregorian::date_from_tm(stm);
  boost::posix_time::ptime pt = boost::posix_time::ptime_from_tm(stm);

  next = r.weekdays.empty();
  for (auto const & wd : r.weekdays)
  {
    if (check_weekday(wd, date))
    {
      if (hitcontext)
        *hitcontext << wd << " ";
      next = true;
    }
  }
  if (!next)
    return next;

  next = r.timespan.empty();
  for (auto const & ts : r.timespan)
  {
    if (check_timespan(ts, date, pt))
    {
      if (hitcontext)
        *hitcontext << ts << " ";
      next = true;
    }
  }
  return next && !(r.timespan.empty() && r.weekdays.empty());
}
} // anonymouse namespace

OSMTimeRange OSMTimeRange::FromString(std::string const & rules)
{
  OSMTimeRange timeRange;
  std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter; // could not work on android
  std::wstring src = converter.from_bytes(rules); // rules should be wstring
  timeRange.m_valid = osmoh::parsing::parse_timerange(src.begin(), src.end(), timeRange.m_rules);
  return timeRange;
}

OSMTimeRange & OSMTimeRange::UpdateState(time_t timestamp)
{
  std::tm stm = *localtime(&timestamp);

  osmoh::State::EState true_state[3][3] = {
    {osmoh::State::eUnknown, osmoh::State::eClosed, osmoh::State::eOpen},
    {osmoh::State::eClosed , osmoh::State::eClosed, osmoh::State::eOpen},
    {osmoh::State::eOpen   , osmoh::State::eClosed, osmoh::State::eOpen}
  };

  osmoh::State::EState false_state[3][3] = {
    {osmoh::State::eUnknown, osmoh::State::eOpen   , osmoh::State::eClosed},
    {osmoh::State::eClosed , osmoh::State::eClosed , osmoh::State::eClosed},
    {osmoh::State::eOpen   , osmoh::State::eOpen   , osmoh::State::eOpen}
  };

  m_state = osmoh::State::eUnknown;
  m_comment = std::string();

  for (auto const & el : m_rules)
  {
    bool hit = false;
    if ((hit = check_rule(el, stm)))
    {
      m_state = true_state[m_state][el.state.state];
      m_comment = el.state.comment;
    }
    else
    {
      m_state = false_state[m_state][el.state.state];
    }
    /* very useful in debug */
    //    char const * st[] = {"unknown", "closed", "open"};
    //    std::cout << "-[" << hit << "]-------------------[" << el << "]: " << st[m_state] << "--------------------" << std::endl;
  }
  return *this;
}

OSMTimeRange & OSMTimeRange::UpdateState(std::string const & timestr, char const * timefmt)
{
  std::tm when = {};
  std::stringstream ss(timestr);
  ss >> std::get_time(&when, timefmt);
  return UpdateState(std::mktime(&when));
}
