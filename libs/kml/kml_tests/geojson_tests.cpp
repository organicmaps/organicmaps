#include "testing/testing.hpp"

#include "geometry/mercator.hpp"

#include "coding/string_utf8_multilang.hpp"
#include "kml/serdes_geojson.hpp"

namespace geojson_tests
{
kml::FileData GenerateKmlFileData();
kml::FileData GenerateKmlFileDataWithTrack();
kml::FileData GenerateKmlFileDataWithMultiTrack();

static kml::FileData LoadGeojsonFromString(std::string_view content)
{
  TEST_NO_THROW(
      {
        kml::FileData dataFromText;
        kml::GeoJsonReader des(dataFromText);
        des.Deserialize(content);
        return dataFromText;
      },
      ());
}

static std::string SaveToGeoJsonString(kml::FileData const & fileData, bool minimize = false)
{
  TEST_NO_THROW(
      {
        std::string buffer;
        MemWriter bufferWriter(buffer);
        kml::GeoJsonWriter exporter(bufferWriter);
        exporter.Write(fileData, minimize);
        return buffer;
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
      /* MultiLineString feature */
      "type": "Feature",
      "properties": {
        "stroke": "green"
      },
      "geometry": {
        "coordinates": [
          /* First line section */
          [
            [
              31.055034,
              29.989067
            ],
            [
              35.182237,
              31.773850
            ]
          ],
          /* Second line section */
          [
            [
              35.159882,
              31.755857
            ],
            [
              35.162575,
              31.749381
            ],
            [
              35.170106,
              31.746114
            ],
            [
              35.178316,
              31.746121
            ]
          ]
        ],
        "type": "MultiLineString"
      },
      "properties": {
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

  // Check track data.
  TEST_EQUAL(dataFromText.m_tracksData.size(), 2, ());
  auto track1 = dataFromText.m_tracksData[0];
  TEST_EQUAL(track1.m_layers[0].m_color, kml::ColorData{.m_rgba = 0x0000FFFF}, ());
  TEST_EQUAL(track1.m_geometry.m_lines.empty(), false, ());
  TEST_EQUAL(track1.m_geometry.m_lines.front().size(), 3, ());

  // Check multiline track data.
  auto track2 = dataFromText.m_tracksData[1];
  TEST_EQUAL(track2.m_layers[0].m_color, kml::ColorData{.m_rgba = 0x008000FF}, ());
  TEST_EQUAL(track2.m_geometry.m_lines.size(), 2, ());
  TEST_EQUAL(track2.m_geometry.m_lines[0].size(), 2, ());
  TEST_EQUAL(track2.m_geometry.m_lines[1].size(), 4, ());
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

UNIT_TEST(GeoJson_Writer_Simple)
{
  std::string_view constexpr expected_geojson = R"({
  "type": "FeatureCollection",
  "features": [
    {
      "type": "Feature",
      "geometry": {
        "type": "Point",
        "coordinates": [
          0,
          0
        ]
      },
      "properties": {
        "description": "Test bookmark description",
        "marker-color": "RoyalBlue",
        "name": "Marcador de prueba"
      }
    },
    {
      "type": "Feature",
      "geometry": {
        "type": "LineString",
        "coordinates": [
          [
            0,
            0
          ],
          [
            0,
            0
          ],
          [
            0,
            0
          ]
        ]
      },
      "properties": {
        "description": "Test track description",
        "name": "Test track",
        "stroke": "#FF0000"
      }
    }
  ]
})";

  kml::FileData testData = GenerateKmlFileDataWithTrack();
  auto jsonString = SaveToGeoJsonString(testData);

  TEST_EQUAL(jsonString, expected_geojson, ());
}

UNIT_TEST(GeoJson_Writer_MultiTrack)
{
  std::string_view constexpr expected_geojson = R"({
  "type": "FeatureCollection",
  "features": [
    {
      "type": "Feature",
      "geometry": {
        "type": "MultiLineString",
        "coordinates": [
          [
            [
              0,
              0
            ],
            [
              0,
              0
            ],
            [
              0,
              0
            ]
          ],
          [
            [
              0,
              0
            ],
            [
              0,
              0
            ],
            [
              0,
              0
            ],
            [
              0,
              0
            ]
          ]
        ]
      },
      "properties": {
        "description": "Test multitrack description",
        "name": "Test multitrack",
        "stroke": "#00FF00"
      }
    }
  ]
})";

  kml::FileData testData = GenerateKmlFileDataWithMultiTrack();
  testData.m_bookmarksData.clear();
  auto jsonString = SaveToGeoJsonString(testData);

  TEST_EQUAL(jsonString, expected_geojson, ());
}

UNIT_TEST(GeoJson_Writer_Simple_Minimized)
{
  // clang-format off
  std::string_view constexpr expected_geojson = R"({"type":"FeatureCollection","features":[{"type":"Feature","geometry":{"type":"Point","coordinates":[0,0]},"properties":{"description":"Test bookmark description","marker-color":"RoyalBlue","name":"Marcador de prueba"}}]})";
  // clang-format on

  kml::FileData const testData = GenerateKmlFileData();
  auto jsonString = SaveToGeoJsonString(testData, true);

  TEST_EQUAL(jsonString, expected_geojson, ());
}

UNIT_TEST(GeoJson_Writer_UMap)
{
  std::string_view constexpr expected_geojson = R"({
  "type": "FeatureCollection",
  "features": [
    {
      "type": "Feature",
      "geometry": {
        "type": "Point",
        "coordinates": [
          0,
          0
        ]
      },
      "properties": {
        "_umap_options": {
          "color": "RoyalBlue",
          "customProperty": "should be preserved",
          "iconClass": "Drop",
          "opacity": 0.8,
          "weight": 4
        },
        "description": "Test bookmark description",
        "marker-color": "RoyalBlue",
        "name": "Marcador de prueba"
      }
    },
    {
      "type": "Feature",
      "geometry": {
        "type": "LineString",
        "coordinates": [
          [
            0,
            0
          ],
          [
            0,
            0
          ],
          [
            0,
            0
          ]
        ]
      },
      "properties": {
        "_umap_options": {
          "color": "#FF0000",
          "dashArray": "5,10",
          "opacity": 0.5,
          "weight": 2
        },
        "description": "Test track description",
        "name": "Test track",
        "stroke": "#FF0000"
      }
    }
  ]
})";

  // NOTE: Glaze will sort keys alphabetically when exported to GeoJson
  auto const bookmark_umap_properties_str = R"(
  {
    "iconClass": "Drop",
    "color": "red",
    "weight": 4,
    "opacity": 0.8,
    "customProperty": "should be preserved"
  })";

  auto const track_umap_properties_str = R"(
  {
    "color": "red",
    "weight": 2,
    "opacity": 0.5,
    "dashArray": "5,10"
  })";

  kml::FileData testData = GenerateKmlFileDataWithTrack();

  // Add '_umap_options' to test data.
  testData.m_bookmarksData[0].m_properties["_umap_options"] = bookmark_umap_properties_str;
  testData.m_tracksData[0].m_properties["_umap_options"] = track_umap_properties_str;

  auto jsonString = SaveToGeoJsonString(testData);

  TEST_EQUAL(jsonString, expected_geojson, ());
}

