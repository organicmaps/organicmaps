#include "testing/testing.hpp"

#include "map/gps_track_collection.hpp"

#include "geometry/latlon.hpp"

#include <map>
#include <utility>

namespace gps_track_collection_test
{
location::GpsInfo MakeGpsTrackInfo(double timestamp, ms::LatLon const & ll, double speed)
{
  location::GpsInfo info;
  info.m_timestamp = timestamp;
  info.m_speed = speed;
  info.m_latitude = ll.m_lat;
  info.m_longitude = ll.m_lon;
  return info;
}

UNIT_TEST(GpsTrackCollection_Simple)
{
  GpsTrackCollection collection;

  std::map<size_t, location::GpsInfo> data;

  for (size_t i = 0; i < 50; ++i)
  {
    auto const info = MakeGpsTrackInfo(100.0 + i, ms::LatLon(-25.0 + i, -49.0 + i), i);
    std::pair<size_t, size_t> addedIds = collection.Add({info});
    TEST_EQUAL(addedIds.second, i, ());
    data[addedIds.second] = info;
  }

  TEST_EQUAL(50, collection.GetSize(), ());

  collection.ForEach([&data](location::GpsInfo const & info, size_t id) -> bool
  {
    TEST(data.end() != data.find(id), ());
    location::GpsInfo const & originInfo = data[id];
    TEST_EQUAL(info.m_latitude, originInfo.m_latitude, ());
    TEST_EQUAL(info.m_longitude, originInfo.m_longitude, ());
    TEST_EQUAL(info.m_speed, originInfo.m_speed, ());
    TEST_EQUAL(info.m_timestamp, originInfo.m_timestamp, ());
    return true;
  });

  auto res = collection.Clear();
  TEST_EQUAL(res.first, 0, ());
  TEST_EQUAL(res.second, 49, ());

  TEST_EQUAL(0, collection.GetSize(), ());
}

UNIT_TEST(GpsTrackCollection_DeduplicatesBeforeUpdatingStatsAndIds)
{
  GpsTrackCollection collection;
  auto start = MakeGpsTrackInfo(1.0, {0.0, 180.0}, 0.0);
  start.m_verticalAccuracy = 1.0;
  auto repeat = MakeGpsTrackInfo(2.0, {0.0, -179.999999}, 0.0);
  repeat.m_altitude = 100.0;
  auto const invalidRange = std::pair{GpsTrackCollection::kInvalidId, GpsTrackCollection::kInvalidId};
  TEST_EQUAL(collection.Add({start}), (std::pair<size_t, size_t>{0, 0}), ());
  TEST_EQUAL(collection.Add({repeat}), invalidRange, ());
  TEST_EQUAL(collection.GetSize(), 1, ());
  TEST_EQUAL(collection.GetTrackStatistics().m_length, 0.0, ());
  TEST_EQUAL(collection.GetTrackStatistics().m_duration, 0.0, ());

  auto next = MakeGpsTrackInfo(3.0, {0.0, -179.99998}, 1.0);
  next.m_verticalAccuracy = 1.0;
  next.m_altitude = 100;
  TEST_EQUAL(collection.Add({next}), (std::pair<size_t, size_t>{1, 1}), ());
  TEST(collection.GetTrackStatistics().HasGpsElevationProfile(), ());
  TEST_EQUAL(collection.UpdateAndGetElevationInfo().GetSize(), 2, ());

  auto const gap = MakeGpsTrackInfo(4.0, {0.0, -179.99996}, 1.0);
  auto back = MakeGpsTrackInfo(5.0, {0.0, 180.0}, 1.0);
  back.m_verticalAccuracy = 1.0;
  TEST_EQUAL(collection.Add({gap, back}), (std::pair<size_t, size_t>{2, 3}), ());
  TEST_EQUAL(collection.GetSize(), 4, ());
  TEST(!collection.GetTrackStatistics().HasGpsElevationProfile(), ());
  TEST_EQUAL(collection.GetTrackStatistics().m_duration, 4.0, ());
  TEST(collection.UpdateAndGetElevationInfo().IsEmpty(), ());

  collection.Clear();
  TEST_EQUAL(collection.GetSize(), 0, ());
  collection.Add({start, next});
  TEST(collection.GetTrackStatistics().HasGpsElevationProfile(), ());
  TEST_EQUAL(collection.UpdateAndGetElevationInfo().GetSize(), 2, ());
}

UNIT_TEST(GpsTrackCollection_UsesSavedAltitudeDisplayRules)
{
  GpsTrackCollection collection;
  auto start = MakeGpsTrackInfo(1, {0, 19}, 1);
  auto finish = MakeGpsTrackInfo(2, {0, 19.001}, 1);
  start.m_verticalAccuracy = finish.m_verticalAccuracy = 1;
  collection.Add({start, finish});
  TEST(!collection.GetTrackStatistics().HasGpsElevationProfile(), ());
  TEST(collection.UpdateAndGetElevationInfo().IsEmpty(), ());

  collection.Clear();
  start.m_altitude = 100.9;
  finish.m_altitude = 101.1;
  collection.Add({start, finish});
  auto const & statistics = collection.GetTrackStatistics();
  TEST(statistics.HasGpsElevationProfile(), ());
  TEST_EQUAL(statistics.m_minElevation, 101, ());
  TEST_EQUAL(statistics.m_maxElevation, 101, ());
  auto const & points = collection.UpdateAndGetElevationInfo().GetLines().front();
  TEST_EQUAL(points.size(), 2, ());
  for (auto const & point : points)
    TEST_EQUAL(point.m_altitude, 101, ());
}
}  // namespace gps_track_collection_test
