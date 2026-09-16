#include "testing/testing.hpp"

#include "map/gps_track.hpp"

#include "platform/platform.hpp"

#include "coding/file_writer.hpp"

#include "geometry/distance_on_sphere.hpp"
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

// About 22 m between points, 1 s apart.
double constexpr kStepDegrees = 0.0002;

vector<location::GpsInfo> MakeWalk(size_t count)
{
  double const timestamp = system_clock::to_time_t(system_clock::now());
  vector<location::GpsInfo> points;
  for (size_t i = 0; i < count; ++i)
    points.emplace_back(Make(timestamp + i, ms::LatLon(kStepDegrees * i, 0), 22));
  return points;
}

double GetWalkLength(size_t count)
{
  return (count - 1) * ms::DistanceOnEarth(ms::LatLon(0, 0), ms::LatLon(kStepDegrees, 0));
}

class GpsTrackStatisticsCallback
{
public:
  void OnUpdate(TrackStatistics const & trackStatistics)
  {
    lock_guard<mutex> lg(m_mutex);
    m_length = trackStatistics.m_length;
    m_cv.notify_one();
  }
  bool WaitForLength(double expected, seconds t)
  {
    unique_lock<mutex> ul(m_mutex);
    return m_cv.wait_for(ul, t, [this, expected]() { return fabs(m_length - expected) < 1.0; });
  }

private:
  mutex m_mutex;
  condition_variable m_cv;
  double m_length = 0;
};

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

// Recording without a displayed track, e.g. started from an Android home screen widget, still needs the statistics.
UNIT_TEST(GpsTrack_StatisticsWithoutDiffCallback)
{
  string const filePath = GetGpsTrackFilePath();
  SCOPE_GUARD(gpsTestFileDeleter, bind(FileWriter::DeleteFileX, filePath));
  FileWriter::DeleteFileX(filePath);

  double const expectedLength = GetWalkLength(5);

  GpsTrack track(filePath);
  GpsTrackStatisticsCallback callback;
  track.SetStatisticsCallback(bind(&GpsTrackStatisticsCallback::OnUpdate, &callback, placeholders::_1));

  track.AddPoints(MakeWalk(5));

  TEST(callback.WaitForLength(expectedLength, kWaitForCallbackTimeout), ());
  TEST(!track.IsEmpty(), ());
  TEST_ALMOST_EQUAL_ABS(track.GetTrackStatistics().m_length, expectedLength, 1.0, ());
}

// A new subscriber gets the current statistics even if no new points are added.
UNIT_TEST(GpsTrack_StatisticsForNewCallback)
{
  string const filePath = GetGpsTrackFilePath();
  SCOPE_GUARD(gpsTestFileDeleter, bind(FileWriter::DeleteFileX, filePath));
  FileWriter::DeleteFileX(filePath);

  double const expectedLength = GetWalkLength(5);

  GpsTrack track(filePath);
  GpsTrackStatisticsCallback first;
  track.SetStatisticsCallback(bind(&GpsTrackStatisticsCallback::OnUpdate, &first, placeholders::_1));
  track.AddPoints(MakeWalk(5));
  TEST(first.WaitForLength(expectedLength, kWaitForCallbackTimeout), ());

  GpsTrackStatisticsCallback second;
  track.SetStatisticsCallback(bind(&GpsTrackStatisticsCallback::OnUpdate, &second, placeholders::_1));
  TEST(second.WaitForLength(expectedLength, kWaitForCallbackTimeout), ());
}

// A recording that continues after a restart without the map, e.g. from an Android home screen widget, reports
// the statistics of the stored track and can save it, even if the user stands still and no new points are added.
UNIT_TEST(GpsTrack_StatisticsOfStoredTrack)
{
  string const filePath = GetGpsTrackFilePath();
  SCOPE_GUARD(gpsTestFileDeleter, bind(FileWriter::DeleteFileX, filePath));
  FileWriter::DeleteFileX(filePath);

  double const expectedLength = GetWalkLength(5);

  {
    GpsTrack track(filePath);
    GpsTrackStatisticsCallback callback;
    track.SetStatisticsCallback(bind(&GpsTrackStatisticsCallback::OnUpdate, &callback, placeholders::_1));
    track.AddPoints(MakeWalk(5));
    TEST(callback.WaitForLength(expectedLength, kWaitForCallbackTimeout), ());
  }

  GpsTrack track(filePath);
  GpsTrackStatisticsCallback callback;
  track.SetStatisticsCallback(bind(&GpsTrackStatisticsCallback::OnUpdate, &callback, placeholders::_1));

  TEST(callback.WaitForLength(expectedLength, kWaitForCallbackTimeout), ());
  TEST(!track.IsEmpty(), ());
}
}  // namespace gps_track_test
