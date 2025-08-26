#include "testing/testing.hpp"

#include "geometry/mercator.hpp"

#include "kml/serdes_geojson.hpp"

namespace geojson_tests
{

static kml::FileData LoadGeojsonFromString(std::string_view content)
{
  TEST_NO_THROW(
  {
    kml::FileData dataFromText;
    kml::DeserializerGeoJson des(dataFromText);
    des.Deserialize(content);
    return dataFromText;
  }, ());
}

UNIT_TEST(GeoJson_Parse_Basic)
{
  std::string_view constexpr input = R"({
  "type": "FeatureCollection",
  "features": [
    {
      "type": "Feature",
      "properties": {
        "stroke": "blue" /* Line color */
      },
      "geometry": {
        "coordinates": [
          [
            14.949382505528291,
            8.16007148457335
          ],
          [
            26.888888114204264,
            9.708105796659268
          ],
          [
            37.54707497642465,
            6.884595662842159
          ]
        ],
        "type": "LineString"
      }
    },
    {
      "type": "Feature",
      "geometry": {
        "coordinates": [
          31.02177966625902,
          29.8310316130992
        ],
        "type": "Point"
      },
      "properties": {
        "name": "Bookmark 1",
        /* Bookmark color */
        "marker-color": "red"
      }
    }
  ]
})";

  kml::FileData const dataFromText = LoadGeojsonFromString(input);

  TEST_EQUAL(dataFromText.m_bookmarksData.size(), 1, ());
  auto bookmark = dataFromText.m_bookmarksData.front();
  TEST_EQUAL(bookmark.m_color, kml::ColorData{.m_rgba = 0xFF0000FF}, ());
  TEST_EQUAL(kml::GetDefaultStr(bookmark.m_name), "Bookmark 1", ());
  TEST_EQUAL(bookmark.m_point, mercator::FromLatLon(29.8310316130992, 31.02177966625902), ());

  TEST_EQUAL(dataFromText.m_tracksData.size(), 1, ());
  auto track = dataFromText.m_tracksData.front();
  TEST_EQUAL(track.m_layers.front().m_color, kml::ColorData{.m_rgba = 0x0000FFFF}, ());
  TEST_EQUAL(track.m_geometry.m_lines.empty(), false, ());
  TEST_EQUAL(track.m_geometry.m_lines.front().size(), 3, ());
}

UNIT_TEST(GeoJson_Parse_basic_2)
{
    std::string_view constexpr input = R"({"type":"FeatureCollection","features":[{"type":"Feature","properties":{"marker-color":"#000000","label":"Hello GeoJson","description":"First import test"},"geometry":{"coordinates":[30.568097444337525,50.46385629798317],"type":"Point"},"id":0}]})";
    kml::FileData const dataFromText = LoadGeojsonFromString(input);

    TEST_EQUAL(dataFromText.m_bookmarksData.size(), 1, ());
}

}  // namespace geojson_tests
