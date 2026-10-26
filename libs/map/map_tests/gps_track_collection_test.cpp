#include "testing/testing.hpp"

#include "map/gps_track_collection.hpp"
#include "map/track.hpp"

#include "geometry/latlon.hpp"
#include "geometry/mercator.hpp"

#include "base/logging.hpp"

#include <algorithm>
#include <chrono>
#include <ctime>
#include <initializer_list>
#include <map>
#include <utility>
#include <vector>

namespace gps_track_collection_test
{
using namespace std::chrono;

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
  time_t const t = system_clock::to_time_t(system_clock::now());
  double const timestamp = t;
  LOG(LINFO, ("Timestamp", ctime(&t), timestamp));

  GpsTrackCollection collection;

  std::map<size_t, location::GpsInfo> data;

  for (size_t i = 0; i < 50; ++i)
  {
    auto info = MakeGpsTrackInfo(timestamp + i, ms::LatLon(-90 + i, -180 + i), i);
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

namespace
{
std::vector<location::GpsInfo> MakeElevationPoints(std::initializer_list<double> altitudes)
{
  std::vector<location::GpsInfo> points;
  for (auto const altitude : altitudes)
  {
    auto point = MakeGpsTrackInfo(1000000 + points.size() * 20, {45.0, points.size() * 0.0003}, 1.0);
    point.m_altitude = altitude;
    point.m_verticalAccuracy = 1.0;
    points.push_back(point);
  }
  return points;
}

Track MakeSavedTrack(GpsTrackCollection const & collection)
{
  kml::TrackData data;
  data.m_id = 42;
  data.m_layers.emplace_back();
  data.m_geometry.m_lines.emplace_back();
  data.m_geometry.m_timestamps.emplace_back();
  collection.ForEach([&](location::GpsInfo const & point, size_t)
  {
    data.m_geometry.m_lines.back().emplace_back(mercator::FromLatLon(point.m_latitude, point.m_longitude),
                                                static_cast<geometry::Altitude>(point.m_altitude));
    data.m_geometry.m_timestamps.back().push_back(point.m_timestamp);
    return true;
  });
  return Track(std::move(data));
}

void CheckSavedStatistics(GpsTrackCollection & collection)
{
  auto const saved = MakeSavedTrack(collection);
  auto const expected = saved.GetStatistics();
  auto const actual = collection.GetTrackStatistics();
  TEST_EQUAL(actual.m_ascent, expected.m_ascent, ());
  TEST_EQUAL(actual.m_descent, expected.m_descent, ());
  TEST_EQUAL(actual.m_minElevation, expected.m_minElevation, ());
  TEST_EQUAL(actual.m_maxElevation, expected.m_maxElevation, ());
  TEST_ALMOST_EQUAL_ABS(actual.m_length, expected.m_length, 1e-9, ());
  TEST_EQUAL(actual.m_duration, expected.m_duration, ());
}
}  // namespace

// https://github.com/organicmaps/organicmaps/issues/13233
UNIT_TEST(GpsTrackCollection_LiveAndSavedElevation)
{
  struct TestCase
  {
    std::initializer_list<double> m_altitudes;
    double m_ascent;
    double m_descent;
    geometry::Altitude m_min;
    geometry::Altitude m_max;
  };

  for (auto const & test : {
           TestCase{{100, 111, 122, 133, 144, 133, 122, 111, 100}, 44, 44, 100, 144},
           TestCase{{100, 111, 107, 122, 118, 133, 129, 144, 133, 137, 122, 126, 111, 115, 100}, 44, 44, 100, 144},
           TestCase{{100, 100, 101, 100, 100}, 0, 0, 100, 100},
           TestCase{{100, 100, 150, 100, 100}, 0, 0, 100, 100},
           TestCase{{100, 100, 50, 100, 100}, 0, 0, 100, 100},
           TestCase{{100, 106, 100}, 6, 6, 100, 106},
           TestCase{{-30, -15, -30}, 15, 15, -30, -15},
           TestCase{{100.9, 111.9, 100.9}, 11, 11, 100, 111},
           TestCase{{0, 0, 0}, 0, 0, 0, 0},
       })
  {
    auto const points = MakeElevationPoints(test.m_altitudes);
    for (size_t const batchSize : {size_t{1}, size_t{3}, points.size()})
    {
      GpsTrackCollection collection;
      for (size_t i = 0; i < points.size(); i += batchSize)
      {
        auto const end = std::min(i + batchSize, points.size());
        collection.Add({points.begin() + i, points.begin() + end});
        if (collection.GetSize() > 1)
          CheckSavedStatistics(collection);
      }

      auto const stats = collection.GetTrackStatistics();
      TEST_EQUAL(stats.m_ascent, test.m_ascent, (test.m_altitudes));
      TEST_EQUAL(stats.m_descent, test.m_descent, (test.m_altitudes));
      TEST_EQUAL(stats.m_minElevation, test.m_min, (test.m_altitudes));
      TEST_EQUAL(stats.m_maxElevation, test.m_max, (test.m_altitudes));

      // Filtering the display profile must not change the points saved or replayed.
      size_t count = 0;
      collection.ForEach([&](location::GpsInfo const & point, size_t)
      {
        TEST_EQUAL(point.m_altitude, points[count].m_altitude, ());
        ++count;
        return true;
      });
      TEST_EQUAL(count, points.size(), ());
    }
  }
}

UNIT_TEST(GpsTrackCollection_ElevationReset)
{
  GpsTrackCollection collection;
  auto checkEmpty = [&]
  {
    auto const stats = collection.GetTrackStatistics();
    TEST_EQUAL(stats.m_length, 0, ());
    TEST_EQUAL(stats.m_duration, 0, ());
    TEST_EQUAL(stats.m_ascent, 0, ());
    TEST_EQUAL(stats.m_descent, 0, ());
    TEST_EQUAL(stats.m_minElevation, 0, ());
    TEST_EQUAL(stats.m_maxElevation, 0, ());
  };
  checkEmpty();

  collection.Add(MakeElevationPoints({100, 100, 150, 100, 100}));
  CheckSavedStatistics(collection);
  collection.Clear(false);
  checkEmpty();

  auto const points = MakeElevationPoints({-30, -15, -30});
  collection.Add({points.front()});
  auto const singlePoint = collection.GetTrackStatistics();
  TEST_EQUAL(singlePoint.m_minElevation, -30, ());
  TEST_EQUAL(singlePoint.m_maxElevation, -30, ());
  TEST_EQUAL(singlePoint.m_ascent, 0, ());
  TEST_EQUAL(singlePoint.m_duration, 0, ());
  collection.Add({points.begin() + 1, points.end()});
  CheckSavedStatistics(collection);

  // Empty batches and old timestamps must not change the cached result.
  collection.Add({});
  collection.Add({points.front()});
  TEST_EQUAL(collection.GetSize(), points.size(), ());
  CheckSavedStatistics(collection);

  collection.Clear();
  checkEmpty();
  collection.Clear();
  checkEmpty();
}
}  // namespace gps_track_collection_test
