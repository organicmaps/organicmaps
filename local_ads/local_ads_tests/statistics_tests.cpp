#include "testing/testing.hpp"

#include "local_ads/statistics.hpp"

#include "coding/file_name_utils.hpp"

namespace
{
class StatisticsGuard
{
public:
  StatisticsGuard(local_ads::Statistics & statistics) : m_statistics(statistics) {}
  ~StatisticsGuard()
  {
    m_statistics.CleanupAfterTesting();
  }

private:
  local_ads::Statistics & m_statistics;
};
}  // namespace

using namespace std::chrono;
using ET = local_ads::EventType;
using TS = local_ads::Timestamp;

UNIT_TEST(LocalAdsStatistics_Read_Write_Simple)
{
  local_ads::Statistics statistics;
  StatisticsGuard guard(statistics);

  std::list<local_ads::Event> events;
  // type, mwmVersion, countryId, featureId, zoomLevel, timestamp, latitude, longitude, accuracyInMeters
  events.emplace_back(ET::ShowPoint, 123456, "Moscow", 111, 15, TS(minutes(5)), 30.0, 64.0, 10);
  events.emplace_back(ET::ShowPoint, 123456, "Moscow", 222, 13, TS(minutes(10)), 20.0, 14.0, 20);
  events.emplace_back(ET::OpenInfo, 123456, "Moscow", 111, 17, TS(minutes(15)), 53.0, 54.0, 10000);
  std::string unusedFileName;
  auto unprocessedEvents = statistics.WriteEventsForTesting(events, unusedFileName);
  TEST_EQUAL(unprocessedEvents.size(), 0, ());

  TEST_EQUAL(statistics.ReadEventsForTesting("Moscow_123456.dat"), events, ());
}

UNIT_TEST(LocalAdsStatistics_Write_With_Unprocessed)
{
  local_ads::Statistics statistics;
  StatisticsGuard guard(statistics);

  std::list<local_ads::Event> events;
  events.emplace_back(ET::ShowPoint, 123456, "Moscow", 111, 15, TS(minutes(5)), 0.0, 0.0, 10);
  events.emplace_back(ET::ShowPoint, 123456, "Moscow", 222, 13, TS(minutes(10)), 20.0, 14.0, 20);
  events.emplace_back(ET::OpenInfo, 123456, "Moscow", 111, 17, TS(minutes(15)), 15.0, 14.0, 20);
  std::string fileNameToRebuild;
  auto unprocessedEvents = statistics.WriteEventsForTesting(events, fileNameToRebuild);
  TEST_EQUAL(unprocessedEvents.size(), 0, ());
  TEST(fileNameToRebuild.empty(), ());

  std::list<local_ads::Event> events2;
  events2.emplace_back(ET::ShowPoint, 123456, "Moscow", 333, 15, TS(minutes(1)), 1.0, 89.0, 20);
  events2.emplace_back(ET::ShowPoint, 123456, "Moscow", 444, 15, TS(minutes(20)), 30.0, 13.0, 15);
  auto unprocessedEvents2 = statistics.WriteEventsForTesting(events2, fileNameToRebuild);

  std::list<local_ads::Event> expectedUnprocessedEvents = events2;
  expectedUnprocessedEvents.sort();

  my::GetNameFromFullPath(fileNameToRebuild);
  TEST_EQUAL(fileNameToRebuild, "Moscow_123456.dat", ());
  TEST_EQUAL(expectedUnprocessedEvents, unprocessedEvents2, ());
}

UNIT_TEST(LocalAdsStatistics_Process_With_Rebuild)
{
  local_ads::Statistics statistics;
  StatisticsGuard guard(statistics);

  std::list<local_ads::Event> events;
  events.emplace_back(ET::ShowPoint, 123456, "Moscow", 111, 15, TS(minutes(5)), 50.0, 14.0, 20);
  events.emplace_back(ET::ShowPoint, 123456, "Moscow", 222, 13, TS(minutes(10)), 69.0, 67.0, 100);
  events.emplace_back(ET::OpenInfo, 123456, "Moscow", 111, 17, TS(minutes(15)), 45.0, 80.0, 34);
  std::string unused;
  statistics.WriteEventsForTesting(events, unused);

  TEST_EQUAL(statistics.ReadEventsForTesting("Moscow_123456.dat"), events, ());

  std::list<local_ads::Event> events2;
  events2.emplace_back(ET::ShowPoint, 123456, "Moscow", 333, 15, TS(minutes(1)), 20.0, 14.0, 12);
  events2.emplace_back(ET::ShowPoint, 123456, "Moscow", 444, 15, TS(minutes(20)), 30.0, 56.0, 3535);

  statistics.ProcessEventsForTesting(events2);

  std::list<local_ads::Event> expectedResult = events;
  expectedResult.insert(expectedResult.end(), events2.begin(), events2.end());
  expectedResult.sort();

  TEST_EQUAL(statistics.ReadEventsForTesting("Moscow_123456.dat"), expectedResult, ());
}

