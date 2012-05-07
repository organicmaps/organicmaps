#include "../../testing/testing.hpp"

#include "../geourl_process.hpp"

using namespace url_scheme;

UNIT_TEST(ProcessURL_Smoke)
{
  Info info;
  ParseURL("geo:53.666,27.666", info);
  TEST(info.IsValid(), ());
  TEST_ALMOST_EQUAL(info.m_lat, 53.666, ());
  TEST_ALMOST_EQUAL(info.m_lon, 27.666, ());

  info.Reset();
  ParseURL("mapswithme:53.666,27.666", info);
  TEST(info.IsValid(), ());
  TEST_ALMOST_EQUAL(info.m_lat, 53.666, ());
  TEST_ALMOST_EQUAL(info.m_lon, 27.666, ());

  info.Reset();
  ParseURL("mapswithme://point/?lon=27.666&lat=53.666&zoom=10", info);
  TEST(info.IsValid(), ());
  TEST_ALMOST_EQUAL(info.m_lat, 53.666, ());
  TEST_ALMOST_EQUAL(info.m_lon, 27.666, ());
  TEST_ALMOST_EQUAL(info.m_zoom, 10.0, ());

  info.Reset();
  ParseURL("geo:53.666", info);
  TEST(!info.IsValid(), ());
}
