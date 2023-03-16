#include "testing/testing.hpp"

#include "map/gps_track.hpp"

#include "platform/platform.hpp"

#include "coding/file_writer.hpp"

#include "geometry/latlon.hpp"

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
  info.m_speedMpS = speed;
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
  GpsTrackCallback()
    : m_toRemove(make_pair(GpsTrack::kInvalidId, GpsTrack::kInvalidId))
    , m_gotCallback(false)
  {
  }
  void OnUpdate(vector<pair<size_t, location::GpsTrackInfo>> && toAdd,
                pair<size_t, size_t> const & toRemove)
  {
    m_toAdd = move(toAdd);
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
    return m_cv.wait_for(ul, t, [this]()->bool{ return m_gotCallback; });
  }

  vector<pair<size_t, location::GpsTrackInfo>> m_toAdd;
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

  size_t const maxItemCount = 100000;
  size_t const writeItemCount = 50000; // less than 24h

  vector<location::GpsInfo> points;
  points.reserve(writeItemCount);
  for (size_t i = 0; i < writeItemCount; ++i)
    points.emplace_back(Make(timestamp + i, ms::LatLon(-90.0 + i, -180.0 + i), 10 + i));

  // Store points
  {
    GpsTrack track(filePath, maxItemCount, hours(24));

    track.AddPoints(points);

    GpsTrackCallback callback;

    track.SetCallback(
        bind(&GpsTrackCallback::OnUpdate, &callback, placeholders::_1, placeholders::_2));

    TEST(callback.WaitForCallback(kWaitForCallbackTimeout), ());

    TEST_EQUAL(callback.m_toRemove.first, GpsTrack::kInvalidId, ());
    TEST_EQUAL(callback.m_toRemove.second, GpsTrack::kInvalidId, ());
    TEST_EQUAL(callback.m_toAdd.size(), writeItemCount, ());
    for (size_t i = 0; i < writeItemCount; ++i)
    {
      TEST_EQUAL(i, callback.m_toAdd[i].first, ());
      TEST_EQUAL(points[i].m_timestamp, callback.m_toAdd[i].second.m_timestamp, ());
      TEST_EQUAL(points[i].m_speedMpS, callback.m_toAdd[i].second.m_speed, ());
      TEST_EQUAL(points[i].m_latitude, callback.m_toAdd[i].second.m_latitude, ());
      TEST_EQUAL(points[i].m_longitude, callback.m_toAdd[i].second.m_longitude, ());
    }
  }

  // Restore points
  {
    GpsTrack track(filePath, maxItemCount, hours(24));

    GpsTrackCallback callback;

    track.SetCallback(
        bind(&GpsTrackCallback::OnUpdate, &callback, placeholders::_1, placeholders::_2));

    TEST(callback.WaitForCallback(kWaitForCallbackTimeout), ());

    TEST_EQUAL(callback.m_toRemove.first, GpsTrack::kInvalidId, ());
    TEST_EQUAL(callback.m_toRemove.second, GpsTrack::kInvalidId, ());
    TEST_EQUAL(callback.m_toAdd.size(), writeItemCount, ());
    for (size_t i = 0; i < writeItemCount; ++i)
    {
      TEST_EQUAL(i, callback.m_toAdd[i].first, ());
      TEST_EQUAL(points[i].m_timestamp, callback.m_toAdd[i].second.m_timestamp, ());
      TEST_EQUAL(points[i].m_speedMpS, callback.m_toAdd[i].second.m_speed, ());
      TEST_EQUAL(points[i].m_latitude, callback.m_toAdd[i].second.m_latitude, ());
      TEST_EQUAL(points[i].m_longitude, callback.m_toAdd[i].second.m_longitude, ());
    }
  }
}

UNIT_TEST(GpsTrack_EvictedByAdd)
{
  string const filePath = GetGpsTrackFilePath();
  SCOPE_GUARD(gpsTestFileDeleter, bind(FileWriter::DeleteFileX, filePath));
  FileWriter::DeleteFileX(filePath);

  time_t const t = system_clock::to_time_t(system_clock::now());
  double const timestamp = t;
  LOG(LINFO, ("Timestamp", ctime(&t), timestamp));

  location::GpsInfo pt1 = Make(timestamp - 25 * 60 * 60, ms::LatLon(30.0, 45.0), 60.0);
  location::GpsInfo pt2 = Make(timestamp, ms::LatLon(75.0, 90.0), 110.0);

  GpsTrack track(filePath, 1000, hours(24));

  GpsTrackCallback callback;
  track.SetCallback(
      bind(&GpsTrackCallback::OnUpdate, &callback, placeholders::_1, placeholders::_2));

  track.AddPoint(pt1);

  TEST(callback.WaitForCallback(kWaitForCallbackTimeout), ());

  // Check pt1 was added
  TEST_EQUAL(1, callback.m_toAdd.size(), ());
  TEST_EQUAL(0, callback.m_toAdd[0].first, ());
  TEST_EQUAL(pt1.m_timestamp, callback.m_toAdd[0].second.m_timestamp, ());
  TEST_EQUAL(pt1.m_speedMpS, callback.m_toAdd[0].second.m_speed, ());
  TEST_EQUAL(pt1.m_latitude, callback.m_toAdd[0].second.m_latitude, ());
  TEST_EQUAL(pt1.m_longitude, callback.m_toAdd[0].second.m_longitude, ());
  // and nothing was evicted
  TEST_EQUAL(callback.m_toRemove.first, GpsTrack::kInvalidId, ());
  TEST_EQUAL(callback.m_toRemove.second, GpsTrack::kInvalidId, ());

  callback.Reset();

  track.AddPoint(pt2);

  TEST(callback.WaitForCallback(kWaitForCallbackTimeout), ());

  // Check pt2 was added
  TEST_EQUAL(1, callback.m_toAdd.size(), ());
  TEST_EQUAL(1, callback.m_toAdd[0].first, ());
  TEST_EQUAL(pt2.m_timestamp, callback.m_toAdd[0].second.m_timestamp, ());
  TEST_EQUAL(pt2.m_speedMpS, callback.m_toAdd[0].second.m_speed, ());
  TEST_EQUAL(pt2.m_latitude, callback.m_toAdd[0].second.m_latitude, ());
  TEST_EQUAL(pt2.m_longitude, callback.m_toAdd[0].second.m_longitude, ());
  // and pt1 was evicted as old
  TEST_EQUAL(callback.m_toRemove.first, 0, ());
  TEST_EQUAL(callback.m_toRemove.second, 0, ());
}
} // namespace gps_track_test
