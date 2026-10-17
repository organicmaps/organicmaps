#include "testing/testing.hpp"

#include "map/gps_track.hpp"
#include "map/track.hpp"

#include "platform/platform.hpp"

#include "coding/file_writer.hpp"

#include "geometry/latlon.hpp"
#include "geometry/mercator.hpp"

#include "base/file_name_utils.hpp"
#include "base/logging.hpp"
#include "base/scope_guard.hpp"

#include <chrono>
#include <functional>
#include <utility>
#include <vector>

namespace gps_track_test
{
using namespace std;
using namespace std::chrono;

inline location::GpsInfo Make(double timestamp, ms::LatLon const & ll, double speed)
{
  location::GpsInfo info;
  info.m_timestamp = timestamp;
  info.m_speed = speed;
  info.m_latitude = ll.m_lat;
  info.m_longitude = ll.m_lon;
  info.m_horizontalAccuracy = 15;
  info.m_source = location::EAndroidNative;
  return info;
}

inline string GetGpsTrackFilePath()
{
  return base::JoinPath(GetPlatform().WritableDir(), "gpstrack_test.bin");
}

class GpsTrackCallback
{
public:
  GpsTrackCallback() : m_toRemove(make_pair(GpsTrack::kInvalidId, GpsTrack::kInvalidId)), m_gotCallback(false) {}
  void OnUpdate(vector<pair<size_t, location::GpsInfo>> && toAdd, pair<size_t, size_t> const & toRemove)
  {
    m_toAdd = std::move(toAdd);
    m_toRemove = toRemove;

    lock_guard<mutex> lg(m_mutex);
    m_gotCallback = true;
    m_cv.notify_one();
  }
  void Reset()
  {
    m_toAdd.clear();
    m_toRemove = make_pair(GpsTrack::kInvalidId, GpsTrack::kInvalidId);

    lock_guard<mutex> lg(m_mutex);
    m_gotCallback = false;
  }
  bool WaitForCallback(seconds t)
  {
    unique_lock<mutex> ul(m_mutex);
    return m_cv.wait_for(ul, t, [this]() -> bool { return m_gotCallback; });
  }

