#include "testing/testing.hpp"

#include "map/gps_track.hpp"

#include "platform/platform.hpp"

#include "coding/file_name_utils.hpp"
#include "coding/file_writer.hpp"

#include "geometry/latlon.hpp"

#include "base/logging.hpp"
#include "base/scope_guard.hpp"

#include "std/bind.hpp"
#include "std/chrono.hpp"

#include "defines.hpp"

namespace
{

inline location::GpsTrackInfo Make(double timestamp, ms::LatLon const & ll, double speed)
{
  location::GpsTrackInfo info;
  info.m_timestamp = timestamp;
  info.m_speed = speed;
  info.m_latitude = ll.lat;
  info.m_longitude = ll.lon;
  return info;
}

inline string GetGpsTrackFilePath()
{
  return my::JoinFoldersToPath(GetPlatform().WritableDir(), GPS_TRACK_FILENAME);
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

} // namespace

UNIT_TEST(GpsTrack_Simple)
{
  string const filePath = GetGpsTrackFilePath();
  MY_SCOPE_GUARD(gpsTestFileDeleter, bind(FileWriter::DeleteFileX, filePath));

  time_t const t = system_clock::to_time_t(system_clock::now());
  double const timestamp = t;
  LOG(LINFO, ("Timestamp", ctime(&t), timestamp));

  size_t const maxItemCount = 100000;
  size_t const writeItemCount = 50000;

  vector<location::GpsTrackInfo> points;
  points.reserve(writeItemCount);
  for (size_t i = 0; i < writeItemCount; ++i)
    points.emplace_back(Make(timestamp + i, ms::LatLon(-90 + i, -180 + i), 10 + i));

  // Store points
  {
    GpsTrack track(filePath, maxItemCount, hours(24));

    track.AddPoints(points);

    GpsTrackCallback callback;

    track.SetCallback(bind(&GpsTrackCallback::OnUpdate, &callback, _1, _2));

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
    GpsTrack track(filePath, maxItemCount, hours(24));

    GpsTrackCallback callback;

    track.SetCallback(bind(&GpsTrackCallback::OnUpdate, &callback, _1, _2));

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
