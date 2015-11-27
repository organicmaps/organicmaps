#include "testing/testing.hpp"

#include "map/gps_track_container.hpp"

#include "std/bind.hpp"

namespace
{

uint32_t constexpr kSecondsPerHour = 60 * 60;

struct GpsTrackContainerCallback
{
public:
  void OnChange(vector<df::GpsTrackPoint> && toAdd, vector<uint32_t> && toRemove)
  {
    m_toAdd.insert(m_toAdd.end(), toAdd.begin(), toAdd.end());
    m_toRemove.insert(m_toRemove.end(), toRemove.begin(), toRemove.end());
  }
  vector<df::GpsTrackPoint> m_toAdd;
  vector<uint32_t> m_toRemove;
};

} // namespace

UNIT_TEST(GpsTrackContainer_Test)
{
  GpsTrackContainer gpstrack;

  time_t timestamp = system_clock::to_time_t(system_clock::now());

  uint32_t id0 = gpstrack.AddPoint(m2::PointD(0,0), 0.0, timestamp);
  uint32_t id1 = gpstrack.AddPoint(m2::PointD(1,1), 1.0, timestamp + kSecondsPerHour);
  uint32_t id2 = gpstrack.AddPoint(m2::PointD(2,2), 2.0, timestamp + 2 * kSecondsPerHour);

  // Set callback and expect toAdd callback with all points

  GpsTrackContainerCallback callback;
  gpstrack.SetCallback(bind(&GpsTrackContainerCallback::OnChange, ref(callback), _1, _2), true /* sendAll */);

  TEST(callback.m_toRemove.empty(), ());

  TEST_EQUAL(3, callback.m_toAdd.size(), ());
  TEST_EQUAL(id0, callback.m_toAdd[0].m_id, ());
  TEST_EQUAL(id1, callback.m_toAdd[1].m_id, ());
  TEST_EQUAL(id2, callback.m_toAdd[2].m_id, ());

  callback.m_toAdd.clear();
  callback.m_toRemove.clear();

  // Add point in 25h (duration is 24h) and expect point id0 is popped and point id25 is added

  uint32_t id25 = gpstrack.AddPoint(m2::PointD(25,25), 25.0, timestamp + 25 * kSecondsPerHour);

  TEST_EQUAL(1, callback.m_toAdd.size(), ());
  TEST_EQUAL(id25, callback.m_toAdd[0].m_id, ());

  TEST_EQUAL(1, callback.m_toRemove.size(), ());
  TEST_EQUAL(id0, callback.m_toRemove[0], ());

  callback.m_toAdd.clear();
  callback.m_toRemove.clear();

  // Set duration in 2h and expect points id1 and id2 are popped

  gpstrack.SetDuration(hours(2));

  TEST(callback.m_toAdd.empty(), ());

  TEST_EQUAL(2, callback.m_toRemove.size(), ());
  TEST_EQUAL(id1, callback.m_toRemove[0], ());
  TEST_EQUAL(id2, callback.m_toRemove[1], ());

  callback.m_toAdd.clear();
  callback.m_toRemove.clear();

  // and test there is only id25 point in the track

  vector<df::GpsTrackPoint> points;
  gpstrack.GetPoints(points);

  TEST_EQUAL(1, points.size(), ());
  TEST_EQUAL(id25, points[0].m_id, ());
}
