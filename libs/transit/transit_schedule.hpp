#pragma once

#include "base/macros.hpp"
#include "base/newtype.hpp"
#include "base/visitor.hpp"

#include <array>
#include <chrono>
#include <cstdint>
#include <ctime>
#include <map>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

#include "3party/just_gtfs/just_gtfs.h"

// This is the implementation of the Schedule class and its helpers for handling transit service
// days and stops timetables.
namespace routing
{
namespace transit
{
template <class Sink>
class Serializer;
template <class Source>
class Deserializer;
template <typename Sink>
class FixedSizeSerializer;
template <typename Sink>
class FixedSizeDeserializer;
}  // namespace transit
}  // namespace routing

namespace transit
{
#define DECLARE_SCHEDULE_TYPES_FRIENDS                  \
  template <class Sink>                                 \
  friend class routing::transit::Serializer;            \
  template <class Source>                               \
  friend class routing::transit::Deserializer;          \
  template <typename Sink>                              \
  friend class routing::transit::FixedSizeSerializer;   \
  template <typename Sink>                              \
  friend class routing::transit::FixedSizeDeserializer;

// Status of some transit itinerary (e.g. line) in the moment of time.
enum class Status : uint8_t
{
  Open,
  Closed,
  Unknown
};

std::string DebugPrint(Status const & status);

struct Date
{
  Date() = default;
  Date(uint32_t year, uint8_t month, uint16_t day) : m_year(year), m_month(month), m_day(day) {}

  DECLARE_SCHEDULE_TYPES_FRIENDS
  DECLARE_VISITOR_AND_DEBUG_PRINT(Date, visitor(m_year, "y"), visitor(m_month, "m"), visitor(m_day, "d"))

  uint32_t m_year = 0;
  uint8_t m_month = 0;
  uint16_t m_day = 0;
};

struct Time
{
  Time() = default;
  Time(uint8_t hour, uint8_t minute, uint8_t second) : m_hour(hour), m_minute(minute), m_second(second) {}

  DECLARE_SCHEDULE_TYPES_FRIENDS
  DECLARE_VISITOR_AND_DEBUG_PRINT(Time, visitor(m_hour, "h"), visitor(m_minute, "m"), visitor(m_second, "s"))

  uint8_t m_hour = 0;
  uint8_t m_minute = 0;
  uint8_t m_second = 0;
};

using WeekSchedule = std::array<bool, 7>;

enum WeekDays
{
  Sunday = 0,
  Monday,
  Tuesday,
  Wednesday,
  Thursday,
  Friday,
  Saturday
};

// Service dates specified using a weekly schedule with start and end dates. Dates range is
// specified by the start_date and end_date fields in the GTFS calendar.txt.
// Dates interval and open/closed states for week days are stored |m_data|.
class DatesInterval
{
public:
  DatesInterval() = default;
  explicit DatesInterval(gtfs::CalendarItem const & calendarItem);
  explicit DatesInterval(uint32_t data);

  bool operator==(DatesInterval const & rhs) const;

  std::tuple<Date, Date, WeekSchedule> Extract() const;
  uint32_t GetRaw() const { return m_data; }

  Status GetStatusInInterval(Date const & date, uint8_t wdIndex) const;

private:
  DECLARE_SCHEDULE_TYPES_FRIENDS
  DECLARE_VISITOR_AND_DEBUG_PRINT(DatesInterval, visitor(m_data, "m_data"))

  // From greater indexes to smaller: year1, month1, day1, years delta (year2 - year1), month2,
  // day2, weekdays. Week indexes start from sunday = 0.
  uint32_t m_data = 0;
};

struct DatesIntervalHasher
{
  size_t operator()(DatesInterval const & key) const { return key.GetRaw(); }
};

// Exceptions for the services defined in the calendar.txt. If calendar.txt is not used, it may
// contain all dates of service. Exception date and its type (added/removed) are stored in |m_data|.
class DateException
{
public:
  DateException() = default;
  DateException(gtfs::Date const & date, gtfs::CalendarDateException const & dateException);
  explicit DateException(uint16_t data);

  bool operator==(DateException const & rhs) const;

  std::tuple<Date, bool> Extract() const;
  uint32_t GetRaw() const { return m_data; }

