#include "transit/transit_schedule.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"

namespace
{
// Constant which is added to the year value while deserizlizing from the unsigned int.
uint32_t constexpr kEpochStartYear = 2020;

uint32_t constexpr kMask4bits = 0xf;
uint32_t constexpr kMask5bits = 0x1f;
uint32_t constexpr kMask6bits = 0x3F;

std::tm ToCalendarTime(time_t const & ts)
{
  std::tm tm;
  localtime_r(&ts, &tm);
  return tm;
}

uint8_t GetRawStatus(gtfs::CalendarAvailability const & status)
{
  return status == gtfs::CalendarAvailability::Available ? 1 : 0;
}

uint8_t GetRawStatus(gtfs::CalendarDateException const & status)
{
  return status == gtfs::CalendarDateException::Added ? 1 : 0;
}

enum class DateTimeRelation
{
  // First element is earlier in time, later or is equal.
  Earlier,
  Later,
  Equal
};

DateTimeRelation GetDatesRelation(::transit::Date const & date1, ::transit::Date const & date2)
{
  if (date1.m_year < date2.m_year)
    return DateTimeRelation::Earlier;
  if (date1.m_year > date2.m_year)
    return DateTimeRelation::Later;

  if (date1.m_month < date2.m_month)
    return DateTimeRelation::Earlier;
  if (date1.m_month > date2.m_month)
    return DateTimeRelation::Later;

  if (date1.m_day < date2.m_day)
    return DateTimeRelation::Earlier;
  if (date1.m_day > date2.m_day)
    return DateTimeRelation::Later;

  return DateTimeRelation::Equal;
}

DateTimeRelation GetTimesRelation(::transit::Time const & time1, ::transit::Time const & time2)
{
  if (time1.m_hour < time2.m_hour)
    return DateTimeRelation::Earlier;
  if (time1.m_hour > time2.m_hour)
    return DateTimeRelation::Later;

  if (time1.m_minute < time2.m_minute)
    return DateTimeRelation::Earlier;
  if (time1.m_minute > time2.m_minute)
    return DateTimeRelation::Later;

  if (time1.m_second < time2.m_second)
    return DateTimeRelation::Earlier;
  if (time1.m_second > time2.m_second)
    return DateTimeRelation::Later;

  return DateTimeRelation::Equal;
}

::transit::Date GetDate(std::tm const & tm)
{
  return ::transit::Date(tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);
}

::transit::Time GetTime(std::tm const & tm)
{
  return ::transit::Time(tm.tm_hour, tm.tm_min, tm.tm_sec);
}
}  // namespace