UNIT_TEST(LocalAdsStatistics_Process_With_Clipping)
{
  local_ads::Statistics statistics;
  StatisticsGuard guard(statistics);

  std::list<local_ads::Event> events;
  events.emplace_back(ET::ShowPoint, 123456, "Moscow", 111, 15, TS(minutes(5)), 20.0, 14.0, 12);
  events.emplace_back(ET::ShowPoint, 123456, "Moscow", 222, 13, TS(minutes(10)), 69.0, 67.0, 100);
  events.emplace_back(ET::OpenInfo, 123456, "Moscow", 111, 17, TS(minutes(25 * 60 + 15)), 20.0,
                      14.0, 20);
  std::string unused;
  statistics.WriteEventsForTesting(events, unused);

  TEST_EQUAL(statistics.ReadEventsForTesting("Moscow_123456.dat"), events, ());

  std::list<local_ads::Event> events2;
  events2.emplace_back(ET::ShowPoint, 123456, "Moscow", 333, 15, TS(minutes(24 * 183 * 60 + 50)),
                       20.0, 14.0, 20);

  statistics.ProcessEventsForTesting(events2);

  std::list<local_ads::Event> expectedResult;
  expectedResult.push_back(local_ads::Event(events.back()));
  expectedResult.insert(expectedResult.end(), events2.begin(), events2.end());
  expectedResult.sort();

  TEST_EQUAL(statistics.ReadEventsForTesting("Moscow_123456.dat"), expectedResult, ());
}

UNIT_TEST(LocalAdsStatistics_Process_Complex)
{
  local_ads::Statistics statistics;
  StatisticsGuard guard(statistics);

  std::list<local_ads::Event> events;
  events.emplace_back(ET::ShowPoint, 123456, "Moscow", 111, 15, TS(minutes(5)), 20.0, 14.0, 20);
  events.emplace_back(ET::ShowPoint, 123456, "Minsk", 222, 13, TS(minutes(10)), 30.0, 14.0, 20);
  events.emplace_back(ET::OpenInfo, 123456, "Minsk", 111, 17, TS(minutes(25)), 40.0, 14.0, 20);
  events.emplace_back(ET::OpenInfo, 123456, "Minsk", 111, 17, TS(minutes(25 * 60 + 15)), 20.0, 14.0,
                      20);
  std::string unused;
  statistics.WriteEventsForTesting(events, unused);

  std::list<local_ads::Event> expectedResult1;
  expectedResult1.push_back(local_ads::Event(events.front()));
  TEST_EQUAL(statistics.ReadEventsForTesting("Moscow_123456.dat"), expectedResult1, ());

  std::list<local_ads::Event> expectedResult2 = events;
  expectedResult2.erase(expectedResult2.begin());
  TEST_EQUAL(statistics.ReadEventsForTesting("Minsk_123456.dat"), expectedResult2, ());

  std::list<local_ads::Event> events2;
  events2.emplace_back(ET::ShowPoint, 123456, "Moscow", 333, 15, TS(minutes(100)), 20.0, 14.0, 20);
  events2.emplace_back(ET::ShowPoint, 123456, "Minsk", 333, 15, TS(minutes(24 * 183 * 60 + 50)),
                       20.0, 14.0, 20);

  statistics.ProcessEventsForTesting(events2);

  expectedResult1.push_back(local_ads::Event(events2.front()));
  TEST_EQUAL(statistics.ReadEventsForTesting("Moscow_123456.dat"), expectedResult1, ());

  expectedResult2.clear();
  expectedResult2.push_back(local_ads::Event(events.back()));
  expectedResult2.push_back(local_ads::Event(events2.back()));
  TEST_EQUAL(statistics.ReadEventsForTesting("Minsk_123456.dat"), expectedResult2, ());
}
