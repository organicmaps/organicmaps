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
      },
      ());
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
        "marker-color": "green"
      }
    },
    {
      /* MultiPoint feature is not supported and should be ignored */
      "type": "Feature",
      "geometry": {
        "coordinates": [
          [
            31.055034,
            29.989067
          ],
          [
            35.182237,
            31.773850
          ]
        ],
        "type": "MultiPoint"
      },
      "properties": {
        "marker-color": "green"
      }
    }
  ]
})";

  kml::FileData const dataFromText = LoadGeojsonFromString(input);

  TEST_EQUAL(dataFromText.m_bookmarksData.size(), 1, ());
  auto bookmark = dataFromText.m_bookmarksData.front();
  auto green = kml::ColorData{.m_predefinedColor = kml::PredefinedColor::Green, .m_rgba = 0x008000FF};
  TEST_EQUAL(bookmark.m_color, green, ());
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
  std::string_view constexpr input =
      R"({"type":"FeatureCollection","features":[{"type":"Feature","properties":{"marker-color":"#000000","label":"Hello GeoJson","description":"First import test"},"geometry":{"coordinates":[30.568097444337525,50.46385629798317],"type":"Point"},"id":0}]})";
  kml::FileData const dataFromText = LoadGeojsonFromString(input);

  TEST_EQUAL(dataFromText.m_bookmarksData.size(), 1, ());
  auto bookmark = dataFromText.m_bookmarksData.front();
  // We don't have PredefinedColor::Black option. So fallback to the closest one Brown
  auto brownColor = kml::ColorData{.m_predefinedColor = kml::PredefinedColor::Brown, .m_rgba = 0x00000FF};
  TEST_EQUAL(bookmark.m_color, brownColor, ());
  TEST_EQUAL(kml::GetDefaultStr(bookmark.m_name), "Hello GeoJson", ());
  TEST(bookmark.m_point.EqualDxDy(mercator::FromLatLon(50.46385629798317, 30.568097444337525), 0.000001), ());
}

UNIT_TEST(GeoJson_Parse_UMapOptions)
{
  std::string_view constexpr input = R"({
  "type": "FeatureCollection",
  "features": [
    {
      "type": "Feature",
      "properties": {
        "name": "UMap Test Point",
        "_umap_options": {
          "iconClass": "Drop",
          "color": "MediumSeaGreen",
          "weight": 4,
          "opacity": 0.8,
          "customProperty": "should be preserved"
        }
      },
      "geometry": {
        "coordinates": [2.3522, 48.8566],
        "type": "Point"
      }
    },
    {
      "type": "Feature",
      "properties": {
        "name": "UMap Test Line",
        "_umap_options": {
          "color": "red",
          "weight": 2,
          "opacity": 0.5,
          "dashArray": "5,10"
        }
      },
      "geometry": {
        "coordinates": [
          [2.3522, 48.8566],
          [2.3540, 48.8580]
        ],
        "type": "LineString"
      }
    }
  ]
})";

  kml::FileData const dataFromText = LoadGeojsonFromString(input);

  // Check bookmark (Point)
  TEST_EQUAL(dataFromText.m_bookmarksData.size(), 1, ());
  auto const & bookmark = dataFromText.m_bookmarksData.front();
  TEST_EQUAL(kml::GetDefaultStr(bookmark.m_name), "UMap Test Point", ());

  // Check that _umap_options is stored as JSON string
  auto const umapOptionsIt = bookmark.m_properties.find("_umap_options");
  TEST(umapOptionsIt != bookmark.m_properties.end(), ("_umap_options should be stored in properties"));

  // Parse the stored JSON to verify it contains the expected data
  std::string const & umapOptionsStr = umapOptionsIt->second;
  glz::json_t umapOptionsJson;
  auto const parseResult = glz::read_json(umapOptionsJson, umapOptionsStr);
  TEST_EQUAL(parseResult, glz::error_code::none, ("Should be able to parse stored _umap_options JSON"));

  TEST(umapOptionsJson.is_object(), ("_umap_options should be an object"));
  auto const & umapObj = umapOptionsJson.get_object();

  // Verify individual properties are preserved
  TEST(umapObj.contains("iconClass"), ("iconClass should be preserved"));
  TEST(umapObj.contains("customProperty"), ("customProperty should be preserved"));
  TEST_EQUAL(umapObj.at("iconClass").get_string(), "Drop", ("iconClass value should be preserved"));
  TEST_EQUAL(umapObj.at("customProperty").get_string(), "should be preserved",
             ("customProperty value should be preserved"));

  // Check track (LineString)
  TEST_EQUAL(dataFromText.m_tracksData.size(), 1, ());
  auto const & track = dataFromText.m_tracksData.front();
  TEST_EQUAL(kml::GetDefaultStr(track.m_name), "UMap Test Line", ());

  // Check that _umap_options is stored for tracks as well
  auto const trackUmapOptionsIt = track.m_properties.find("_umap_options");
  TEST(trackUmapOptionsIt != track.m_properties.end(), ("_umap_options should be stored in track properties"));

  // Parse the stored JSON to verify it contains the expected data
  std::string const & trackUmapOptionsStr = trackUmapOptionsIt->second;
  glz::json_t trackUmapOptionsJson;
  auto const trackParseResult = glz::read_json(trackUmapOptionsJson, trackUmapOptionsStr);
  TEST_EQUAL(trackParseResult, glz::error_code::none, ("Should be able to parse stored track _umap_options JSON"));

  TEST(trackUmapOptionsJson.is_object(), ("track _umap_options should be an object"));
  auto const & trackUmapObj = trackUmapOptionsJson.get_object();

  // Verify track properties are preserved
  TEST(trackUmapObj.contains("dashArray"), ("dashArray should be preserved"));
  TEST_EQUAL(trackUmapObj.at("dashArray").get_string(), "5,10", ("dashArray value should be preserved"));
  TEST_EQUAL(trackUmapObj.at("color").get_string(), "red", ("color value should be preserved"));
  TEST_EQUAL(trackUmapObj.at("weight").as<int>(), 2, ("weight value should be preserved"));
  TEST_EQUAL(trackUmapObj.at("opacity").as<double>(), 0.5, ("opacity value should be preserved"));
}

