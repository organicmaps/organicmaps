#include "tracking/reporter.hpp"

#include "coding/traffic.hpp"

#include "platform/location.hpp"
#include "platform/platform_tests_support/test_socket.hpp"
#include "platform/socket.hpp"

#include "testing/testing.hpp"

#include "base/math.hpp"
#include "base/thread.hpp"

#include "std/cmath.hpp"

using namespace tracking;
using namespace platform::tests_support;

namespace
{
void TransferLocation(Reporter & reporter, TestSocket & testSocket, double timestamp,
                      double latidute, double longtitude)
{
  location::GpsInfo gpsInfo;
  gpsInfo.m_timestamp = timestamp;
  gpsInfo.m_latitude = latidute;
  gpsInfo.m_longitude = longtitude;
  reporter.AddLocation(gpsInfo);

  vector<uint8_t> buffer;
  size_t const readSize = testSocket.ReadServer(buffer);
  TEST_GREATER(readSize, 0, ());

  vector<coding::TrafficGPSEncoder::DataPoint> points;
  MemReader memReader(buffer.data(), buffer.size());
  ReaderSource<MemReader> src(memReader);
  coding::TrafficGPSEncoder::DeserializeDataPoints(coding::TrafficGPSEncoder::kLatestVersion, src,
                                                   points);

  TEST_EQUAL(points.size(), 1, ());
  auto const & point = points[0];
  TEST_EQUAL(point.m_timestamp, timestamp, ());
  TEST(my::AlmostEqualAbs(point.m_latLon.lat, latidute, 0.001), ());
  TEST(my::AlmostEqualAbs(point.m_latLon.lon, longtitude, 0.001), ());
}
}

UNIT_TEST(Reporter_TransferLocations)
{
  auto socket = make_unique<TestSocket>();
  TestSocket & testSocket = *socket.get();

  Reporter reporter(move(socket), milliseconds(10) /* pushDelay */);
  TransferLocation(reporter, testSocket, 1.0, 2.0, 3.0);
  TransferLocation(reporter, testSocket, 4.0, 5.0, 6.0);
  TransferLocation(reporter, testSocket, 7.0, 8.0, 9.0);
}
