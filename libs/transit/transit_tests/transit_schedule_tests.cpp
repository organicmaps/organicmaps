#include "testing/testing.hpp"

#include "transit/transit_schedule.hpp"

#include "base/assert.hpp"

#include <array>
#include <cstdint>
#include <string>

#include "3party/just_gtfs/just_gtfs.h"

namespace transit_schedule_tests
{
using namespace ::transit;

// String dates are provided in GTFS date format YYYYMMDD.
// String times are provided in GTFS time format HH:MM:SS.
uint32_t GetYear(std::string const & date)
{
  return std::stoi(date.substr(0, 4));
}

uint32_t GetMonth(std::string const & date)
{
  return std::stoi(date.substr(4, 2));
}

uint32_t GetDay(std::string const & date)
{
  return std::stoi(date.substr(6));
}

uint32_t GetHour(std::string const & time)
{
  return std::stoi(time.substr(0, 2));
}

uint32_t GetMinute(std::string const & time)
{
  return std::stoi(time.substr(3, 2));
}

uint32_t GetSecond(std::string const & time)
{
  return std::stoi(time.substr(6));
}

gtfs::Frequency GetFrequency(std::string const & startTime, std::string const & endTime, Frequency headwayS)
{
  gtfs::Frequency freq;
  freq.start_time = gtfs::Time(startTime);
  freq.end_time = gtfs::Time(endTime);
  freq.headway_secs = headwayS;

  return freq;
}

gtfs::CalendarAvailability GetAvailability(bool available)
{
  return available ? gtfs::CalendarAvailability::Available : gtfs::CalendarAvailability::NotAvailable;
}

void TestDatesInterval(std::string const & date1, std::string const & date2, WeekSchedule weekDays)
{
  CHECK_EQUAL(date1.size(), 8, ());
  CHECK_EQUAL(date2.size(), 8, ());

  gtfs::CalendarItem ci;

  ci.start_date = gtfs::Date(date1);
  ci.end_date = gtfs::Date(date2);

  ci.sunday = GetAvailability(weekDays[0]);
  ci.monday = GetAvailability(weekDays[1]);
  ci.tuesday = GetAvailability(weekDays[2]);
  ci.wednesday = GetAvailability(weekDays[3]);
  ci.thursday = GetAvailability(weekDays[4]);
  ci.friday = GetAvailability(weekDays[5]);
  ci.saturday = GetAvailability(weekDays[6]);

  DatesInterval const interval(ci);

  auto const & [intervalDate1, intervalDate2, wd] = interval.Extract();

  TEST_EQUAL(intervalDate1.m_year, GetYear(date1), ());
  TEST_EQUAL(intervalDate1.m_month, GetMonth(date1), ());
  TEST_EQUAL(intervalDate1.m_day, GetDay(date1), ());

  TEST_EQUAL(intervalDate2.m_year, GetYear(date2), ());
  TEST_EQUAL(intervalDate2.m_month, GetMonth(date2), ());
  TEST_EQUAL(intervalDate2.m_day, GetDay(date2), ());

  for (size_t i = 0; i < wd.size(); ++i)
    TEST_EQUAL(wd[i], weekDays[i], ());
}

void TestDateException(std::string const & date, bool isOpen)
{
  gtfs::CalendarDateException const ex =
      isOpen ? gtfs::CalendarDateException::Added : gtfs::CalendarDateException::Removed;

  DateException const dateException(gtfs::Date(date), ex);

  auto const & [exDate, exStatus] = dateException.Extract();

  TEST_EQUAL(exDate.m_year, GetYear(date), ());
  TEST_EQUAL(exDate.m_month, GetMonth(date), ());
  TEST_EQUAL(exDate.m_day, GetDay(date), ());

  TEST_EQUAL(exStatus, isOpen, ());
}

void TestTime(std::string const & timePlan, Time const & timeFact)
{
  TEST_EQUAL(timeFact.m_hour, GetHour(timePlan), ());
  TEST_EQUAL(timeFact.m_minute, GetMinute(timePlan), ());
  TEST_EQUAL(timeFact.m_second, GetSecond(timePlan), ());
}

void TestTimeInterval(std::string const & time1, std::string const & time2)
{
  CHECK_EQUAL(time1.size(), 8, ());
  CHECK_EQUAL(time2.size(), 8, ());

  auto const & [startTime, endTime] = TimeInterval(gtfs::Time(time1), gtfs::Time(time2)).Extract();

  TestTime(time1, startTime);
  TestTime(time2, endTime);
}

UNIT_TEST(TransitSchedule_DatesInterval)
{
  TestDatesInterval("20200902", "20210531",
                    {
                        false /* sunday */, true /* monday */, false /* tuesday */, true /* wednesday */,
                        false /* thursday */, false /* friday */, false /* saturday */
                    });

  TestDatesInterval("20201101", "20201130",
                    {
                        true /* sunday */, false /* monday */, false /* tuesday */, false /* wednesday */,
                        false /* thursday */, false /* friday */, true /* saturday */
                    });

  TestDatesInterval("20210101", "20210228",
                    {
                        false /* sunday */, true /* monday */, true /* tuesday */, true /* wednesday */,
                        true /* thursday */, true /* friday */, false /* saturday */
                    });

  TestDatesInterval("20220101", "20240101",
                    {
                        false /* sunday */, false /* monday */, false /* tuesday */, false /* wednesday */,
                        true /* thursday */, true /* friday */, true /* saturday */
                    });
}

UNIT_TEST(TransitSchedule_DateException)
{
  TestDateException("20210316", true);
  TestDateException("20210101", false);
  TestDateException("20221231", true);
  TestDateException("20221231", false);
}

UNIT_TEST(TransitSchedule_TimeInterval)
{
  TestTimeInterval("14:30:00", "14:30:30");
  TestTimeInterval("00:00:00", "21:45:40");
  TestTimeInterval("07:10:30", "11:25:00");
  TestTimeInterval("13:00:00", "13:50:00");
  TestTimeInterval("23:40:50", "23:59:59");
}

UNIT_TEST(TransitSchedule_FrequencyIntervals)
{
  gtfs::Frequencies const frequencies{GetFrequency("14:40:00", "15:55:30", 600),
                                      GetFrequency("15:55:40", "17:00:00", 1200),
                                      GetFrequency("21:10:20", "22:45:40", 300)};

  FrequencyIntervals const intervals(frequencies);

  TEST_EQUAL(intervals.GetFrequency(Time(13, 15, 30)), kDefaultFrequency, ());

  TEST_EQUAL(intervals.GetFrequency(Time(14, 55, 0)), 600, ());
  TEST_EQUAL(intervals.GetFrequency(Time(15, 0, 55)), 600, ());

  TEST_EQUAL(intervals.GetFrequency(Time(15, 55, 39)), kDefaultFrequency, ());
  TEST_EQUAL(intervals.GetFrequency(Time(15, 55, 40)), 1200, ());
  TEST_EQUAL(intervals.GetFrequency(Time(16, 0, 0)), 1200, ());

  TEST_EQUAL(intervals.GetFrequency(Time(21, 50, 0)), 300, ());
  TEST_EQUAL(intervals.GetFrequency(Time(22, 14, 20)), 300, ());
  TEST_EQUAL(intervals.GetFrequency(Time(22, 50, 10)), kDefaultFrequency, ());
}

UNIT_TEST(TransitSchedule_Schedule_DatesInterval_Status)
{
  gtfs::CalendarItem calendarItem1;
  calendarItem1.start_date = gtfs::Date("20200401");
  calendarItem1.end_date = gtfs::Date("20201120");
  calendarItem1.friday = gtfs::CalendarAvailability::Available;
  calendarItem1.saturday = gtfs::CalendarAvailability::Available;

  gtfs::CalendarItem calendarItem2;
  calendarItem2.start_date = gtfs::Date("20201121");
  calendarItem2.end_date = gtfs::Date("20201231");
  calendarItem2.wednesday = gtfs::CalendarAvailability::Available;

  gtfs::Frequencies const frequencies1{GetFrequency("12:10:30", "16:00:40", 4500),
                                       GetFrequency("18:00:00", "19:00:00", 1800)};

  gtfs::Frequencies const frequencies2{GetFrequency("07:25:00", "12:10:00", 2700)};

  Schedule busSchedule;
  busSchedule.AddDatesInterval(calendarItem1, frequencies1);
  busSchedule.AddDatesInterval(calendarItem2, frequencies2);

  // 11.04.2020, saturday.
  TEST_EQUAL(busSchedule.GetStatus(time_t(1586600488)), Status::Open, ());
  // 12.04.2020, sunday.
  TEST_EQUAL(busSchedule.GetStatus(time_t(1586686888)), Status::Closed, ());
  // 07.08.2020, friday.
  TEST_EQUAL(busSchedule.GetStatus(time_t(1596795688)), Status::Open, ());
  // 08.08.2020, saturday.
  TEST_EQUAL(busSchedule.GetStatus(time_t(1596882088)), Status::Open, ());
  // 09.08.2020, sunday.
  TEST_EQUAL(busSchedule.GetStatus(time_t(1596968488)), Status::Closed, ());
}

UNIT_TEST(TransitSchedule_Schedule_DateException_Status)
{
  gtfs::Frequencies const emptyFrequencies;

  Schedule busSchedule;
  busSchedule.AddDateException(gtfs::Date("20200606"), gtfs::CalendarDateException::Added, emptyFrequencies);
  busSchedule.AddDateException(gtfs::Date("20200607"), gtfs::CalendarDateException::Removed, emptyFrequencies);
  busSchedule.AddDateException(gtfs::Date("20211029"), gtfs::CalendarDateException::Added, emptyFrequencies);
  busSchedule.AddDateException(gtfs::Date("20211128"), gtfs::CalendarDateException::Removed, emptyFrequencies);

  // 06.06.2020.
  TEST(busSchedule.GetStatus(time_t(1591438888)) == Status::Open, ());
  // 07.06.2020.
  TEST(busSchedule.GetStatus(time_t(1591525288)) == Status::Closed, ());
  // 29.10.2021.
  TEST(busSchedule.GetStatus(time_t(1635502888)) == Status::Open, ());
  // 28.11.2021.
  TEST(busSchedule.GetStatus(time_t(1638094888)) == Status::Closed, ());
  // 01.03.2021.
  TEST(busSchedule.GetStatus(time_t(1614594088)) == Status::Unknown, ());
  // 01.01.2020.
  TEST(busSchedule.GetStatus(time_t(1577874088)) == Status::Unknown, ());
}
}  // namespace transit_schedule_tests
