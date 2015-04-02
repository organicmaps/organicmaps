#pragma once

#include <string>
#include <vector>
#include <iostream>

namespace osmoh
{
  class Time
  {
  public:
    enum EFlags
    {
      eNone = 0,
      eHours = 1,
      eMinutes = 2,
      ePlus = 4,
      eMinus = 8,
      eExt = 16,
      eSunrise = 32,
      eSunset = 64
    };

    uint8_t hours;
    uint8_t minutes;
    uint8_t flags;

    Time() : hours(0), minutes(0), flags(eNone) {}
    inline Time & Hours(uint8_t h) { hours = h; flags |= eHours; return *this; }
    inline Time & Minutes(uint8_t m) { minutes = m; flags |= eMinutes; return *this; }
    inline Time & Sunset() { flags = eSunset; return *this; }
    inline Time & Sunrise() { flags = eSunrise; return *this; }

    friend std::ostream & operator << (std::ostream & s, Time const & t);
  };

  class TimeSpan
  {
  public:
    Time from;
    Time to;
    uint8_t flags;
    Time period;

    TimeSpan() : flags(Time::eNone) {}

    friend std::ostream & operator << (std::ostream & s, TimeSpan const & span);
  };

  class Weekdays
  {
  public:
    uint8_t weekdays;
    uint16_t nth;
    int32_t offset;

    Weekdays() : weekdays(0), nth(0), offset(0) {}

    friend std::ostream & operator << (std::ostream & s, Weekdays const & w);
  };

  class State
  {
  public:
    enum EState {
      eUnknown = 0,
      eClosed = 1,
      eOpen = 2
    };

    uint8_t state;
    std::string comment;

    State() : state(eUnknown) {}
  };

  class TimeRule
  {
  public:
    std::vector<Weekdays> weekdays;
    std::vector<TimeSpan> timespan;
    State state;
    uint8_t int_flags = 0;
  };
} // namespace osmoh

class OSMTimeRange
{
  std::string m_sourceString;
  bool m_valid;
  osmoh::State::EState m_state;
  std::vector<osmoh::TimeRule> m_rules;
  std::string m_comment;

public:
  OSMTimeRange(std::string const & rules);

  inline bool IsValid() const { return m_valid; }
  inline bool IsOpen() const { return m_state == osmoh::State::eOpen; }
  inline bool IsClosed() const { return m_state == osmoh::State::eClosed; }
  inline bool IsUnknown() const { return m_state == osmoh::State::eUnknown; }
  inline std::string const & Comment() const { return m_comment; }

  OSMTimeRange & operator()(time_t timestamp);
  OSMTimeRange & operator()(std::string const & timestr, char const * timefmt="%d-%m-%Y %R");

private:
  void parse();
};

