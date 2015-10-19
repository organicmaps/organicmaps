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
#include <chrono>
#include <type_traits>

namespace osmoh
{
class Weekday;
class Timespan;
class TimeRule;

using TWeekdays = std::vector<Weekday>;
using TTimespans = std::vector<Timespan>;

class Time
{
  enum EState
  {
    eIsNotTime = 0,
    eHaveHours = 1,
    eHaveMinutes = 2,
  };

 public:
  enum class EEvent
  {
    eNotEvent,
    eDawn,
    eSunrise,
    eSunset,
    eDusk
  };

  using THours = std::chrono::hours;
  using TMinutes = std::chrono::minutes;

 public:
  Time() = default;
  Time(Time const &) = default;
  Time(TMinutes const minutes);
  Time(THours const hours, TMinutes const minutes);
  Time(EEvent const event);

  THours::rep GetHoursCount() const;
  TMinutes::rep GetMinutesCount() const;

  THours GetHours() const;
  TMinutes GetMinutes() const;

  void SetHours(THours const hours);
  void SetMinutes(TMinutes const minutes);

  EEvent GetEvent() const {return m_event;}
  void SetEvent(EEvent const event);

  bool IsEvent() const;
  bool IsEventOffset() const;
  bool IsHoursMinutes() const;
  bool IsMinutes() const;
  bool IsTime() const;
  bool HasValue() const;

  Time & operator-(Time const & time);
  Time & operator-();

 private:
  Time GetEventTime() const;

 private:
  EEvent m_event{EEvent::eNotEvent};
  TMinutes m_duration{TMinutes::zero()};
  std::underlying_type<EState>::type m_state{EState::eIsNotTime};
};

inline constexpr Time::THours operator ""_h(unsigned long long h)
{
  return Time::THours(h);
}

inline constexpr Time::TMinutes operator ""_min(unsigned long long m)
{
  return Time::TMinutes(m);
}

std::ostream & operator<<(std::ostream & ost, Time::EEvent const event);
std::ostream & operator<<(std::ostream & ost, Time const & time);

class Timespan
{
 public:
  Timespan() = default;
  Timespan(Timespan const &) = default;
  Timespan(Time const & start, bool plus = false);
  Timespan(Time const & start, Time const & end, bool plus = false);
  Timespan(Time const & start, Time const & end, Time const & period);

  bool IsOpen() const;
  bool HasPlus() const;
  bool HasPeriod() const;

  Time const & GetStart() const;
  Time const & GetEnd() const;
  Time const & GetPeriod() const;

  void SetStart(Time const & start);
  void SetEnd(Time const & end);
  void SetPeriod(Time const & period);
  void SetPlus(bool const plus);

  bool IsValid() const;

 private:
  Time m_start;
  Time m_end;
  Time m_period;
  bool m_plus{false};
};

std::ostream & operator<<(std::ostream & ost, Timespan const & span);
std::ostream & operator<<(std::ostream & ost, osmoh::TTimespans const & timespans);
} // namespace osmoh

// class Weekday
// {
//  public:
//   uint8_t weekdays;
//   uint16_t nth;
//   int32_t offset;

//   Weekday() : weekdays(0), nth(0), offset(0) {}

//   std::string ToString() const;
//   friend std::ostream & operator << (std::ostream & s, Weekday const & w);
// };

// class State
// {
//  public:
//   enum EState {
//     eUnknown = 0,
//     eClosed = 1,
//     eOpen = 2
//   };

//   uint8_t state;
//   std::string comment;

//   State() : state(eUnknown) {}
// };

// class TimeRule
// {
//  public:
//   TWeekdays weekdays;
//   TTimeSpans timespan; // TODO(mgsergio) rename to timespans
//   State state;
//   uint8_t int_flags = 0;

//   friend std::ostream & operator <<(std::ostream & s, TimeRule const & r);
// };
// } // namespace osmoh

// class OSMTimeRange
// {
//  public:
//   OSMTimeRange() = default;

//   bool IsValid() const { return m_valid; }
//   bool IsOpen() const { return m_state == osmoh::State::eOpen; }
//   bool IsClosed() const { return m_state == osmoh::State::eClosed; }
//   bool IsUnknown() const { return m_state == osmoh::State::eUnknown; }
//   std::string const & Comment() const { return m_comment; }

//   OSMTimeRange & UpdateState(time_t timestamp);
//   OSMTimeRange & UpdateState(std::string const & timestr,
//                              char const * timefmt="%d-%m-%Y %R");

//   std::string ToString() const;

//   static OSMTimeRange FromString(std::string const & rules);

//  private:
//   bool m_valid{false};
//   osmoh::State::EState m_state{osmoh::State::eUnknown};

//   osmoh::TTimeRules m_rules;
//   std::string m_comment;
// };




/// Trash

// class TimeEx
// {
//  public:
//   enum EFlags
//   {
//     eNone = 0,
//     eHours = 1,
//     eMinutes = 2,
//     ePlus = 4,
//     eMinus = 8,
//     eExt = 16,
//     eSunrise = 32,
//     eSunset = 64
//   };

//   uint8_t hours;
//   uint8_t minutes;
//   uint8_t flags;

//   Time() : hours(0), minutes(0), flags(eNone) {}
//   Time & Hours(uint8_t h) { hours = h; flags |= eHours; return *this; }
//   Time & Minutes(uint8_t m) { minutes = m; flags |= eMinutes; return *this; }
//   Time & Sunset() { flags = eSunset; return *this; }
//   Time & Sunrise() { flags = eSunrise; return *this; }

//   std::string ToString() const;
//   friend std::ostream & operator << (std::ostream & s, Time const & t);
// };

// class TimeSpanEx
// {
//  public:
//   Time from;
//   Time to;
//   uint8_t flags;
//   Time period;

//   TimeSpan() : flags(Time::eNone) {}

//   std::string ToString() const;
//   friend std::ostream & operator << (std::ostream & s, TimeSpan const & span);
// };
