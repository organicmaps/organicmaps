#include "testing/testing.hpp"

#include "ge0/geo_url_parser.hpp"


namespace geo_url_tests
{
double const kEps = 1e-10;

using namespace geo;

UNIT_TEST(GeoURL_Smoke)
{
  GeoURLInfo info;

  info.Parse("geo:53.666,27.666");
  TEST(info.IsValid(), ());
  TEST_ALMOST_EQUAL_ABS(info.m_lat, 53.666, kEps, ());
  TEST_ALMOST_EQUAL_ABS(info.m_lon, 27.666, kEps, ());

  info.Parse("geo://point/?lon=27.666&lat=53.666&zoom=10");
  TEST(info.IsValid(), ());
  TEST_ALMOST_EQUAL_ABS(info.m_lat, 53.666, kEps, ());
  TEST_ALMOST_EQUAL_ABS(info.m_lon, 27.666, kEps, ());
  TEST_ALMOST_EQUAL_ABS(info.m_zoom, 10.0, kEps, ());

  info.Parse("geo:53.666");
  TEST(!info.IsValid(), ());

  info.Parse("mapswithme:123.33,32.22/showmethemagic");
  TEST(!info.IsValid(), ());

  info.Parse("mapswithme:32.22, 123.33/showmethemagic");
  TEST(info.IsValid(), ());
  TEST_ALMOST_EQUAL_ABS(info.m_lat, 32.22, kEps, ());
  TEST_ALMOST_EQUAL_ABS(info.m_lon, 123.33, kEps, ());

  info.Parse("model: iphone 7,1");
  TEST(!info.IsValid(), ());
}

UNIT_TEST(GeoURL_Instagram)
{
  GeoURLInfo info;
  info.Parse("geo:0,0?z=14&q=54.683486138,25.289361259 (Forto%20dvaras)");
  TEST(info.IsValid(), ());
  TEST_ALMOST_EQUAL_ABS(info.m_lat, 54.683486138, kEps, ());
  TEST_ALMOST_EQUAL_ABS(info.m_lon, 25.289361259, kEps, ());
  TEST_ALMOST_EQUAL_ABS(info.m_zoom, 14.0, kEps, ());
}

UNIT_TEST(GeoURL_GoogleMaps)
{
  GeoURLInfo info;

  info.Parse("https://maps.google.com/maps?z=16&q=Mezza9%401.3067198,103.83282");
  TEST(info.IsValid(), ());
  TEST_ALMOST_EQUAL_ABS(info.m_lat, 1.3067198, kEps, ());
  TEST_ALMOST_EQUAL_ABS(info.m_lon, 103.83282, kEps, ());
  TEST_ALMOST_EQUAL_ABS(info.m_zoom, 16.0, kEps, ());

  info.Parse("https://maps.google.com/maps?z=16&q=House+of+Seafood+%40+180%401.356706,103.87591");
  TEST(info.IsValid(), ());
  TEST_ALMOST_EQUAL_ABS(info.m_lat, 1.356706, kEps, ());
  TEST_ALMOST_EQUAL_ABS(info.m_lon, 103.87591, kEps, ());
  TEST_ALMOST_EQUAL_ABS(info.m_zoom, 16.0, kEps, ());

  info.Parse("https://www.google.com/maps/place/Falafel+M.+Sahyoun/@33.8904447,35.5044618,16z");
  TEST(info.IsValid(), ());
  TEST_ALMOST_EQUAL_ABS(info.m_lat, 33.8904447, kEps, ());
  TEST_ALMOST_EQUAL_ABS(info.m_lon, 35.5044618, kEps, ());
  // Sic: zoom is not parsed
  //TEST_ALMOST_EQUAL_ABS(info.m_zoom, 16.0, kEps, ());

  info.Parse("https://www.google.com/maps?q=55.751809,37.6130029");
  TEST(info.IsValid(), ());
  TEST_ALMOST_EQUAL_ABS(info.m_lat, 55.751809, kEps, ());
  TEST_ALMOST_EQUAL_ABS(info.m_lon, 37.6130029, kEps, ());
}

UNIT_TEST(GeoURL_2GIS)
{
  GeoURLInfo info;

  info.Parse("https://2gis.ru/moscow/firm/4504127908589159/center/37.6186,55.7601/zoom/15.9764");
  TEST(info.IsValid(), ());
  TEST_ALMOST_EQUAL_ABS(info.m_lat, 55.7601, kEps, ());
  TEST_ALMOST_EQUAL_ABS(info.m_lon, 37.6186, kEps, ());
  TEST_ALMOST_EQUAL_ABS(info.m_zoom, 15.9764, kEps, ());

  info.Parse("https://2gis.ru/moscow/firm/4504127908589159/center/37,55/zoom/15");
  TEST(info.IsValid(), ());
  TEST_ALMOST_EQUAL_ABS(info.m_lat, 55.0, kEps, ());
  TEST_ALMOST_EQUAL_ABS(info.m_lon, 37.0, kEps, ());
  TEST_ALMOST_EQUAL_ABS(info.m_zoom, 15.0, kEps, ());

  info.Parse("https://2gis.ru/moscow/firm/4504127908589159?m=37.618632%2C55.760069%2F15.232");
  TEST(info.IsValid(), ());
  TEST_ALMOST_EQUAL_ABS(info.m_lat, 55.760069, kEps, ());
  TEST_ALMOST_EQUAL_ABS(info.m_lon, 37.618632, kEps, ());
  TEST_ALMOST_EQUAL_ABS(info.m_zoom, 15.232, kEps, ());

  info.Parse("https://2gis.ru/moscow/firm/4504127908589159?m=37.618632%2C55.760069%2F15");
  TEST(info.IsValid(), ());
  TEST_ALMOST_EQUAL_ABS(info.m_lat, 55.760069, kEps, ());
  TEST_ALMOST_EQUAL_ABS(info.m_lon, 37.618632, kEps, ());
  TEST_ALMOST_EQUAL_ABS(info.m_zoom, 15.0, kEps, ());
}

UNIT_TEST(GeoURL_OpenStreetMap)
{
  GeoURLInfo info;

  info.Parse("https://www.openstreetmap.org/#map=16/33.89041/35.50664");
  TEST(info.IsValid(), ());
  TEST_ALMOST_EQUAL_ABS(info.m_lat, 33.89041, kEps, ());
  TEST_ALMOST_EQUAL_ABS(info.m_lon, 35.50664, kEps, ());
  TEST_ALMOST_EQUAL_ABS(info.m_zoom, 16.0, kEps, ());

  info.Parse("https://www.openstreetmap.org/search?query=Falafel%20Sahyoun#map=16/33.89041/35.50664");
  TEST(info.IsValid(), ());
  TEST_ALMOST_EQUAL_ABS(info.m_lat, 33.89041, kEps, ());
  TEST_ALMOST_EQUAL_ABS(info.m_lon, 35.50664, kEps, ());
  TEST_ALMOST_EQUAL_ABS(info.m_zoom, 16.0, kEps, ());
}

UNIT_TEST(GeoURL_CaseInsensitive)
{
  GeoURLInfo info;
  info.Parse("geo:52.23405,21.01547?Z=10");
  TEST(info.IsValid(), ());
  TEST_ALMOST_EQUAL_ABS(info.m_lat, 52.23405, kEps, ());
  TEST_ALMOST_EQUAL_ABS(info.m_lon, 21.01547, kEps, ());
  TEST_ALMOST_EQUAL_ABS(info.m_zoom, 10.0, kEps, ());
}

UNIT_TEST(GeoURL_BadZoom)
{
  GeoURLInfo info;

  info.Parse("geo:52.23405,21.01547?Z=19");
  TEST(info.IsValid(), ());
  TEST_ALMOST_EQUAL_ABS(info.m_lat, 52.23405, kEps, ());
  TEST_ALMOST_EQUAL_ABS(info.m_lon, 21.01547, kEps, ());
  TEST_ALMOST_EQUAL_ABS(info.m_zoom, 17.0, kEps, ());

  info.Parse("geo:52.23405,21.01547?Z=nineteen");
  TEST(info.IsValid(), ());
  TEST_ALMOST_EQUAL_ABS(info.m_lat, 52.23405, kEps, ());
  TEST_ALMOST_EQUAL_ABS(info.m_lon, 21.01547, kEps, ());
  TEST_ALMOST_EQUAL_ABS(info.m_zoom, 17.0, kEps, ());

  info.Parse("geo:52.23405,21.01547?Z=-1");
  TEST(info.IsValid(), ());
  TEST_ALMOST_EQUAL_ABS(info.m_lat, 52.23405, kEps, ());
  TEST_ALMOST_EQUAL_ABS(info.m_lon, 21.01547, kEps, ());
  TEST_ALMOST_EQUAL_ABS(info.m_zoom, 0.0, kEps, ());
}
} // namespace geo_url_tests
