#include "base/thread.hpp"
#include "coding/traffic.hpp"

#include "platform/location.hpp"
#include "platform/socket.hpp"

#include "std/cmath.hpp"

#include "testing/testing.hpp"

#include "tracking/reporter.hpp"

namespace
{
template <class Condition>
bool WaitCondition(Condition condition, size_t durationMs = 1000)
{
  size_t sleepMs = 10;
  size_t cyclesLimit = durationMs / sleepMs;
  for (size_t i = 0; i < cyclesLimit; ++i)
  {
    threads::Sleep(sleepMs);
    if (condition())
      return true;
  }

  return false;
}
}  // namespace

using namespace tracking;

UNIT_TEST(Reporter_TransferLocation)
{
  unique_ptr<platform::TestSocket> socket = platform::createTestSocket();
  platform::TestSocket * testSocket = socket.get();

  Reporter reporter(move(socket), 10);

  location::GpsInfo gpsInfo;
  gpsInfo.m_timestamp = 3.0;
  gpsInfo.m_latitude = 4.0;
  gpsInfo.m_longitude = 5.0;
  reporter.AddLocation(gpsInfo);

  TEST(WaitCondition([testSocket] { return testSocket->HasOutput(); }), ());

  vector<uint8_t> buffer;
  testSocket->ReadServer(buffer);

  vector<coding::TrafficGPSEncoder::DataPoint> points;
  MemReader memReader(buffer.data(), buffer.size());
  ReaderSource<MemReader> src(memReader);
  coding::TrafficGPSEncoder::DeserializeDataPoints(coding::TrafficGPSEncoder::kLatestVersion, src,
                                                   points);

  TEST_EQUAL(points.size(), 1, ());
  coding::TrafficGPSEncoder::DataPoint const & point = points[0];
  TEST_EQUAL(point.m_timestamp, 3, ());
  TEST(abs(point.m_latLon.lat - 4.0) < 0.001, ());
  TEST(abs(point.m_latLon.lon - 5.0) < 0.001, ());
}