UNIT_TEST(GeoJson_Writer_UMap_Invalid_Json)
{
  std::string_view constexpr expected_geojson = R"({
  "type": "FeatureCollection",
  "features": [
    {
      "type": "Feature",
      "geometry": {
        "type": "Point",
        "coordinates": [
          0,
          0
        ]
      },
      "properties": {
        "description": "Test bookmark description",
        "marker-color": "RoyalBlue",
        "name": "Marcador de prueba"
      }
    }
  ]
})";

  // Invalid JSON property will lead to warning. But no errors.
  auto const bookmark_umap_properties_str = R"({some $ invalid ^ json})";

  kml::FileData testData = GenerateKmlFileData();

  // Add '_umap_options' to test data.
  testData.m_bookmarksData[0].m_properties["_umap_options"] = bookmark_umap_properties_str;

  // Expecting some warning here.
  auto jsonString = SaveToGeoJsonString(testData);

  TEST_EQUAL(jsonString, expected_geojson, ());
}

kml::FileData GenerateKmlFileData()
{
  auto const kDefaultLang = StringUtf8Multilang::kDefaultCode;
  auto const kEnLang = StringUtf8Multilang::kEnglishCode;
  auto const kEsLang = static_cast<int8_t>(21);

  kml::FileData result;
  result.m_deviceId = "AAAA";
  result.m_serverId = "AAAA-BBBB-CCCC-DDDD";

  kml::BookmarkData bookmarkData;
  bookmarkData.m_name[kDefaultLang] = "Marcador de prueba";
  bookmarkData.m_name[kEnLang] = "Test bookmark";
  bookmarkData.m_description[kDefaultLang] = "Test bookmark description";
  bookmarkData.m_description[kEsLang] = "Descripción del marcador de prueba";
  bookmarkData.m_featureTypes = {718, 715};
  bookmarkData.m_customName[kDefaultLang] = "Mi lugar favorito";
  bookmarkData.m_customName[kEnLang] = "My favorite place";
  bookmarkData.m_color = {kml::PredefinedColor::Blue, 0};
  bookmarkData.m_icon = kml::BookmarkIcon::None;
  bookmarkData.m_timestamp = kml::TimestampClock::from_time_t(800);
  bookmarkData.m_point = mercator::FromLatLon(0.0, 0.0);  // TODO: Replace with real coordinates after Json Serialization is fixed
  bookmarkData.m_visible = false;
  bookmarkData.m_minZoom = 10;
  bookmarkData.m_properties = {{"bm_property1", "value1"}, {"bm_property2", "value2"}, {"score", "5"}};
  result.m_bookmarksData.emplace_back(std::move(bookmarkData));

  return result;
}

