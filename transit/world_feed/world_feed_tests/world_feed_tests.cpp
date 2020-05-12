#include "testing/testing.hpp"

#include "transit/world_feed/date_time_helpers.hpp"
#include "transit/world_feed/feed_helpers.hpp"

#include "base/assert.hpp"

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

#include "3party/just_gtfs/just_gtfs.h"
#include "3party/opening_hours/opening_hours.hpp"

using namespace transit;

namespace
{
std::vector<gtfs::CalendarAvailability> GetCalendarAvailability(std::vector<size_t> const & data)
{
  CHECK_EQUAL(data.size(), 7, ());
  std::vector<gtfs::CalendarAvailability> res;

  for (auto val : data)
  {
    if (val == 0)
      res.push_back(gtfs::CalendarAvailability::NotAvailable);
    else
      res.push_back(gtfs::CalendarAvailability::Available);
  }

  return res;
}

gtfs::StopTimes GetFakeStopTimes(std::vector<std::string> const & transitIds)
{
  auto ids = transitIds;
  std::sort(ids.begin(), ids.end());
  gtfs::StopTimes res;
  for (size_t i = 0; i < ids.size(); ++i)
  {
    gtfs::StopTime st;
    st.trip_id = ids[i];
    st.stop_sequence = i;
    res.emplace_back(st);
  }
  return res;
}

void TestInterval(WeekdaysInterval const & interval, size_t start, size_t end,
                  osmoh::RuleSequence::Modifier status)
{
  TEST_EQUAL(interval.m_start, start, ());
  TEST_EQUAL(interval.m_end, end, ());
  TEST_EQUAL(interval.m_status, status, ());
}

void TestExceptionIntervals(gtfs::CalendarDates const & dates, size_t intervalsCount,
                            std::string const & resOpeningHoursStr)
{
  osmoh::TRuleSequences rules;
  GetServiceDaysExceptionsOsmoh(dates, rules);
  // TEST_EQUAL(rules.size(), intervalsCount, ());
  auto const openingHours = ToString(osmoh::OpeningHours(rules));
  TEST_EQUAL(openingHours, resOpeningHoursStr, ());
}

void TestPlanFact(size_t planIndex, bool planInsert, std::pair<size_t, bool> const & factRes)
{
  TEST_EQUAL(factRes.first, planIndex, ());
  TEST_EQUAL(factRes.second, planInsert, ());
}

UNIT_TEST(Transit_GTFS_OpenCloseInterval1)
{
  auto const & intervals = GetOpenCloseIntervals(GetCalendarAvailability({1, 1, 1, 1, 1, 0, 0}));
  TEST_EQUAL(intervals.size(), 2, ());

  TestInterval(intervals[0], 0, 4, osmoh::RuleSequence::Modifier::DefaultOpen);
  TestInterval(intervals[1], 5, 6, osmoh::RuleSequence::Modifier::Closed);
}

UNIT_TEST(Transit_GTFS_OpenCloseInterval2)
{
  auto const & intervals = GetOpenCloseIntervals(GetCalendarAvailability({0, 0, 0, 0, 0, 1, 0}));
  TEST_EQUAL(intervals.size(), 3, ());

  TestInterval(intervals[0], 0, 4, osmoh::RuleSequence::Modifier::Closed);
  TestInterval(intervals[1], 5, 5, osmoh::RuleSequence::Modifier::DefaultOpen);
  TestInterval(intervals[2], 6, 6, osmoh::RuleSequence::Modifier::Closed);
}

UNIT_TEST(Transit_GTFS_OpenCloseInterval3)
{
  auto const & intervals = GetOpenCloseIntervals(GetCalendarAvailability({0, 0, 0, 0, 0, 0, 1}));
  TEST_EQUAL(intervals.size(), 2, ());

  TestInterval(intervals[0], 0, 5, osmoh::RuleSequence::Modifier::Closed);
  TestInterval(intervals[1], 6, 6, osmoh::RuleSequence::Modifier::DefaultOpen);
}

UNIT_TEST(Transit_GTFS_GetTimeOsmoh)
{
  size_t const hours = 21;
  size_t const minutes = 5;
  size_t const seconds = 30;
  gtfs::Time const timeGtfs(hours, minutes, seconds);

  auto const timeOsmoh = GetTimeOsmoh(timeGtfs);
  TEST_EQUAL(timeOsmoh.GetMinutesCount(), minutes, ());
  TEST_EQUAL(timeOsmoh.GetHoursCount(), hours, ());
}

UNIT_TEST(Transit_GTFS_ServiceDaysExceptions1)
{
  gtfs::CalendarDates const exceptionDays{
      {"serviceId1", gtfs::Date(2015, 01, 30), gtfs::CalendarDateException::Removed},
      {"serviceId1", gtfs::Date(2015, 01, 31), gtfs::CalendarDateException::Removed},
      {"serviceId1", gtfs::Date(2015, 02, 01), gtfs::CalendarDateException::Removed},
      {"serviceId1", gtfs::Date(2015, 04, 03), gtfs::CalendarDateException::Added}};
  TestExceptionIntervals(
      exceptionDays, 2 /* intervalsCount */,
      "2015 Apr 03-2015 Apr 03; 2015 Jan 30-2015 Feb 01 closed" /* resOpeningHoursStr */);
}

UNIT_TEST(Transit_GTFS_ServiceDaysExceptions2)
{
  gtfs::CalendarDates const exceptionDays{
      {"serviceId2", gtfs::Date(1999, 11, 14), gtfs::CalendarDateException::Removed}};
  TestExceptionIntervals(exceptionDays, 1 /* intervalsCount */,
                         "1999 Nov 14-1999 Nov 14 closed" /* resOpeningHoursStr */);
}

UNIT_TEST(Transit_GTFS_ServiceDaysExceptions3)
{
  gtfs::CalendarDates const exceptionDays{
      {"serviceId2", gtfs::Date(2005, 8, 01), gtfs::CalendarDateException::Added},
      {"serviceId2", gtfs::Date(2005, 8, 12), gtfs::CalendarDateException::Added},
      {"serviceId2", gtfs::Date(2005, 10, 11), gtfs::CalendarDateException::Removed},
      {"serviceId2", gtfs::Date(2005, 10, 12), gtfs::CalendarDateException::Removed},
      {"serviceId2", gtfs::Date(2005, 10, 13), gtfs::CalendarDateException::Added},
      {"serviceId2", gtfs::Date(1999, 10, 14), gtfs::CalendarDateException::Removed}};
  TestExceptionIntervals(
      exceptionDays, 2 /* intervalsCount */,
      "2005 Aug 01-2005 Aug 01, 2005 Aug 12-2005 Aug 12, 2005 Oct 13-2005 Oct 13; 2005 Oct 11-2005 "
      "Oct 12, 1999 Oct 14-1999 Oct 14 closed" /* resOpeningHoursStr */);
}

UNIT_TEST(Transit_GTFS_FindStopTimesByTransitId)
{
  auto const allStopTimes = GetFakeStopTimes({"4", "5", "6", "2", "10", "2", "2", "6"});
  auto const stopTimes1 = GetStopTimesForTrip(allStopTimes, "2");
  TEST_EQUAL(stopTimes1.size(), 3, ());

  auto const stopTimes10 = GetStopTimesForTrip(allStopTimes, "10");
  TEST_EQUAL(stopTimes10.size(), 1, ());

  auto const stopTimes6 = GetStopTimesForTrip(allStopTimes, "6");
  TEST_EQUAL(stopTimes6.size(), 2, ());

  auto const stopTimesNonExistent1 = GetStopTimesForTrip(allStopTimes, "11");
  TEST(stopTimesNonExistent1.empty(), ());

  auto const stopTimesNonExistent2 = GetStopTimesForTrip(allStopTimes, "1");
  TEST(stopTimesNonExistent1.empty(), ());
}

UNIT_TEST(Transit_GTFS_FindStopTimesByTransitId2)
{
  auto const allStopTimes = GetFakeStopTimes({"28", "28", "28", "28"});
  auto const stopTimes = GetStopTimesForTrip(allStopTimes, "28");
  TEST_EQUAL(stopTimes.size(), 4, ());

  auto const stopTimesNonExistent = GetStopTimesForTrip(allStopTimes, "3");
  TEST(stopTimesNonExistent.empty(), ());
}

// Stops are marked as *, points on polyline as +. Points have indexes, stops have letters.
//
//    *A
//
//  +----+---------------+----------------------+
//  0    1               2                      3
//
//                                          *B  *C
//
UNIT_TEST(Transit_GTFS_ProjectStopToLine_Simple)
{
  double const y = 0.0002;
  std::vector<m2::PointD> shape{{0.001, y}, {0.0015, y}, {0.004, y}, {0.005, y}};

  m2::PointD const point_A{0.0012, 0.0003};
  m2::PointD const point_B{0.00499, 0.0001};
  m2::PointD const point_C{0.005, 0.0001};

  // Test that point_A is projected between two existing polyline points and the new point is
  // added in the place of its projection.
  TestPlanFact(1 /* planIndex */, true /* planInsert */,
               PrepareNearestPointOnTrack(point_A, 0 /* startIndex */, shape));

  TEST_EQUAL(shape.size(), 5, ());
  TEST_EQUAL(shape[1 /* expectedIndex */], m2::PointD(point_A.x, y), ());

  // Test that repeated point_A projection to the polyline doesn't lead to the second insertion.
  // Expected point projection index is the same.
  // But this projection is not inserted (it is already present).
  TestPlanFact(1 /* planIndex */, false /* planInsert */,
               PrepareNearestPointOnTrack(point_A, 0 /* startIndex */, shape));
  // So the shape size remains the same.
  TEST_EQUAL(shape.size(), 5, ());

  // Test that point_B insertion leads to addition of the new projection to the shape.
  TestPlanFact(4, true, PrepareNearestPointOnTrack(point_B, 1 /* startIndex */, shape));

  // Test that point_C insertion does not lead to the addition of the new projection.
  TestPlanFact(5, false, PrepareNearestPointOnTrack(point_C, 4 /* startIndex */, shape));
}

// Stop is on approximately the same distance from the segment (0, 1) and segment (1, 2).
// Its projection index and projection coordinate depend on the |startIndex| parameter.
//
// 1 +----------+ 2
//   |
//   |    *A
//   |
// 0 +
//
UNIT_TEST(Transit_GTFS_ProjectStopToLine_DifferentStartIndexes)
{
  std::vector<m2::PointD> const referenceShape{{0.001, 0.001}, {0.001, 0.002}, {0.003, 0.002}};
  m2::PointD const point_A{0.0015, 0.0015};

  // Test for |startIndex| = 0.
  {
    auto shape = referenceShape;
    TestPlanFact(1, true, PrepareNearestPointOnTrack(point_A, 0 /* startIndex */, shape));
    TEST_EQUAL(shape.size(), 4, ());
    TEST_EQUAL(shape[1 /* expectedIndex */], m2::PointD(0.001, point_A.y), ());
  }

  // Test for |startIndex| = 1.
  {
    auto shape = referenceShape;
    TestPlanFact(2, true, PrepareNearestPointOnTrack(point_A, 1 /* startIndex */, shape));
    TEST_EQUAL(shape.size(), 4, ());
    TEST_EQUAL(shape[2 /* expectedIndex */], m2::PointD(point_A.x, 0.002), ());
  }
}

// Real-life example of stop being closer to the other side of the route (4, 5) then to its real
// destination (0, 1).
// We handle this type of situations by using constant max distance of departing from this stop
// on the polyline in |PrepareNearestPointOnTrack()|.
//
//  5                                4
//  +--------------------------------+---------------------------------------+ 3
//                                                                           |
//                        /+-------------------------------------------------+ 2
//      *A               / 1
//                      /
//                     + 0
//
UNIT_TEST(Transit_GTFS_ProjectStopToLine_MaxDistance)
{
  std::vector<m2::PointD> shape{{0.002, 0.001},  {0.003, 0.003},  {0.010, 0.003},
                                {0.010, 0.0031}, {0.005, 0.0031}, {0.001, 0.0031}};
  m2::PointD const point_A{0.0028, 0.0029};
  TestPlanFact(1, true, PrepareNearestPointOnTrack(point_A, 0 /* startIndex */, shape));
}

// Complex shape with multiple points on it and multiple stops for projection.
//
//                   +-----+
//              C*  /       \
//          /+\    /         \  *D
//       + /    \*/           \
//      /                     +
//     /                      |       *E
//    +                       +-----+
//    |                             |
//    |                             |
//    +---+\                  +-----+
//          \                 |
//       B*  +                |
//       A*   \     +---------+
//             +    |
//             |    +
//             +         *F
//
UNIT_TEST(Transit_GTFS_ProjectStopToLine_NearCircle)
{
  std::vector<m2::PointD> shape{
      {0.003, 0.001},   {0.003, 0.0015},  {0.0025, 0.002},  {0.002, 0.0025},  {0.001, 0.0025},
      {0.001, 0.0035},  {0.0015, 0.0045}, {0.0025, 0.005},  {0.0035, 0.0045}, {0.004, 0.0055},
      {0.0055, 0.0055}, {0.0065, 0.0045}, {0.0065, 0.0035}, {0.0075, 0.0035}, {0.0075, 0.0025},
      {0.0065, 0.0025}, {0.0065, 0.0015}, {0.004, 0.0015},  {0.004, 0.001}};

  m2::PointD const point_A{0.0024, 0.0018};
  m2::PointD const point_B{0.002499, 0.00199};
  m2::PointD const point_C{0.0036, 0.0049};
  m2::PointD const point_D{0.0063, 0.005};
  m2::PointD const point_E{0.008, 0.004};
  m2::PointD const point_F{0.0047, 0.0005};
  TestPlanFact(2, true, PrepareNearestPointOnTrack(point_A, 0 /* startIndex */, shape));
  TestPlanFact(3, false, PrepareNearestPointOnTrack(point_B, 2 /* startIndex */, shape));
  TestPlanFact(10, true, PrepareNearestPointOnTrack(point_C, 3 /* startIndex */, shape));
  TestPlanFact(12, false, PrepareNearestPointOnTrack(point_D, 10 /* startIndex */, shape));
  TestPlanFact(14, true, PrepareNearestPointOnTrack(point_E, 12 /* startIndex */, shape));
  TestPlanFact(20, true, PrepareNearestPointOnTrack(point_F, 14 /* startIndex */, shape));
}
}  // namespace
