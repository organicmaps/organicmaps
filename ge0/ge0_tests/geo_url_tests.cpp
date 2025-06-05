#include "testing/testing.hpp"

#include "ge0/geo_url_parser.hpp"

namespace geo_url_tests
{
double const kEps = 1e-10;

using namespace geo;

UNIT_TEST(GeoUrl_Geo)
{
  UnifiedParser parser;
  GeoURLInfo info;

  // Bare RFC5870 URI with lat,lon.
  TEST(parser.Parse("geo:53.666,-27.666", info), ());
  TEST(info.IsLatLonValid(), ());
  TEST_ALMOST_EQUAL_ABS(info.m_lat, 53.666, kEps, ());
  TEST_ALMOST_EQUAL_ABS(info.m_lon, -27.666, kEps, ());

  // Bare RFC5870 URI with lat,lon,altitude.
  TEST(parser.Parse("geo:53.666,-27.666,1000", info), ());
  TEST(info.IsLatLonValid(), ());
  TEST_ALMOST_EQUAL_ABS(info.m_lat, 53.666, kEps, ());
  TEST_ALMOST_EQUAL_ABS(info.m_lon, -27.666, kEps, ());
  // Altitude is ignored.

  // OSMAnd on Android 2023-11-12.
  TEST(parser.Parse("geo:35.34156,33.32210?z=16", info), ());
  TEST(info.IsLatLonValid(), ());
  TEST_ALMOST_EQUAL_ABS(info.m_lat, 35.34156, kEps, ());
  TEST_ALMOST_EQUAL_ABS(info.m_lon, 33.32210, kEps, ());
  TEST_ALMOST_EQUAL_ABS(info.m_zoom, 16.0, kEps, ());

  // Bare RFC5870 URI with a space between lat, lon.
  TEST(parser.Parse("geo:53.666, -27.666", info), ());
  TEST(info.IsLatLonValid(), ());
  TEST_ALMOST_EQUAL_ABS(info.m_lat, 53.666, kEps, ());
  TEST_ALMOST_EQUAL_ABS(info.m_lon, -27.666, kEps, ());

  // Google's q= extension: 0,0 are treated as special values.
  TEST(parser.Parse("geo:0,0?q=Kyrenia%20Castle&z=15", info), ());
  TEST(!info.IsLatLonValid(), ());
  TEST_EQUAL(info.m_query, "Kyrenia Castle", ());
  TEST_EQUAL(info.m_zoom, 15, ());

  // Coordinates in q= parameter.
  TEST(parser.Parse("geo:0,0?z=14&q=-54.683486138,25.289361259", info), ());
  TEST(info.IsLatLonValid(), ());
  TEST_ALMOST_EQUAL_ABS(info.m_lat, -54.683486138, kEps, ());
  TEST_ALMOST_EQUAL_ABS(info.m_lon, 25.289361259, kEps, ());
  TEST_ALMOST_EQUAL_ABS(info.m_zoom, 14.0, kEps, ());

  // Instagram on Android 2023-11-12.
  TEST(parser.Parse("geo:?q=35.20488357543945, 33.345027923583984", info), ());
  TEST(info.IsLatLonValid(), ());
  TEST_ALMOST_EQUAL_ABS(info.m_lat, 35.20488357543945, kEps, ());
  TEST_ALMOST_EQUAL_ABS(info.m_lon, 33.345027923583984, kEps, ());

  // Invalid lat,lon in q= parameter is saved as a query.
  TEST(parser.Parse("geo:0,0?z=14&q=-100500.232,1213.232", info), ());
  TEST(!info.IsLatLonValid(), ());
  TEST_EQUAL(info.m_query, "-100500.232,1213.232", ());
  TEST_ALMOST_EQUAL_ABS(info.m_zoom, 14.0, kEps, ());

  // RFC5870 additional parameters are ignored.
  TEST(parser.Parse("geo:48.198634,16.371648;crs=wgs84;u=40", info), ());
  TEST(info.IsLatLonValid(), ());
  TEST_ALMOST_EQUAL_ABS(info.m_lat, 48.198634, kEps, ());
  TEST_ALMOST_EQUAL_ABS(info.m_lon, 16.371648, kEps, ());
  // ;crs=wgs84;u=40 tail is ignored

  // 0,0 are not treated as special values if q is empty.
  TEST(parser.Parse("geo:0,0", info), ());
  TEST_ALMOST_EQUAL_ABS(info.m_lat, 0.0, kEps, ());
  TEST_ALMOST_EQUAL_ABS(info.m_lon, 0.0, kEps, ());
  TEST_EQUAL(info.m_query, "", ());

  // 0,0 are not treated as special values if q is empty.
  TEST(parser.Parse("geo:0,0?q=", info), ());
  TEST_ALMOST_EQUAL_ABS(info.m_lat, 0.0, kEps, ());
  TEST_ALMOST_EQUAL_ABS(info.m_lon, 0.0, kEps, ());
  TEST_EQUAL(info.m_query, "", ());

  // Lat is 0.
  TEST(parser.Parse("geo:0,16.371648", info), ());
  TEST(info.IsLatLonValid(), ());
  TEST_ALMOST_EQUAL_ABS(info.m_lat, 0.0, kEps, ());
  TEST_ALMOST_EQUAL_ABS(info.m_lon, 16.371648, kEps, ());

  // Lon is 0.
  TEST(parser.Parse("geo:53.666,0", info), ());
  TEST(info.IsLatLonValid(), ());
  TEST_ALMOST_EQUAL_ABS(info.m_lat, 53.666, kEps, ());
  TEST_ALMOST_EQUAL_ABS(info.m_lon, 0.0, kEps, ());

  // URL Encoded comma (%2C) as delimiter
  TEST(parser.Parse("geo:-18.9151863%2C-48.28712359999999?q=-18.9151863%2C-48.28712359999999", info), ());
  TEST(info.IsLatLonValid(), ());
  TEST_ALMOST_EQUAL_ABS(info.m_lat, -18.9151863, kEps, ());
  TEST_ALMOST_EQUAL_ABS(info.m_lon, -48.28712359999999, kEps, ());

  // URL Encoded comma (%2C) and space (%20)
  TEST(parser.Parse("geo:-18.9151863%2C%20-48.28712359999999?q=-18.9151863%2C-48.28712359999999", info), ());
  TEST(info.IsLatLonValid(), ());
  TEST_ALMOST_EQUAL_ABS(info.m_lat, -18.9151863, kEps, ());
  TEST_ALMOST_EQUAL_ABS(info.m_lon, -48.28712359999999, kEps, ());

  // URL Encoded comma (%2C) and space as +
  TEST(parser.Parse("geo:-18.9151863%2C+-48.28712359999999?q=-18.9151863%2C-48.28712359999999", info), ());
  TEST(info.IsLatLonValid(), ());
  TEST_ALMOST_EQUAL_ABS(info.m_lat, -18.9151863, kEps, ());
  TEST_ALMOST_EQUAL_ABS(info.m_lon, -48.28712359999999, kEps, ());

  // URL encoded with altitude
  TEST(parser.Parse("geo:53.666%2C-27.666%2C+1000", info), ());
  TEST(info.IsLatLonValid(), ());
  TEST_ALMOST_EQUAL_ABS(info.m_lat, 53.666, kEps, ());
  TEST_ALMOST_EQUAL_ABS(info.m_lon, -27.666, kEps, ());

  TEST(parser.Parse("geo:-32.899583,139.043969&z=12", info), ("& instead of ? from a user report"));
  TEST(info.IsLatLonValid(), ());
  TEST_ALMOST_EQUAL_ABS(info.m_lat, -32.899583, kEps, ());
  TEST_ALMOST_EQUAL_ABS(info.m_lon, 139.043969, kEps, ());
  TEST_EQUAL(info.m_zoom, 12, ());

  // Invalid coordinates.
  TEST(!parser.Parse("geo:0,0garbage", info), ());
  TEST(!parser.Parse("geo:garbage0,0", info), ());
  TEST(!parser.Parse("geo:53.666", info), ());

  // Garbage.
  TEST(!parser.Parse("geo:", info), ());

  TEST(!parser.Parse("geo:", info), ());
  TEST(!parser.Parse("geo:random,garbage", info), ());
  TEST(!parser.Parse("geo://random,garbage", info), ());
  TEST(!parser.Parse("geo://point/?lon=27.666&lat=53.666&zoom=10", info), ());
}

UNIT_TEST(GeoURL_GeoLabel)
{
  UnifiedParser parser;
  GeoURLInfo info;

  TEST(parser.Parse("geo:0,0?z=14&q=-54.683486138,25.289361259 (Forto%20dvaras)", info), ());
  TEST_ALMOST_EQUAL_ABS(info.m_lat, -54.683486138, kEps, ());
  TEST_ALMOST_EQUAL_ABS(info.m_lon, 25.289361259, kEps, ());
  TEST_ALMOST_EQUAL_ABS(info.m_zoom, 14.0, kEps, ());
  TEST_EQUAL(info.m_label, "Forto dvaras", ());

  TEST(parser.Parse("geo:0,0?z=14&q=-54.683486138,25.289361259(Forto%20dvaras)", info), ());
  TEST_ALMOST_EQUAL_ABS(info.m_lat, -54.683486138, kEps, ());
  TEST_ALMOST_EQUAL_ABS(info.m_lon, 25.289361259, kEps, ());
  TEST_ALMOST_EQUAL_ABS(info.m_zoom, 14.0, kEps, ());
  TEST_EQUAL(info.m_label, "Forto dvaras", ());

  TEST(parser.Parse("geo:0,0?q=-54.683486138,25.289361259&z=14     (Forto%20dvaras)", info), ());
  TEST_ALMOST_EQUAL_ABS(info.m_lat, -54.683486138, kEps, ());
  TEST_ALMOST_EQUAL_ABS(info.m_lon, 25.289361259, kEps, ());
  TEST_ALMOST_EQUAL_ABS(info.m_zoom, 14.0, kEps, ());
  TEST_EQUAL(info.m_label, "Forto dvaras", ());

  TEST(parser.Parse("geo:0,0(Forto%20dvaras)", info), ());
  TEST(info.IsLatLonValid(), ());
  TEST_ALMOST_EQUAL_ABS(info.m_lat, 0.0, kEps, ());
  TEST_ALMOST_EQUAL_ABS(info.m_lon, 0.0, kEps, ());
  TEST_EQUAL(info.m_label, "Forto dvaras", ());

  TEST(parser.Parse("geo:0,0?q=Forto%20dvaras)", info), ());
  TEST(!info.IsLatLonValid(), ());
  TEST_EQUAL(info.m_query, "Forto dvaras)", ());

  TEST(!parser.Parse("geo:(Forto%20dvaras)", info), ());
}

UNIT_TEST(GeoURL_GoogleMaps)
{
  UnifiedParser parser;
  GeoURLInfo info;

  TEST(parser.Parse("https://maps.google.com/maps?z=16&q=Mezza9%401.3067198,103.83282", info), ());
  TEST_ALMOST_EQUAL_ABS(info.m_lat, 1.3067198, kEps, ());
  TEST_ALMOST_EQUAL_ABS(info.m_lon, 103.83282, kEps, ());
  TEST_ALMOST_EQUAL_ABS(info.m_zoom, 16.0, kEps, ());

  TEST(parser.Parse("https://maps.google.com/maps?z=16&q=House+of+Seafood+%40+180%40-1.356706,103.87591", info), ());
  TEST_ALMOST_EQUAL_ABS(info.m_lat, -1.356706, kEps, ());
  TEST_ALMOST_EQUAL_ABS(info.m_lon, 103.87591, kEps, ());
  TEST_ALMOST_EQUAL_ABS(info.m_zoom, 16.0, kEps, ());

  TEST(parser.Parse("https://www.google.com/maps/place/Falafel+M.+Sahyoun/@33.8904447,35.5044618,16z", info), ());
  TEST_ALMOST_EQUAL_ABS(info.m_lat, 33.8904447, kEps, ());
  TEST_ALMOST_EQUAL_ABS(info.m_lon, 35.5044618, kEps, ());
  // Sic: zoom is not parsed
  // TEST_ALMOST_EQUAL_ABS(info.m_zoom, 16.0, kEps, ());

  TEST(parser.Parse("https://www.google.com/maps?q=55.751809,-37.6130029", info), ());
  TEST_ALMOST_EQUAL_ABS(info.m_lat, 55.751809, kEps, ());
  TEST_ALMOST_EQUAL_ABS(info.m_lon, -37.6130029, kEps, ());
}

UNIT_TEST(GeoURL_Yandex)
{
  UnifiedParser parser;
  GeoURLInfo info;

  TEST(parser.Parse("https://yandex.ru/maps/213/moscow/?ll=37.000000%2C55.000000&z=10", info), ());
  TEST_ALMOST_EQUAL_ABS(info.m_lat, 55.0, kEps, ());
  TEST_ALMOST_EQUAL_ABS(info.m_lon, 37.0, kEps, ());
  TEST_ALMOST_EQUAL_ABS(info.m_zoom, 10.0, kEps, ());
}

UNIT_TEST(GeoURL_2GIS)
{
  UnifiedParser parser;
  GeoURLInfo info;

  TEST(parser.Parse("https://2gis.ru/moscow/firm/4504127908589159/center/37.6186,55.7601/zoom/15.9764", info), ());
  TEST_ALMOST_EQUAL_ABS(info.m_lat, 55.7601, kEps, ());
  TEST_ALMOST_EQUAL_ABS(info.m_lon, 37.6186, kEps, ());
  TEST_ALMOST_EQUAL_ABS(info.m_zoom, 15.9764, kEps, ());

  TEST(parser.Parse("https://2gis.ru/moscow/firm/4504127908589159/center/-37,55/zoom/15", info), ());
  TEST_ALMOST_EQUAL_ABS(info.m_lat, 55.0, kEps, ());
  TEST_ALMOST_EQUAL_ABS(info.m_lon, -37.0, kEps, ());
  TEST_ALMOST_EQUAL_ABS(info.m_zoom, 15.0, kEps, ());

  TEST(parser.Parse("https://2gis.ru/moscow/firm/4504127908589159?m=37.618632%2C55.760069%2F15.232", info), ());
  TEST_ALMOST_EQUAL_ABS(info.m_lat, 55.760069, kEps, ());
  TEST_ALMOST_EQUAL_ABS(info.m_lon, 37.618632, kEps, ());
  TEST_ALMOST_EQUAL_ABS(info.m_zoom, 15.232, kEps, ());

  TEST(parser.Parse("https://2gis.ru/moscow/firm/4504127908589159?m=37.618632%2C-55.760069%2F15", info), ());
  TEST_ALMOST_EQUAL_ABS(info.m_lat, -55.760069, kEps, ());
  TEST_ALMOST_EQUAL_ABS(info.m_lon, 37.618632, kEps, ());
  TEST_ALMOST_EQUAL_ABS(info.m_zoom, 15.0, kEps, ());
}

UNIT_TEST(GeoURL_LatLon)
{
  UnifiedParser parser;
  GeoURLInfo info;

  TEST(!parser.Parse("mapswithme:123.33,32.22/showmethemagic", info), ());
  TEST(!parser.Parse("mapswithme:32.22, 123.33/showmethemagic", info), ());
  TEST(!parser.Parse("model: iphone 7,1", info), ());
}

UNIT_TEST(GeoURL_OpenStreetMap)
{
  UnifiedParser parser;
  GeoURLInfo info;

  TEST(parser.Parse("https://www.openstreetmap.org/#map=16/33.89041/35.50664", info), ());
  TEST_ALMOST_EQUAL_ABS(info.m_lat, 33.89041, kEps, ());
  TEST_ALMOST_EQUAL_ABS(info.m_lon, 35.50664, kEps, ());
  TEST_ALMOST_EQUAL_ABS(info.m_zoom, 16.0, kEps, ());

  TEST(parser.Parse("https://www.openstreetmap.org/search?query=Falafel%20Sahyoun#map=16/33.89041/35.50664", info), ());
  TEST_ALMOST_EQUAL_ABS(info.m_lat, 33.89041, kEps, ());
  TEST_ALMOST_EQUAL_ABS(info.m_lon, 35.50664, kEps, ());
  TEST_ALMOST_EQUAL_ABS(info.m_zoom, 16.0, kEps, ());

  TEST(parser.Parse("https://www.openstreetmap.org/#map=21/53.90323/-27.55806", info), ());
  TEST_ALMOST_EQUAL_ABS(info.m_lat, 53.90323, kEps, ());
  TEST_ALMOST_EQUAL_ABS(info.m_lon, -27.55806, kEps, ());
  TEST_ALMOST_EQUAL_ABS(info.m_zoom, 20.0, kEps, ());

  TEST(parser.Parse("https://www.openstreetmap.org/way/45394171#map=10/34.67379/33.04422", info), ());
  TEST_ALMOST_EQUAL_ABS(info.m_lat, 34.67379, kEps, ());
  TEST_ALMOST_EQUAL_ABS(info.m_lon, 33.04422, kEps, ());
  TEST_ALMOST_EQUAL_ABS(info.m_zoom, 10.0, kEps, ());

  // Links in such format are generated by
  // https://geohack.toolforge.org/geohack.php?pagename=White_House&params=38_53_52_N_77_02_11_W_type:landmark_region:US-DC
  TEST(parser.Parse("https://www.openstreetmap.org/index.html?mlat=48.277222&mlon=24.152222&zoom=15", info), ());
  TEST_ALMOST_EQUAL_ABS(info.m_lat, 48.277222, kEps, ());
  TEST_ALMOST_EQUAL_ABS(info.m_lon, 24.152222, kEps, ());
  TEST_ALMOST_EQUAL_ABS(info.m_zoom, 15.0, kEps, ());
}

UNIT_TEST(GeoURL_BadZoom)
{
  UnifiedParser parser;
  GeoURLInfo info;

  TEST(parser.Parse("geo:52.23405,21.01547?z=22", info), ());
  TEST_ALMOST_EQUAL_ABS(info.m_lat, 52.23405, kEps, ());
  TEST_ALMOST_EQUAL_ABS(info.m_lon, 21.01547, kEps, ());
  TEST_ALMOST_EQUAL_ABS(info.m_zoom, 20.0, kEps, ());

  TEST(parser.Parse("geo:-52.23405,21.01547?z=nineteen", info), ());
  TEST_ALMOST_EQUAL_ABS(info.m_lat, -52.23405, kEps, ());
  TEST_ALMOST_EQUAL_ABS(info.m_lon, 21.01547, kEps, ());
  TEST_ALMOST_EQUAL_ABS(info.m_zoom, 0.0, kEps, ());

  TEST(parser.Parse("geo:52.23405,21.01547?z=-1", info), ());
  TEST_ALMOST_EQUAL_ABS(info.m_lat, 52.23405, kEps, ());
  TEST_ALMOST_EQUAL_ABS(info.m_lon, 21.01547, kEps, ());
  TEST_GREATER_OR_EQUAL(info.m_zoom, 0.0, ());
}
}  // namespace geo_url_tests