kml::FileData GenerateKmlFileDataWithTrack()
{
  auto const kDefaultLang = StringUtf8Multilang::kDefaultCode;
  auto const kEsLang = static_cast<int8_t>(21);

  kml::FileData result = GenerateKmlFileData();

  kml::TrackData trackData;
  trackData.m_localId = 0;
  trackData.m_name[kDefaultLang] = "Test track";
  trackData.m_name[kEsLang] = "Pista de prueba.";
  trackData.m_description[kDefaultLang] = "Test track description";
  trackData.m_description[kEsLang] = "Descripción de prueba de la pista.";
  trackData.m_layers = {{6.0, {kml::PredefinedColor::None, 0xff0000ff}},
                        {7.0, {kml::PredefinedColor::None, 0x00ff00ff}}};
  trackData.m_timestamp = kml::TimestampClock::from_time_t(900);

  // TODO: Replace with real coordinates after Json Serialization is fixed
  trackData.m_geometry.AddLine(
      {{mercator::FromLatLon(0, 0), 1}, {mercator::FromLatLon(0, 0), 2}, {mercator::FromLatLon(0, 0), 3}});

  trackData.m_properties = {{"tr_property1", "value1"}, {"tr_property2", "value2"}};
  result.m_tracksData.emplace_back(std::move(trackData));

  return result;
}

kml::FileData GenerateKmlFileDataWithMultiTrack()
{
  auto const kDefaultLang = StringUtf8Multilang::kDefaultCode;
  auto const kEsLang = static_cast<int8_t>(21);

  kml::FileData result = GenerateKmlFileData();

  kml::TrackData trackData;
  trackData.m_localId = 0;
  trackData.m_name[kDefaultLang] = "Test multitrack";
  trackData.m_name[kEsLang] = "Pista de prueba.";
  trackData.m_description[kDefaultLang] = "Test multitrack description";
  trackData.m_description[kEsLang] = "Descripción de prueba de la pista.";
  trackData.m_layers = {{6.0, {kml::PredefinedColor::None, 0x00ff00ff}},
                        {7.0, {kml::PredefinedColor::None, 0x0000ffff}}};
  trackData.m_timestamp = kml::TimestampClock::from_time_t(900);

  // TODO: Replace with real coordinates after Json Serialization is fixed
  trackData.m_geometry.AddLine(
      {{mercator::FromLatLon(0, 0), 1}, {mercator::FromLatLon(0, 0), 2}, {mercator::FromLatLon(0, 0), 3}});
  trackData.m_geometry.AddLine({{mercator::FromLatLon(0, 0), 1},
                                {mercator::FromLatLon(0, 0), 2},
                                {mercator::FromLatLon(0, 0), 3},
                                {mercator::FromLatLon(0, 0), 4}});

  result.m_tracksData.emplace_back(std::move(trackData));

  return result;
}

}  // namespace geojson_tests
