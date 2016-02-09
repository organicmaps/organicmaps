#include "testing/testing.hpp"

#include "map/gps_track_collection.hpp"

#include "base/logging.hpp"

#include "std/map.hpp"

namespace
{

location::GpsTrackInfo MakeGpsTrackInfo(double timestamp, ms::LatLon const & ll, double speed)
{
  location::GpsTrackInfo info;
  info.m_timestamp = timestamp;
  info.m_speed = speed;
  info.m_latitude = ll.lat;
  info.m_longitude = ll.lon;
  return info;
}

} // namespace

UNIT_TEST(GpsTrackCollection_Simple)
{
  time_t const t = system_clock::to_time_t(system_clock::now());
  double const timestamp = t;
  LOG(LINFO, ("Timestamp", ctime(&t), timestamp));

  GpsTrackCollection collection(100, hours(24));

  map<size_t, location::GpsTrackInfo> data;

  TEST_EQUAL(100, collection.GetMaxSize(), ());

  for (size_t i = 0; i < 50; ++i)
  {
    auto info = MakeGpsTrackInfo(timestamp + i, ms::LatLon(-90 + i, -180 + i), i);
    pair<size_t, size_t> evictedIds;
    size_t addedId = collection.Add(info, evictedIds);
    TEST_EQUAL(addedId, i, ());
    TEST_EQUAL(evictedIds.first, GpsTrackCollection::kInvalidId, ());
    TEST_EQUAL(evictedIds.second, GpsTrackCollection::kInvalidId, ());
    data[addedId] = info;
  }

  TEST_EQUAL(50, collection.GetSize(), ());

  collection.ForEach([&data](location::GpsTrackInfo const & info, size_t id)->bool
  {
    TEST(data.end() != data.find(id), ());
    location::GpsTrackInfo const & originInfo = data[id];
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

UNIT_TEST(GpsTrackCollection_EvictedByTimestamp)
{
  time_t const t = system_clock::to_time_t(system_clock::now());
  double const timestamp = t;
  LOG(LINFO, ("Timestamp", ctime(&t), timestamp));

  double const timestamp_1h = timestamp + 60 * 60;
  double const timestamp_25h = timestamp + 25 * 60 * 60;

  GpsTrackCollection collection(100, hours(24));

  pair<size_t, size_t> evictedIds;
  size_t addedId = collection.Add(MakeGpsTrackInfo(timestamp, ms::LatLon(-90, -180), 1), evictedIds);
  TEST_EQUAL(addedId, 0, ());
  TEST_EQUAL(evictedIds.first, GpsTrackCollection::kInvalidId, ());
  TEST_EQUAL(evictedIds.second, GpsTrackCollection::kInvalidId, ());

  addedId = collection.Add(MakeGpsTrackInfo(timestamp_1h, ms::LatLon(90, 180), 2), evictedIds);
  TEST_EQUAL(addedId, 1, ());
  TEST_EQUAL(evictedIds.first, GpsTrackCollection::kInvalidId, ());
  TEST_EQUAL(evictedIds.second, GpsTrackCollection::kInvalidId, ());

  TEST_EQUAL(2, collection.GetSize(), ());

  auto lastInfo = MakeGpsTrackInfo(timestamp_25h, ms::LatLon(45, 60), 3);
  addedId = collection.Add(lastInfo, evictedIds);
  TEST_EQUAL(addedId, 2, ());
  TEST_EQUAL(evictedIds.first, 0, ());
  TEST_EQUAL(evictedIds.second, 1, ());

  TEST_EQUAL(1, collection.GetSize(), ());

  collection.ForEach([&](location::GpsTrackInfo const & info, size_t id)->bool
  {
    TEST_EQUAL(id, 2, ());
    TEST_EQUAL(info.m_latitude, lastInfo.m_latitude, ());
    TEST_EQUAL(info.m_longitude, lastInfo.m_longitude, ());
    TEST_EQUAL(info.m_speed, lastInfo.m_speed, ());
    TEST_EQUAL(info.m_timestamp, lastInfo.m_timestamp, ());
    return true;
  });

  auto res = collection.Clear();
  TEST_EQUAL(res.first, 2, ());
  TEST_EQUAL(res.second, 2, ());

  TEST_EQUAL(0, collection.GetSize(), ());
}

UNIT_TEST(GpsTrackCollection_EvictedByCount)
{
  time_t const t = system_clock::to_time_t(system_clock::now());
  double const timestamp = t;
  LOG(LINFO, ("Timestamp", ctime(&t), timestamp));

  GpsTrackCollection collection(100, hours(24));

  map<size_t, location::GpsTrackInfo> data;

  TEST_EQUAL(100, collection.GetMaxSize(), ());

  for (size_t i = 0; i < 100; ++i)
  {
    auto info = MakeGpsTrackInfo(timestamp + i, ms::LatLon(-90 + i, -180 + i), i);
    pair<size_t, size_t> evictedIds;
    size_t addedId = collection.Add(info, evictedIds);
    TEST_EQUAL(addedId, i, ());
    TEST_EQUAL(evictedIds.first, GpsTrackCollection::kInvalidId, ());
    TEST_EQUAL(evictedIds.second, GpsTrackCollection::kInvalidId, ());
    data[addedId] = info;
  }

  TEST_EQUAL(100, collection.GetSize(), ());

  auto info = MakeGpsTrackInfo(timestamp + 100, ms::LatLon(45, 60), 110);
  pair<size_t, size_t> evictedIds;
  size_t addedId = collection.Add(info, evictedIds);
  TEST_EQUAL(addedId, 100, ());
  TEST_EQUAL(evictedIds.first, 0, ());
  TEST_EQUAL(evictedIds.second, 0, ());
  data[addedId] = info;

  TEST_EQUAL(100, collection.GetSize(), ());

  data.erase(0);

  collection.ForEach([&data](location::GpsTrackInfo const & info, size_t id)->bool
  {
    TEST(data.end() != data.find(id), ());
    location::GpsTrackInfo const & originInfo = data[id];
    TEST_EQUAL(info.m_latitude, originInfo.m_latitude, ());
    TEST_EQUAL(info.m_longitude, originInfo.m_longitude, ());
    TEST_EQUAL(info.m_speed, originInfo.m_speed, ());
    TEST_EQUAL(info.m_timestamp, originInfo.m_timestamp, ());
    return true;
  });

  auto res = collection.Clear();
  TEST_EQUAL(res.first, 1, ());
  TEST_EQUAL(res.second, 100, ());

  TEST_EQUAL(0, collection.GetSize(), ());
}

UNIT_TEST(GpsTrackCollection_EvictedByTimestamp2)
{
  time_t const t = system_clock::to_time_t(system_clock::now());
  double const timestamp = t;
  LOG(LINFO, ("Timestamp", ctime(&t), timestamp));

  double const timestamp_12h = timestamp + 12 * 60 * 60;
  double const timestamp_35h = timestamp + 35 * 60 * 60;

  GpsTrackCollection collection(1000, hours(24));

  TEST_EQUAL(1000, collection.GetMaxSize(), ());

  for (size_t i = 0; i < 500; ++i)
  {
    auto info = MakeGpsTrackInfo(timestamp + i, ms::LatLon(-90 + i, -180 + i), i);
    pair<size_t, size_t> evictedIds;
    size_t addedId = collection.Add(info, evictedIds);
    TEST_EQUAL(addedId, i, ());
    TEST_EQUAL(evictedIds.first, GpsTrackCollection::kInvalidId, ());
    TEST_EQUAL(evictedIds.second, GpsTrackCollection::kInvalidId, ());
  }

  auto time = collection.GetTimestampRange();

  TEST_EQUAL(500, collection.GetSize(), ());

  for (size_t i = 0; i < 500; ++i)
  {
    auto info = MakeGpsTrackInfo(timestamp_12h + i, ms::LatLon(-90 + i, -180 + i), i);
    pair<size_t, size_t> evictedIds;
    size_t addedId = collection.Add(info, evictedIds);
    TEST_EQUAL(addedId, 500 + i, ());
    TEST_EQUAL(evictedIds.first, GpsTrackCollection::kInvalidId, ());
    TEST_EQUAL(evictedIds.second, GpsTrackCollection::kInvalidId, ());
  }

  time = collection.GetTimestampRange();

  TEST_EQUAL(1000, collection.GetSize(), ());

  auto info = MakeGpsTrackInfo(timestamp_35h, ms::LatLon(45, 60), 110);
  pair<size_t, size_t> evictedIds;
  size_t addedId = collection.Add(info, evictedIds);
  TEST_EQUAL(addedId, 1000, ());
  TEST_EQUAL(evictedIds.first, 0, ());
  TEST_EQUAL(evictedIds.second, 499, ());

  auto res = collection.Clear();
  TEST_EQUAL(res.first, 500, ());
  TEST_EQUAL(res.second, 1000, ());

  TEST_EQUAL(0, collection.GetSize(), ());
}