  vector<pair<size_t, location::GpsInfo>> m_toAdd;
  pair<size_t, size_t> m_toRemove;

private:
  mutex m_mutex;
  condition_variable m_cv;
  bool m_gotCallback;
};

seconds const kWaitForCallbackTimeout = seconds(5);

UNIT_TEST(GpsTrack_Simple)
{
  string const filePath = GetGpsTrackFilePath();
  SCOPE_GUARD(gpsTestFileDeleter, bind(FileWriter::DeleteFileX, filePath));
  FileWriter::DeleteFileX(filePath);

  time_t const t = system_clock::to_time_t(system_clock::now());
  double const timestamp = t;
  LOG(LINFO, ("Timestamp", ctime(&t), timestamp));

  size_t const writeItemCount = 50000;

  vector<location::GpsInfo> points;
  points.reserve(writeItemCount);
  for (size_t i = 0; i < writeItemCount; ++i)
    points.emplace_back(Make(timestamp + i, ms::LatLon(-90.0 + i, -180.0 + i), 10 + i));

  // Store points
  {
    GpsTrack track(filePath);

    track.AddPoints(points);

    GpsTrackCallback callback;

    track.SetCallback(bind(&GpsTrackCallback::OnUpdate, &callback, placeholders::_1, placeholders::_2));

    TEST(callback.WaitForCallback(kWaitForCallbackTimeout), ());

    TEST_EQUAL(callback.m_toRemove.first, GpsTrack::kInvalidId, ());
    TEST_EQUAL(callback.m_toRemove.second, GpsTrack::kInvalidId, ());
    TEST_EQUAL(callback.m_toAdd.size(), writeItemCount, ());
    for (size_t i = 0; i < writeItemCount; ++i)
    {
      TEST_EQUAL(i, callback.m_toAdd[i].first, ());
      TEST_EQUAL(points[i].m_timestamp, callback.m_toAdd[i].second.m_timestamp, ());
      TEST_EQUAL(points[i].m_speed, callback.m_toAdd[i].second.m_speed, ());
      TEST_EQUAL(points[i].m_latitude, callback.m_toAdd[i].second.m_latitude, ());
      TEST_EQUAL(points[i].m_longitude, callback.m_toAdd[i].second.m_longitude, ());
    }
  }

  // Restore points
  {
    GpsTrack track(filePath);

    GpsTrackCallback callback;

    track.SetCallback(bind(&GpsTrackCallback::OnUpdate, &callback, placeholders::_1, placeholders::_2));

    TEST(callback.WaitForCallback(kWaitForCallbackTimeout), ());

    TEST_EQUAL(callback.m_toRemove.first, GpsTrack::kInvalidId, ());
    TEST_EQUAL(callback.m_toRemove.second, GpsTrack::kInvalidId, ());
    TEST_EQUAL(callback.m_toAdd.size(), writeItemCount, ());
    for (size_t i = 0; i < writeItemCount; ++i)
    {
      TEST_EQUAL(i, callback.m_toAdd[i].first, ());
      TEST_EQUAL(points[i].m_timestamp, callback.m_toAdd[i].second.m_timestamp, ());
      TEST_EQUAL(points[i].m_speed, callback.m_toAdd[i].second.m_speed, ());
      TEST_EQUAL(points[i].m_latitude, callback.m_toAdd[i].second.m_latitude, ());
      TEST_EQUAL(points[i].m_longitude, callback.m_toAdd[i].second.m_longitude, ());
    }
  }
}

UNIT_TEST(GpsTrack_ElevationSnapshots)
{
  auto const filePath = base::JoinPath(GetPlatform().WritableDir(), "gpstrack_elevation_test.bin");
  SCOPE_GUARD(gpsTestFileDeleter, bind(FileWriter::DeleteFileX, filePath));
  FileWriter::DeleteFileX(filePath);

  auto makePoint = [](size_t index, double altitude)
  {
    auto point = Make(1000000 + index * 20, {45.0, index * 0.0003}, 1.0);
    point.m_altitude = altitude;
    point.m_verticalAccuracy = 1.0;
    return point;
  };

  GpsTrackCallback callback;
  TrackStatistics callbackStatistics;
  GpsTrack track(filePath);
  track.SetCallback([&](auto && toAdd, auto const & toRemove, GpsTrack::StatisticsGetter const & getStatistics)
  {
    callbackStatistics = getStatistics();
    callback.OnUpdate(std::move(toAdd), toRemove);
  });
  track.AddPoints({makePoint(0, 100), makePoint(1, 100), makePoint(2, 150)});
  TEST(callback.WaitForCallback(kWaitForCallbackTimeout), ());

  auto const snapshot = track.GetElevationSnapshot();
  TEST(snapshot, ());
  TEST_EQUAL(snapshot->m_elevation.GetLines().front().back().m_altitude, 150, ());

  callback.Reset();
  track.AddPoints({makePoint(3, 100), makePoint(4, 100)});
  TEST(callback.WaitForCallback(kWaitForCallbackTimeout), ());
  TEST_EQUAL(callbackStatistics.m_ascent, 0, ());
  TEST_EQUAL(callbackStatistics.m_descent, 0, ());
  TEST_EQUAL(callbackStatistics.m_maxElevation, 100, ());
  TEST_EQUAL(track.GetElevationSnapshot()->m_elevation.GetSize(), 2, ());
  // The worker may replace its cached profile while a UI consumer holds an older snapshot.
  TEST_EQUAL(snapshot->m_elevation.GetLines().front().back().m_altitude, 150, ());

  TEST_EQUAL(track.Finalize(), 5, ());
  kml::TrackData data;
  data.m_id = 42;
  data.m_layers.emplace_back();
  data.m_geometry.m_lines.emplace_back();
  data.m_geometry.m_timestamps.emplace_back();
  track.ForEachPoint([&](location::GpsInfo const & point, size_t)
  {
    data.m_geometry.m_lines.back().emplace_back(mercator::FromLatLon(point.m_latitude, point.m_longitude),
                                                static_cast<geometry::Altitude>(point.m_altitude));
    data.m_geometry.m_timestamps.back().push_back(point.m_timestamp);
    return true;
  });
  auto const saved = Track(std::move(data)).GetStatistics();
  TEST_EQUAL(callbackStatistics.m_ascent, saved.m_ascent, ());
  TEST_EQUAL(callbackStatistics.m_descent, saved.m_descent, ());
  TEST_EQUAL(callbackStatistics.m_minElevation, saved.m_minElevation, ());
  TEST_EQUAL(callbackStatistics.m_maxElevation, saved.m_maxElevation, ());
  TEST_EQUAL(callbackStatistics.m_length, saved.m_length, ());
  TEST_EQUAL(callbackStatistics.m_duration, saved.m_duration, ());
}
UNIT_TEST(GpsTrack_ElevationCacheCancellationAndReset)
{
  auto const filePath = base::JoinPath(GetPlatform().WritableDir(), "gpstrack_elevation_cache_test.bin");
  SCOPE_GUARD(gpsTestFileDeleter, bind(FileWriter::DeleteFileX, filePath));
  FileWriter::DeleteFileX(filePath);
  GpsTrackCallback callback;
  GpsTrack track(filePath);
  // A renderer can consume the raw diff without requesting altitude statistics.
  track.SetCallback(bind(&GpsTrackCallback::OnUpdate, &callback, placeholders::_1, placeholders::_2));
  auto point = [](size_t index, int altitude)
  {
    auto gps = Make(1000000 + index * 20, {45.0, index * 0.0003}, 1);
    gps.m_altitude = altitude;
    return gps;
  };
  std::vector<location::GpsInfo> points;
  for (size_t i = 0; i < 700; ++i)
    points.push_back(point(i, 100 + i % 50));
  track.AddPoints(points);
  TEST(callback.WaitForCallback(kWaitForCallbackTimeout), ());
  auto const first = track.GetElevationSnapshot();
  TEST(first, ());
  auto checkProfile = [&](GpsTrack::ElevationSnapshot const & snapshot)
  {
    kml::TrackData data;
    data.m_id = 42;
    data.m_layers.emplace_back();
    data.m_geometry.m_lines.emplace_back();
    data.m_geometry.m_timestamps.emplace_back();
    for (auto const & gps : points)
    {
      data.m_geometry.m_lines.back().emplace_back(mercator::FromLatLon(gps.m_latitude, gps.m_longitude),
                                                  static_cast<geometry::Altitude>(gps.m_altitude));
      data.m_geometry.m_timestamps.back().push_back(gps.m_timestamp);
    }
    Track saved(std::move(data));
    auto const & expected = saved.GetElevationInfo()->GetLines().front();
    auto const & actual = snapshot.m_elevation.GetLines().front();
    TEST_EQUAL(actual.size(), expected.size(), ());
    for (size_t i = 0; i < std::min(actual.size(), expected.size()); ++i)
    {
      TEST_EQUAL(actual[i].m_altitude, expected[i].m_altitude, (i));
      TEST_EQUAL(actual[i].m_distance, expected[i].m_distance, (i));
    }
  };
  checkProfile(*first);
  TEST_EQUAL(first->m_revision, points.size(), ());
  TEST_EQUAL(first, track.GetElevationSnapshot(), ());
  // Cancellation callbacks are invoked outside the recording lock, including cache hits.
  TEST_EQUAL(first,
             track.GetElevationSnapshot(
                 [&]
  {
    TEST_EQUAL(track.GetTrackStatistics().m_ascent, first->m_statistics.m_ascent, ());
    return false;
  }),
             ());

  callback.Reset();
  points.push_back(point(700, 125));
  track.AddPoint(points.back());
  TEST(callback.WaitForCallback(kWaitForCallbackTimeout), ());
  size_t calls = 0;
  TEST(!track.GetElevationSnapshot([&] { return ++calls == 4; }), ());
  auto const next = track.GetElevationSnapshot();
  checkProfile(*next);
  TEST_EQUAL(next->m_revision, 701, ());
  TEST_EQUAL(first->m_revision, 700, ());
  TEST_EQUAL(next->m_statistics.m_ascent, track.GetTrackStatistics().m_ascent, ());

  callback.Reset();
  track.Clear();
  TEST(!track.GetElevationSnapshot(), ());
  TEST(callback.WaitForCallback(kWaitForCallbackTimeout), ());
  callback.Reset();
  track.AddPoints({point(0, 300), point(1, 311), point(2, 300)});
  TEST(callback.WaitForCallback(kWaitForCallbackTimeout), ());
  auto const reset = track.GetElevationSnapshot();
  TEST_NOT_EQUAL(reset->m_epoch, first->m_epoch, ());
  TEST_EQUAL(reset->m_revision, 3, ());
  TEST_EQUAL(reset->m_statistics.m_ascent, 11, ());
  TEST_EQUAL(reset->m_statistics.m_descent, 11, ());
  TEST_EQUAL(first->m_revision, 700, ());
}
}  // namespace gps_track_test
