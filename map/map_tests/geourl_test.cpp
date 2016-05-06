#include "testing/testing.hpp"

#include "map/geourl_process.hpp"


using namespace url_scheme;

UNIT_TEST(ProcessURL_Smoke)
{
  Info info;
  ParseGeoURL("geo:53.666,27.666", info);
  TEST(info.IsValid(), ());
  TEST_ALMOST_EQUAL_ULPS(info.m_lat, 53.666, ());
  TEST_ALMOST_EQUAL_ULPS(info.m_lon, 27.666, ());

  info.Reset();
  ParseGeoURL("geo://point/?lon=27.666&lat=53.666&zoom=10", info);
  TEST(info.IsValid(), ());
  TEST_ALMOST_EQUAL_ULPS(info.m_lat, 53.666, ());
  TEST_ALMOST_EQUAL_ULPS(info.m_lon, 27.666, ());
  TEST_ALMOST_EQUAL_ULPS(info.m_zoom, 10.0, ());

  info.Reset();
  ParseGeoURL("geo:53.666", info);
  TEST(!info.IsValid(), ());

  info.Reset();
  ParseGeoURL("mapswithme:123.33,32.22/showmethemagic", info);
  TEST(!info.IsValid(), ());

  info.Reset();
  ParseGeoURL("mapswithme:32.22, 123.33/showmethemagic", info);
  TEST(info.IsValid(), ());
  TEST_ALMOST_EQUAL_ULPS(info.m_lat, 32.22, ());
  TEST_ALMOST_EQUAL_ULPS(info.m_lon, 123.33, ());

  info.Reset();
  ParseGeoURL("model: iphone 7,1", info);
  TEST(!info.IsValid(), ());
}

UNIT_TEST(ProcessURL_Instagram)
{
  Info info;
  ParseGeoURL("geo:0,0?z=14&q=54.683486138,25.289361259 (Forto%20dvaras)", info);
  TEST(info.IsValid(), ());
  TEST_ALMOST_EQUAL_ULPS(info.m_lat, 54.683486138, ());
  TEST_ALMOST_EQUAL_ULPS(info.m_lon, 25.289361259, ());
  TEST_ALMOST_EQUAL_ULPS(info.m_zoom, 14.0, ());
}

UNIT_TEST(ProcessURL_GoogleMaps)
{
  Info info;
  ParseGeoURL("https://maps.google.com/maps?z=16&q=Mezza9%401.3067198,103.83282", info);
  TEST(info.IsValid(), ());
  TEST_ALMOST_EQUAL_ULPS(info.m_lat, 1.3067198, ());
  TEST_ALMOST_EQUAL_ULPS(info.m_lon, 103.83282, ());
  TEST_ALMOST_EQUAL_ULPS(info.m_zoom, 16.0, ());

  info.Reset();
  ParseGeoURL("https://maps.google.com/maps?z=16&q=House+of+Seafood+%40+180%401.356706,103.87591", info);
  TEST(info.IsValid(), ());
  TEST_ALMOST_EQUAL_ULPS(info.m_lat, 1.356706, ());
  TEST_ALMOST_EQUAL_ULPS(info.m_lon, 103.87591, ());
  TEST_ALMOST_EQUAL_ULPS(info.m_zoom, 16.0, ());
}