UNIT_TEST(GeoJson_Parse_FromGoogle)
{
  std::string_view constexpr input = R"({
  "type": "FeatureCollection",
  "features": [
    {
      "geometry": {
        "coordinates": [
          -0.1195192,
          51.5031864
        ],
        "type": "Point"
      },
      "properties": {
        "date": "2025-11-17T09:06:04Z",
        "google_maps_url": "http://maps.google.com/?cid=4796882358840715922",
        "location": {
          "address": "Riverside Building, County Hall, Westminster Bridge Rd, London SE1 7PB, United Kingdom",
          "country_code": "GB",
          "name": "London Eye"
        }
      },
      "type": "Feature"
    },
    {
      "geometry": {
        "coordinates": [
          0,
          0
        ],
        "type": "Point"
      },
      "properties": {
        "date": "2025-11-17T09:08:03Z",
        "google_maps_url": "http://maps.google.com/?q=41.993752,5.326894",
        "Comment": "No location information is available for this saved place"
      },
      "type": "Feature"
    },
    {
      "geometry": {
        "coordinates": [
          0,
          0
        ],
        "type": "Point"
      },
      "properties": {
        "date": "2025-11-17T09:06:35Z",
        "google_maps_url": "http://maps.google.com/?q=00120+Vatican+City&ftid=0x1325890a57d42d3d:0x94f9ab23a7eb0",
        "Comment": "No location information is available for this saved place"
      },
      "type": "Feature"
    }
  ]
})";

  kml::FileData const dataFromText = LoadGeojsonFromString(input);

  // Check bookmark (Point)
  TEST_EQUAL(dataFromText.m_bookmarksData.size(), 3, ());

  // Check bookmark with coodinates and Google link
  auto const & londonEyeBookmark = dataFromText.m_bookmarksData[0];
  TEST_EQUAL(kml::GetDefaultStr(londonEyeBookmark.m_name), "London Eye", ());
  TEST_EQUAL(kml::GetDefaultStr(londonEyeBookmark.m_description),
             "<a href=\"https://maps.google.com/?cid=4796882358840715922\">London Eye</a>", ());

  // Check bookmark Google link
  auto const & bookmark = dataFromText.m_bookmarksData[1];
  TEST(bookmark.m_point.EqualDxDy(mercator::FromLatLon(41.993752, 5.326894), 0.000001), ());
  TEST_EQUAL(kml::GetDefaultStr(bookmark.m_description), "https://maps.google.com/?q=41.993752,5.326894", ());

  // Check bookmark Google link
  auto const & vaticanBookmark = dataFromText.m_bookmarksData[2];
  TEST(vaticanBookmark.m_point.EqualDxDy(mercator::FromLatLon(0, 0), 0.000001), ());
  TEST_EQUAL(kml::GetDefaultStr(vaticanBookmark.m_description),
             "https://maps.google.com/?q=00120+Vatican+City&ftid=0x1325890a57d42d3d:0x94f9ab23a7eb0", ());
}

}  // namespace geojson_tests
