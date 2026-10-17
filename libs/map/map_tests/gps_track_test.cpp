#include "testing/testing.hpp"

#include "map/gps_track.hpp"

#include "platform/platform.hpp"

#include "coding/file_writer.hpp"

#include "geometry/latlon.hpp"

#include "base/file_name_utils.hpp"
#include "base/logging.hpp"
#include "base/scope_guard.hpp"

#include <atomic>
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

class StatisticsObserver
{
public:
  void OnUpdate(TrackStatistics const & statistics)
  {
    lock_guard lock(m_mutex);
    m_updates.push_back(statistics);
    m_cv.notify_one();
  }

  bool WaitForCount(size_t count)
  {
    unique_lock lock(m_mutex);
    return m_cv.wait_for(lock, kWaitForCallbackTimeout, [&] { return m_updates.size() >= count; });
  }

  vector<TrackStatistics> GetUpdates()
  {
    lock_guard lock(m_mutex);
    return m_updates;
  }

private:
  mutex m_mutex;
  condition_variable m_cv;
  vector<TrackStatistics> m_updates;
};

vector<location::GpsInfo> MakeRecordingPoints()
{
  vector<location::GpsInfo> points;
  for (size_t i = 0; i < 5; ++i)
  {
    auto point = Make(1000 + 20 * i, ms::LatLon(45 + 0.0003 * i, 10), 2);
    point.m_altitude = 100 + i;
    point.m_verticalAccuracy = 1;
    points.push_back(point);
  }
  return points;
}

UNIT_TEST(GpsTrack_StatisticsSubscription)
{
  auto const filePath = GetGpsTrackFilePath();
  SCOPE_GUARD(gpsTestFileDeleter, bind(FileWriter::DeleteFileX, filePath));
  FileWriter::DeleteFileX(filePath);

  StatisticsObserver statistics;
  GpsTrackCallback map;
  GpsTrack track(filePath);
  track.SetStatisticsCallback([&](auto const & update) { statistics.OnUpdate(update); });
  TEST(statistics.WaitForCount(1), ());
  TEST_EQUAL(statistics.GetUpdates().back().m_length, 0, ());

  auto const points = MakeRecordingPoints();
  track.AddPoints(points);
  TEST(statistics.WaitForCount(2), ());
  TEST_GREATER(statistics.GetUpdates().back().m_length, 100, ());
  track.SetCallback(bind(&GpsTrackCallback::OnUpdate, &map, placeholders::_1, placeholders::_2));
  TEST(map.WaitForCallback(kWaitForCallbackTimeout), ());
  TEST_EQUAL(statistics.GetUpdates().size(), 2, ());

  track.SetCallback(nullptr);
  auto next = points.back();
  next.m_latitude += 0.0003;
  next.m_timestamp += 20;
  track.AddPoint(next);
  TEST(statistics.WaitForCount(3), ());
  TEST_GREATER(statistics.GetUpdates().back().m_length, statistics.GetUpdates()[1].m_length, ());

  track.Clear();
  TEST(statistics.WaitForCount(4), ());
  TEST_EQUAL(statistics.GetUpdates().back().m_length, 0, ());
  track.SetStatisticsCallback(nullptr);
  track.AddPoints(points);
  track.Finalize();
  TEST_EQUAL(statistics.GetUpdates().size(), 4, ());
}

UNIT_TEST(GpsTrack_RestoreStatisticsWithoutMap)
{
  auto const filePath = GetGpsTrackFilePath();
  SCOPE_GUARD(gpsTestFileDeleter, bind(FileWriter::DeleteFileX, filePath));
  FileWriter::DeleteFileX(filePath);
  auto const points = MakeRecordingPoints();
  TrackStatistics expected;
  for (auto const & point : points)
    expected.AddGpsInfoPoint(point);
  {
    GpsTrack writer(filePath);
    writer.AddPoints(points);
    writer.Finalize();
  }

  StatisticsObserver statistics;
  GpsTrack restored(filePath);
  restored.SetStatisticsCallback([&](auto const & update) { statistics.OnUpdate(update); });
  TEST(statistics.WaitForCount(1), ());
  auto const update = statistics.GetUpdates().front();
  TEST_ALMOST_EQUAL_ABS(update.m_length, expected.m_length, 1e-6, ());
  TEST_EQUAL(update.m_duration, expected.m_duration, ());
  TEST(!restored.IsEmpty(), ());
  TEST_EQUAL(restored.Finalize(), points.size(), ());
  TEST_EQUAL(statistics.GetUpdates().size(), 1, ());
}

UNIT_TEST(GpsTrack_ReplaceStatisticsCallbackDuringRecording)
{
  auto const filePath = GetGpsTrackFilePath();
  SCOPE_GUARD(gpsTestFileDeleter, bind(FileWriter::DeleteFileX, filePath));
  FileWriter::DeleteFileX(filePath);

  atomic<size_t> calls = 0;
  GpsTrack track(filePath);
  auto const callback = [&](TrackStatistics const &) { ++calls; };
  thread producer([&]
  {
    for (size_t i = 0; i < 500; ++i)
      track.AddPoint(Make(1000 + i, ms::LatLon(45 + i * 0.0003, 10), 2));
  });
  for (size_t i = 0; i < 500; ++i)
  {
    track.SetStatisticsCallback(callback);
    track.SetStatisticsCallback(nullptr);
  }
  producer.join();
  track.SetStatisticsCallback(nullptr);
  auto const callsAfterUnsubscribe = calls.load();
  track.Finalize();
  TEST_EQUAL(calls.load(), callsAfterUnsubscribe, ());
}

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
}  // namespace gps_track_test