  Status GetExceptionStatus(Date const & date) const;

private:
  DECLARE_SCHEDULE_TYPES_FRIENDS
  DECLARE_VISITOR_AND_DEBUG_PRINT(DateException, visitor(m_data, "m_data"))

  // From greater indexes to smaller: year, month, day, status
  uint16_t m_data = 0;
};

struct DateExceptionHasher
{
  size_t operator()(DateException const & key) const { return key.GetRaw(); }
};

// Time range for lines frequencies or stop timetables. Start time and end time are stored in
// m_data.
class TimeInterval
{
public:
  TimeInterval() = default;
  TimeInterval(gtfs::Time const & startTime, gtfs::Time const & endTime);
  explicit TimeInterval(uint64_t data);

  bool operator<(TimeInterval const & rhs) const;
  bool operator==(TimeInterval const & rhs) const;

  std::pair<Time, Time> Extract() const;
  uint64_t GetRaw() const { return m_data; }

  Status GetTimeStatus(Time const & time) const;

private:
  DECLARE_SCHEDULE_TYPES_FRIENDS
  DECLARE_VISITOR_AND_DEBUG_PRINT(TimeInterval, visitor(m_data, "m_data"))

  // From greater indexes to smaller: start time, end time.
  uint64_t m_data = 0;
};

using Frequency = uint32_t;
inline Frequency constexpr kDefaultFrequency = 0;

// Headway (interval between times that a vehicle arrives at and departs from stops) for
// headway-based service or a compressed representation of fixed-schedule service. For each time
// range there is a frequency value.
class FrequencyIntervals
{
public:
  FrequencyIntervals() = default;
  FrequencyIntervals(gtfs::Frequencies const & frequencies);

  bool operator==(FrequencyIntervals const & rhs) const;

  void AddInterval(TimeInterval const & timeInterval, Frequency frequency);

  Frequency GetFrequency(Time const & time) const;
  std::map<TimeInterval, Frequency> const & GetFrequencies() const;

private:
  DECLARE_SCHEDULE_TYPES_FRIENDS
  DECLARE_VISITOR_AND_DEBUG_PRINT(FrequencyIntervals, visitor(m_intervals, "m_intervals"))

  std::map<TimeInterval, Frequency> m_intervals;
};

using DatesIntervals = std::unordered_map<DatesInterval, FrequencyIntervals, DatesIntervalHasher>;
using DatesExceptions = std::unordered_map<DateException, FrequencyIntervals, DateExceptionHasher>;

// Line schedule with line service days (as DatesInterval ranges) and exceptions in service days
// (as DateException items). For each date there are frequency intervals (time ranges with headway
// in seconds). This schedule is useful while building transit routes based on particular route
// start time.
class Schedule
{
public:
  bool operator==(Schedule const & rhs) const;

  void AddDatesInterval(gtfs::CalendarItem const & calendarItem, gtfs::Frequencies const & frequencies);
  void AddDateException(gtfs::Date const & date, gtfs::CalendarDateException const & dateException,
                        gtfs::Frequencies const & frequencies);

  Status GetStatus(time_t const & time) const;
  Frequency GetFrequency(time_t const & time) const;
  Frequency GetFrequency() const { return m_defaultFrequency; }

  DatesIntervals const & GetServiceIntervals() const;
  DatesExceptions const & GetServiceExceptions() const;

  void AddDatesInterval(DatesInterval const & interval, FrequencyIntervals const & frequencies);
  void AddDateException(DateException const & dateException, FrequencyIntervals const & frequencies);

  void SetDefaultFrequency(Frequency const & frequency) { m_defaultFrequency = frequency; }

private:
  std::pair<Date, uint8_t> GetDateAndWeekIndex(time_t const & time) const;
  std::tuple<Date, Time, uint8_t> GetDateTimeAndWeekIndex(time_t const & time) const;

  DECLARE_SCHEDULE_TYPES_FRIENDS
  DECLARE_VISITOR_AND_DEBUG_PRINT(Schedule, visitor(m_serviceIntervals, "m_serviceIntervals"),
                                  visitor(m_serviceExceptions, "m_serviceExceptions"),
                                  visitor(m_defaultFrequency, "m_defaultFrequency"))

  DatesIntervals m_serviceIntervals;
  DatesExceptions m_serviceExceptions;

  Frequency m_defaultFrequency = kDefaultFrequency;
};

#undef DECLARE_SCHEDULE_TYPES_FRIENDS
}  // namespace transit
