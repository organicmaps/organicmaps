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
public:
  OSMTimeRange() = default;

  inline bool IsValid() const { return m_valid; }
  inline bool IsOpen() const { return m_state == osmoh::State::eOpen; }
  inline bool IsClosed() const { return m_state == osmoh::State::eClosed; }
  inline bool IsUnknown() const { return m_state == osmoh::State::eUnknown; }
  inline std::string const & Comment() const { return m_comment; }

  OSMTimeRange & operator()(time_t timestamp);
  OSMTimeRange & operator()(std::string const & timestr,
                            char const * timefmt="%d-%m-%Y %R");

  static OSMTimeRange FromString(std::string const & rules);

private:
  bool m_valid{false};
  osmoh::State::EState m_state{osmoh::State::eUnknown};
  std::vector<osmoh::TimeRule> m_rules;
  std::string m_comment;
};