namespace transit
{
// Status ------------------------------------------------------------------------------------------
std::string DebugPrint(Status const & status)
{
  switch (status)
  {
  case Status::Open: return "Open";
  case Status::Closed: return "Closed";
  case Status::Unknown: return "Unknown";
  }
  UNREACHABLE();
}

// DatesInterval -----------------------------------------------------------------------------------
DatesInterval::DatesInterval(gtfs::CalendarItem const & calendarItem)
{
  uint32_t y1 = 0;
  uint32_t m1 = 0;
  uint32_t d1 = 0;
  uint32_t y2 = 0;
  uint32_t m2 = 0;
  uint32_t d2 = 0;

  std::tie(y1, m1, d1) = calendarItem.start_date.get_yyyy_mm_dd();
  std::tie(y2, m2, d2) = calendarItem.end_date.get_yyyy_mm_dd();

  y1 -= kEpochStartYear;
  y2 -= kEpochStartYear;

  uint8_t const yDelta = y2 - y1;

  // From high bit to the least significant bit (from 31 to 0):
  // year1 (4 bits), month1 (4 bits), day1 (5 bits), year delta (3 bits), month2 (4 bits), day2 (5
  // bits), sunday, monday, ..., saturday - 7 bits, each week day is 1 bit.
  m_data = y1 << 28 | m1 << 24 | d1 << 19 | yDelta << 16 | m2 << 12 | d2 << 7 | GetRawStatus(calendarItem.sunday) << 6 |
           GetRawStatus(calendarItem.monday) << 5 | GetRawStatus(calendarItem.tuesday) << 4 |
           GetRawStatus(calendarItem.wednesday) << 3 | GetRawStatus(calendarItem.thursday) << 2 |
           GetRawStatus(calendarItem.friday) << 1 | GetRawStatus(calendarItem.saturday);
}

DatesInterval::DatesInterval(uint32_t data) : m_data(data) {}

std::tuple<Date, Date, WeekSchedule> DatesInterval::Extract() const
{
  Date date1;
  Date date2;

  WeekSchedule week;

  static uint32_t constexpr mask3bits = 0x7;

  date1.m_year = (m_data >> 28) + kEpochStartYear;
  date1.m_month = (m_data >> 24) & kMask4bits;
  date1.m_day = (m_data >> 19) & kMask5bits;
  uint8_t yDelta = (m_data >> 16) & mask3bits;
  date2.m_year = date1.m_year + yDelta;
  date2.m_month = (m_data >> 12) & kMask4bits;
  date2.m_day = (m_data >> 7) & kMask5bits;

  week[WeekDays::Sunday] = m_data & 0x40;
  week[WeekDays::Monday] = m_data & 0x20;
  week[WeekDays::Tuesday] = m_data & 0x10;
  week[WeekDays::Wednesday] = m_data & 0x8;
  week[WeekDays::Thursday] = m_data & 0x4;
  week[WeekDays::Friday] = m_data & 0x2;
  week[WeekDays::Saturday] = m_data & 0x1;

  return {date1, date2, week};
}

Status DatesInterval::GetStatusInInterval(Date const & date, uint8_t wdIndex) const
{
  auto const & [date1, date2, wd] = Extract();

  if (GetDatesRelation(date, date1) != DateTimeRelation::Earlier &&
      GetDatesRelation(date, date2) != DateTimeRelation::Later)
  {
    return wd[wdIndex] ? Status::Open : Status::Closed;
  }

  return Status::Closed;
}

bool DatesInterval::operator==(DatesInterval const & rhs) const
{
  return m_data == rhs.m_data;
}

// DateException -----------------------------------------------------------------------------------
DateException::DateException(gtfs::Date const & date, gtfs::CalendarDateException const & dateException)
{
  uint32_t y = 0;
  uint32_t m = 0;
  uint32_t d = 0;

  std::tie(y, m, d) = date.get_yyyy_mm_dd();

  y -= kEpochStartYear;

  // From high bit - 1 to the least significant bit (from 14 to 0):
  // year1 (4 bits), month1 (4 bits), day1 (5 bits), exctption statys (1 bit).
  m_data = y << 10 | m << 6 | d << 1 | GetRawStatus(dateException);
}

DateException::DateException(uint16_t data) : m_data(data) {}

bool DateException::operator==(DateException const & rhs) const
{
  return m_data == rhs.m_data;
}

std::tuple<Date, bool> DateException::Extract() const
{
  Date date;

  date.m_year = (m_data >> 10) + kEpochStartYear;
  date.m_month = (m_data >> 6) & kMask4bits;
  date.m_day = (m_data >> 1) & kMask5bits;
  bool const isOpen = m_data & 0x1;

  return {date, isOpen};
}

Status DateException::GetExceptionStatus(Date const & date) const
{
  auto const & [dateExc, isOpen] = Extract();

  if (GetDatesRelation(date, dateExc) == DateTimeRelation::Equal)
    return isOpen ? Status::Open : Status::Closed;

  return Status::Unknown;
}

// TimeInterval ------------------------------------------------------------------------------------
TimeInterval::TimeInterval(gtfs::Time const & startTime, gtfs::Time const & endTime)
{
  uint64_t h1 = 0;
  uint64_t m1 = 0;
  uint64_t s1 = 0;

  uint64_t h2 = 0;
  uint64_t m2 = 0;
  uint64_t s2 = 0;

  std::tie(h1, m1, s1) = startTime.get_hh_mm_ss();
  std::tie(h2, m2, s2) = endTime.get_hh_mm_ss();

  // From 33 bit to 0 bit:
  // hour1 (5 bits), minute1 (6 bits), second1 (6 bits), hour2 (5 bits), minute2 (6 bits), second2
  // (6 bits).
  m_data = h1 << 29 | m1 << 23 | s1 << 17 | h2 << 12 | m2 << 6 | s2;
}

TimeInterval::TimeInterval(uint64_t data) : m_data(data) {}

bool TimeInterval::operator<(TimeInterval const & rhs) const
{
  return m_data < rhs.m_data;
}

bool TimeInterval::operator==(TimeInterval const & rhs) const
{
  return m_data == rhs.m_data;
}

std::pair<Time, Time> TimeInterval::Extract() const
{
  Time startTime;
  Time endTime;

  startTime.m_hour = (m_data >> 29) & kMask5bits;
  startTime.m_minute = (m_data >> 23) & kMask6bits;
  startTime.m_second = (m_data >> 17) & kMask6bits;

  endTime.m_hour = (m_data >> 12) & kMask5bits;
  endTime.m_minute = (m_data >> 6) & kMask6bits;
  endTime.m_second = m_data & kMask6bits;

  return {startTime, endTime};
}

Status TimeInterval::GetTimeStatus(Time const & time) const
{
  auto const & [startTime, endTime] = Extract();
  if (GetTimesRelation(time, startTime) != DateTimeRelation::Earlier &&
      GetTimesRelation(time, endTime) != DateTimeRelation::Later)
  {
    return Status::Open;
  }

  return Status::Closed;
}

// FrequencyIntervals ------------------------------------------------------------------------------
FrequencyIntervals::FrequencyIntervals(gtfs::Frequencies const & frequencies)
{
  for (auto const & freq : frequencies)
    if (freq.headway_secs > 0)
      m_intervals.emplace(TimeInterval(freq.start_time, freq.end_time), freq.headway_secs);
    else
      LOG(LINFO, ("Bad headway_secs:", freq.headway_secs));
}

bool FrequencyIntervals::operator==(FrequencyIntervals const & rhs) const
{
  return m_intervals == rhs.m_intervals;
}

void FrequencyIntervals::AddInterval(TimeInterval const & timeInterval, Frequency frequency)
{
  m_intervals[timeInterval] = frequency;
}

Frequency FrequencyIntervals::GetFrequency(Time const & time) const
{
  for (auto const & [interval, freq] : m_intervals)
    if (interval.GetTimeStatus(time) == Status::Open)
      return freq;

  return kDefaultFrequency;
}

std::map<TimeInterval, Frequency> const & FrequencyIntervals::GetFrequencies() const
{
  return m_intervals;
}

// Schedule ----------------------------------------------------------------------------------------
bool Schedule::operator==(Schedule const & rhs) const
{
  return m_serviceIntervals == rhs.m_serviceIntervals && m_serviceExceptions == rhs.m_serviceExceptions &&
         m_defaultFrequency == rhs.m_defaultFrequency;
}

void Schedule::AddDatesInterval(gtfs::CalendarItem const & calendarItem, gtfs::Frequencies const & frequencies)
{
  m_serviceIntervals.emplace(DatesInterval(calendarItem), FrequencyIntervals(frequencies));
}

void Schedule::AddDateException(gtfs::Date const & date, gtfs::CalendarDateException const & dateException,
                                gtfs::Frequencies const & frequencies)
{
  m_serviceExceptions.emplace(DateException(date, dateException), FrequencyIntervals(frequencies));
}

DatesIntervals const & Schedule::GetServiceIntervals() const
{
  return m_serviceIntervals;
}

DatesExceptions const & Schedule::GetServiceExceptions() const
{
  return m_serviceExceptions;
}

void Schedule::AddDatesInterval(DatesInterval const & interval, FrequencyIntervals const & frequencies)
{
  m_serviceIntervals[interval] = frequencies;
}

void Schedule::AddDateException(DateException const & dateException, FrequencyIntervals const & frequencies)
{
  m_serviceExceptions[dateException] = frequencies;
}

Status Schedule::GetStatus(time_t const & time) const
{
  auto const & [date, wdIndex] = GetDateAndWeekIndex(time);

  for (auto const & [dateException, freq] : m_serviceExceptions)
  {
    Status const & status = dateException.GetExceptionStatus(date);

    if (status != Status::Unknown)
      return status;
  }

  Status res = Status::Unknown;

  for (auto const & [datesInterval, freq] : m_serviceIntervals)
  {
    Status const & status = datesInterval.GetStatusInInterval(date, wdIndex);

    if (status != Status::Unknown)
      res = status;

    if (res == Status::Open)
      return res;
  }

  return res;
}

Frequency Schedule::GetFrequency(time_t const & time) const
{
  auto const & [date, timeHms, wdIndex] = GetDateTimeAndWeekIndex(time);

  for (auto const & [dateException, freqInts] : m_serviceExceptions)
    if (dateException.GetExceptionStatus(date) == Status::Open)
      return freqInts.GetFrequency(timeHms);

  for (auto const & [datesInterval, freqInts] : m_serviceIntervals)
    if (datesInterval.GetStatusInInterval(date, wdIndex) == Status::Open)
      return freqInts.GetFrequency(timeHms);

  LOG(LWARNING, ("No frequency for date", date, "time", timeHms));
  return m_defaultFrequency;
}

std::pair<Date, uint8_t> Schedule::GetDateAndWeekIndex(time_t const & time) const
{
  std::tm const tm = ToCalendarTime(time);
  return {GetDate(tm), tm.tm_wday};
}

std::tuple<Date, Time, uint8_t> Schedule::GetDateTimeAndWeekIndex(time_t const & time) const
{
  std::tm const tm = ToCalendarTime(time);
  return {GetDate(tm), GetTime(tm), tm.tm_wday};
}
}  // namespace transit
