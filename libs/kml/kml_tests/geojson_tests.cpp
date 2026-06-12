#include "testing/testing.hpp"

#include "kml/serdes_geojson.hpp"

#include "base/timer.hpp"
#include "indexer/classificator.hpp"

#include "geometry/mercator.hpp"
#include "geometry/point_with_altitude.hpp"

#include "coding/string_utf8_multilang.hpp"

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
  auto const bookmark = dataFromText.m_bookmarksData.front();
  auto const green = kml::ColorData{.m_predefinedColor = kml::PredefinedColor::Green};
  TEST_EQUAL(bookmark.m_color, green, ());
  TEST_EQUAL(kml::GetDefaultStr(bookmark.m_name), "Bookmark 1", ());
  TEST_EQUAL(bookmark.m_point, mercator::FromLatLon(29.8310316130992, 31.02177966625902), ());

  // Check track data.
  TEST_EQUAL(dataFromText.m_tracksData.size(), 2, ());
  auto const track1 = dataFromText.m_tracksData[0];
  TEST_EQUAL(track1.m_layers[0].m_color, kml::ColorData{.m_predefinedColor = kml::PredefinedColor::Blue}, ());
  TEST_EQUAL(track1.m_geometry.m_lines.empty(), false, ());
  TEST_EQUAL(track1.m_geometry.m_lines.front().size(), 3, ());

  // Check multiline track data.
  auto const track2 = dataFromText.m_tracksData[1];
  TEST_EQUAL(track2.m_layers[0].m_color, kml::ColorData{.m_predefinedColor = kml::PredefinedColor::Green}, ());
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
  auto const bookmark = dataFromText.m_bookmarksData.front();
  // A hex marker-color imports as an explicit custom color (no Black preset exists anyway).
  auto const customBlack = kml::ColorData{.m_predefinedColor = kml::PredefinedColor::None, .m_rgba = 0x000000FF};
  TEST_EQUAL(bookmark.m_color, customBlack, ());
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
          "color": "#1A2B3C",
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

  // The point color must be read from _umap_options.color (an explicit custom color), not from a
  // top-level "color" property which UMap points do not carry.
  TEST_EQUAL(bookmark.m_color.m_predefinedColor, kml::PredefinedColor::None, ());
  TEST_EQUAL(bookmark.m_color.m_rgba, 0x1A2B3CFFu, ());

  // Check that _umap_options is stored as JSON string
  auto const umapOptionsIt = bookmark.m_properties.find("_umap_options");
  TEST(umapOptionsIt != bookmark.m_properties.end(), ("_umap_options should be stored in properties"));

  // Parse the stored JSON to verify it contains the expected data
  std::string const & umapOptionsStr = umapOptionsIt->second;
  glz::generic umapOptionsJson;
  auto const parseResult = glz::read_json(umapOptionsJson, umapOptionsStr);
  TEST(parseResult == glz::error_code::none, ("Should be able to parse stored _umap_options JSON"));

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
  glz::generic trackUmapOptionsJson;
  auto const trackParseResult = glz::read_json(trackUmapOptionsJson, trackUmapOptionsStr);
  TEST(trackParseResult == glz::error_code::none, ("Should be able to parse stored track _umap_options JSON"));

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

  // Check bookmark with coodinates and Google link
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
          13.39712,
          52.48982
        ]
      },
      "properties": {
        "name": "Marcador de prueba",
        "marker-color": "blue",
        "description": "Test bookmark description"
      }
    },
    {
      "type": "Feature",
      "geometry": {
        "type": "LineString",
        "coordinates": [
          [
            45.25,
            47,
            1
          ],
          [
            45.5,
            48,
            2
          ],
          [
            45.75,
            49,
            3
          ]
        ]
      },
      "properties": {
        "name": "Test track",
        "stroke": "#FF0000",
        "description": "Test track description"
      }
    },
    {
      "type": "Feature",
      "geometry": {
        "type": "LineString",
        "coordinates": [
          [
            30.1,
            22,
            1
          ],
          [
            30.2,
            23,
            2
          ],
          [
            30.3,
            24,
            3
          ]
        ]
      },
      "properties": {
        "name": "Another track",
        "stroke": "#93BF39"
      }
    }
  ]
})";

  kml::FileData const testData = GenerateKmlFileDataWithTrack();
  auto const jsonString = SaveToGeoJsonString(testData);

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
              45.25,
              47,
              1
            ],
            [
              45.5,
              48,
              2
            ],
            [
              45.75,
              49,
              3
            ]
          ],
          [
            [
              45.95,
              49.2,
              1
            ],
            [
              46.15,
              49.4,
              2
            ],
            [
              46.4,
              49.6,
              3
            ],
            [
              46.55,
              49.8,
              4
            ]
          ]
        ]
      },
      "properties": {
        "name": "Test multitrack",
        "stroke": "#00FF00",
        "description": "Test multitrack description"
      }
    }
  ]
})";

  kml::FileData testData = GenerateKmlFileDataWithMultiTrack();
  testData.m_bookmarksData.clear();
  auto const jsonString = SaveToGeoJsonString(testData);

  TEST_EQUAL(jsonString, expected_geojson, ());
}

UNIT_TEST(GeoJson_Writer_Simple_Minimized)
{
  // clang-format off
  std::string_view constexpr expected_geojson = R"({"type":"FeatureCollection","features":[{"type":"Feature","geometry":{"type":"Point","coordinates":[13.39712,52.48982]},"properties":{"name":"Marcador de prueba","marker-color":"blue","description":"Test bookmark description"}}]})";
  // clang-format on

  kml::FileData const testData = GenerateKmlFileData();
  auto const jsonString = SaveToGeoJsonString(testData, true);

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
          13.39712,
          52.48982
        ]
      },
      "properties": {
        "name": "Marcador de prueba",
        "marker-color": "blue",
        "description": "Test bookmark description",
        "_umap_options": {
          "iconClass": "Drop",
          "color": "blue",
          "weight": 4,
          "opacity": 0.8,
          "customProperty": "should be preserved"
        }
      }
    },
    {
      "type": "Feature",
      "geometry": {
        "type": "LineString",
        "coordinates": [
          [
            45.25,
            47,
            1
          ],
          [
            45.5,
            48,
            2
          ],
          [
            45.75,
            49,
            3
          ]
        ]
      },
      "properties": {
        "name": "Test track",
        "stroke": "#FF0000",
        "description": "Test track description",
        "_umap_options": {
          "color": "#FF0000",
          "weight": 2,
          "opacity": 0.5,
          "dashArray": "5,10"
        }
      }
    },
    {
      "type": "Feature",
      "geometry": {
        "type": "LineString",
        "coordinates": [
          [
            30.1,
            22,
            1
          ],
          [
            30.2,
            23,
            2
          ],
          [
            30.3,
            24,
            3
          ]
        ]
      },
      "properties": {
        "name": "Another track",
        "stroke": "#93BF39",
        "_umap_options": {
          "color": "#93BF39",
          "weight": 2,
          "opacity": 0.5,
          "dashArray": "5,10"
        }
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
  testData.m_tracksData[1].m_properties["_umap_options"] = track_umap_properties_str;

  auto const jsonString = SaveToGeoJsonString(testData);

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
          13.39712,
          52.48982
        ]
      },
      "properties": {
        "name": "Marcador de prueba",
        "marker-color": "blue",
        "description": "Test bookmark description"
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
  auto const jsonString = SaveToGeoJsonString(testData);

  TEST_EQUAL(jsonString, expected_geojson, ());
}

UNIT_TEST(GeoJson_Parse_FeatureCollection_NonStringProperties)
{
  // FeatureCollection-level "properties" may contain non-string values
  // (numbers, booleans, nested objects). This should parse without errors.
  std::string_view constexpr input = R"({
  "type": "FeatureCollection",
  "properties": {
    "name": "My Collection",
    "count": 42,
    "active": true,
    "rating": 3.14,
    "metadata": {
      "source": "test",
      "version": 2
    }
  },
  "features": [
    {
      "type": "Feature",
      "properties": {
        "name": "Test Point"
      },
      "geometry": {
        "coordinates": [13.39712, 52.48982],
        "type": "Point"
      }
    }
  ]
})";

  kml::FileData const dataFromText = LoadGeojsonFromString(input);

  TEST_EQUAL(dataFromText.m_bookmarksData.size(), 1, ());
  TEST_EQUAL(kml::GetDefaultStr(dataFromText.m_bookmarksData.front().m_name), "Test Point", ());
}

UNIT_TEST(GeoJson_Parse_FeatureCollection_NoProperties)
{
  // FeatureCollection without "properties" field should also parse fine.
  std::string_view constexpr input = R"({
  "type": "FeatureCollection",
  "features": [
    {
      "type": "Feature",
      "properties": {
        "name": "No Collection Props"
      },
      "geometry": {
        "coordinates": [2.3522, 48.8566],
        "type": "Point"
      }
    }
  ]
})";

  kml::FileData const dataFromText = LoadGeojsonFromString(input);

  TEST_EQUAL(dataFromText.m_bookmarksData.size(), 1, ());
  TEST_EQUAL(kml::GetDefaultStr(dataFromText.m_bookmarksData.front().m_name), "No Collection Props", ());
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

  auto const & cl = classif();
  bookmarkData.m_featureTypes = {cl.GetTypeByPath({"historic", "castle"}), cl.GetTypeByPath({"historic", "memorial"})};

  bookmarkData.m_customName[kDefaultLang] = "Mi lugar favorito";
  bookmarkData.m_customName[kEnLang] = "My favorite place";
  bookmarkData.m_color = {kml::PredefinedColor::Blue, 0};
  bookmarkData.m_icon = kml::BookmarkIcon::None;
  bookmarkData.m_timestamp = kml::TimestampClock::from_time_t(800);
  bookmarkData.m_point = mercator::FromLatLon(52.48982, 13.39712);
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

  trackData.m_geometry.AddLine({{mercator::FromLatLon(47, 45.25), 1},
                                {mercator::FromLatLon(48, 45.5), 2},
                                {mercator::FromLatLon(49, 45.75), 3}});

  trackData.m_properties = {{"tr_property1", "value1"}, {"tr_property2", "value2"}};
  result.m_tracksData.emplace_back(std::move(trackData));

  kml::TrackData trackData2;
  trackData2.m_localId = 1;
  trackData2.m_name[kDefaultLang] = "Another track";
  trackData2.m_layers = {{6.0, {kml::PredefinedColor::None, 0x93bf39ff}}};
  trackData2.m_timestamp = kml::TimestampClock::from_time_t(960);

  trackData2.m_geometry.AddLine(
      {{mercator::FromLatLon(22, 30.1), 1}, {mercator::FromLatLon(23, 30.2), 2}, {mercator::FromLatLon(24, 30.3), 3}});

  result.m_tracksData.emplace_back(std::move(trackData2));

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

  trackData.m_geometry.AddLine({{mercator::FromLatLon(47, 45.25), 1},
                                {mercator::FromLatLon(48, 45.5), 2},
                                {mercator::FromLatLon(49, 45.75), 3}});
  trackData.m_geometry.AddLine({{mercator::FromLatLon(49.2, 45.95), 1},
                                {mercator::FromLatLon(49.4, 46.15), 2},
                                {mercator::FromLatLon(49.6, 46.40), 3},
                                {mercator::FromLatLon(49.8, 46.55), 4}});

  result.m_tracksData.emplace_back(std::move(trackData));

  return result;
}

UNIT_TEST(GeoJson_Parse_Elevation)
{
  // Mixed 2D/3D: the 2D point gets kDefaultAltitudeMeters (0), not kInvalidAltitude,
  // because other points in the same line carry Z — preserving the elevation chart.
  std::string_view constexpr inputMixed = R"({
  "type": "FeatureCollection",
  "features": [
    {
      "type": "Feature",
      "properties": {"name": "Track with Z"},
      "geometry": {
        "type": "LineString",
        "coordinates": [
          [8.5417, 47.3769, 410],
          [8.5418, 47.3770, 411],
          [8.5419, 47.3771]
        ]
      }
    }
  ]
})";

  kml::FileData const mixed = LoadGeojsonFromString(inputMixed);
  TEST_EQUAL(mixed.m_tracksData.size(), 1, ());
  auto const & line = mixed.m_tracksData[0].m_geometry.m_lines[0];
  TEST_EQUAL(line.size(), 3, ());
  TEST_EQUAL(line[0].GetAltitude(), 410, ());
  TEST_EQUAL(line[1].GetAltitude(), 411, ());
  // 2D point in a mixed line gets kDefaultAltitudeMeters (sea level) to preserve
  // the elevation chart — it does NOT get kInvalidAltitude.
  TEST_EQUAL(line[2].GetAltitude(), geometry::kDefaultAltitudeMeters, ());

  // All-2D line: every point gets kInvalidAltitude (no elevation data at all).
  std::string_view constexpr inputAllFlat = R"({
  "type": "FeatureCollection",
  "features": [
    {
      "type": "Feature",
      "properties": {},
      "geometry": {
        "type": "LineString",
        "coordinates": [[8.0, 47.0], [8.1, 47.1]]
      }
    }
  ]
})";

  kml::FileData const flat = LoadGeojsonFromString(inputAllFlat);
  TEST_EQUAL(flat.m_tracksData.size(), 1, ());
  for (auto const & pt : flat.m_tracksData[0].m_geometry.m_lines[0])
    TEST_EQUAL(pt.GetAltitude(), geometry::kInvalidAltitude, ());
}

UNIT_TEST(GeoJson_Writer_Elevation_Mixed)
{
  // A track where some points have kInvalidAltitude — Z should still be emitted
  // with 0 used in place of kInvalidAltitude, so all three coordinates are 3-element.
  kml::FileData fileData;
  kml::TrackData track;
  track.m_layers = {{5.0, {kml::PredefinedColor::Red, 0}}};
  track.m_geometry.AddLine({{mercator::FromLatLon(47.0, 8.0), 100},
                            {mercator::FromLatLon(47.1, 8.1), geometry::kInvalidAltitude},
                            {mercator::FromLatLon(47.2, 8.2), 200}});
  track.m_geometry.AddTimestamps({});
  fileData.m_tracksData.emplace_back(std::move(track));

  // Parse the output and verify each coordinate has 3 elements with the right Z value.
  auto const json = SaveToGeoJsonString(fileData, true);
  kml::FileData const parsed = LoadGeojsonFromString(json);
  TEST_EQUAL(parsed.m_tracksData.size(), 1, ());
  auto const & line = parsed.m_tracksData[0].m_geometry.m_lines[0];
  TEST_EQUAL(line.size(), 3, ());
  TEST_EQUAL(line[0].GetAltitude(), 100, ());
  TEST_EQUAL(line[1].GetAltitude(), 0, ());  // kInvalidAltitude fallback → 0
  TEST_EQUAL(line[2].GetAltitude(), 200, ());
}

UNIT_TEST(GeoJson_Writer_Elevation_AllInvalid)
{
  // When all altitudes are kInvalidAltitude, Z must not be emitted.
  kml::FileData fileData;
  kml::TrackData track;
  track.m_layers = {{5.0, {kml::PredefinedColor::Red, 0}}};
  track.m_geometry.AddLine({{mercator::FromLatLon(47.0, 8.0), geometry::kInvalidAltitude},
                            {mercator::FromLatLon(47.1, 8.1), geometry::kInvalidAltitude}});
  track.m_geometry.AddTimestamps({});
  fileData.m_tracksData.emplace_back(std::move(track));

  // Round-trip: parse exported JSON back and verify no Z values came through.
  kml::FileData parsed = LoadGeojsonFromString(SaveToGeoJsonString(fileData, true));
  TEST_EQUAL(parsed.m_tracksData.size(), 1, ());
  auto const & line = parsed.m_tracksData[0].m_geometry.m_lines[0];
  for (auto const & pt : line)
    TEST_EQUAL(pt.GetAltitude(), geometry::kInvalidAltitude, ());
}

// --- Timestamp tests ---

UNIT_TEST(GeoJson_Parse_Timestamps)
{
  std::string_view constexpr input = R"({
  "type": "FeatureCollection",
  "features": [
    {
      "type": "Feature",
      "properties": {
        "name": "Recorded track",
        "time": "2026-05-31T12:34:56Z",
        "coordTimes": [
          "2026-05-31T12:34:56Z",
          "2026-05-31T12:35:01Z",
          "2026-05-31T12:35:06Z"
        ]
      },
      "geometry": {
        "type": "LineString",
        "coordinates": [
          [8.5417, 47.3769, 410],
          [8.5418, 47.3770, 411],
          [8.5419, 47.3771, 412]
        ]
      }
    }
  ]
})";

  kml::FileData const data = LoadGeojsonFromString(input);
  TEST_EQUAL(data.m_tracksData.size(), 1, ());
  auto const & track = data.m_tracksData[0];
  TEST(track.m_geometry.HasTimestamps(), ());
  TEST(track.m_geometry.HasTimestampsFor(0), ());
  auto const & timestamps = track.m_geometry.m_timestamps[0];
  TEST_EQUAL(timestamps.size(), 3, ());
  TEST_EQUAL(timestamps[0], base::StringToTimestamp("2026-05-31T12:34:56Z"), ());
  TEST_EQUAL(timestamps[1], base::StringToTimestamp("2026-05-31T12:35:01Z"), ());
  TEST_EQUAL(timestamps[2], base::StringToTimestamp("2026-05-31T12:35:06Z"), ());
}

UNIT_TEST(GeoJson_Parse_Timestamps_Formats)
{
  // Verify all required timestamp formats parse correctly. Kept in monotonic non-decreasing
  // order so they survive import sanitation (see GeoJson_Parse_Timestamps_NonMonotonic).
  std::string_view constexpr input = R"({
  "type": "FeatureCollection",
  "features": [
    {
      "type": "Feature",
      "properties": {
        "coordTimes": [
          "2017-01-01T10:00:00-0700",
          "2017-06-02T00:00:00",
          "2022-10-10T21:21:49.168Z",
          1748691296,
          1748691297.5
        ]
      },
      "geometry": {
        "type": "LineString",
        "coordinates": [
          [8.0, 47.0], [8.1, 47.1], [8.2, 47.2], [8.3, 47.3], [8.4, 47.4]
        ]
      }
    }
  ]
})";

  kml::FileData const data = LoadGeojsonFromString(input);
  TEST_EQUAL(data.m_tracksData.size(), 1, ());
  auto const & timestamps = data.m_tracksData[0].m_geometry.m_timestamps[0];
  TEST_EQUAL(timestamps.size(), 5, ());
  TEST_EQUAL(timestamps[0], base::StringToTimestamp("2017-01-01T10:00:00-07:00"), ());
  TEST_EQUAL(timestamps[1], base::StringToTimestamp("2017-06-02T00:00:00"), ());
  TEST_EQUAL(timestamps[2], base::StringToTimestamp("2022-10-10T21:21:49Z"), ());  // ms dropped
  TEST_EQUAL(timestamps[3], static_cast<time_t>(1748691296), ());
  TEST_EQUAL(timestamps[4], static_cast<time_t>(1748691297), ());  // float truncated
}

UNIT_TEST(GeoJson_Parse_Timestamps_AltSources)
{
  // Falls back to "times" then "coordinateProperties.times" when "coordTimes" absent.
  time_t const kT0 = base::StringToTimestamp("2020-01-01T00:00:00Z");
  time_t const kT1 = base::StringToTimestamp("2020-01-01T00:00:01Z");

  auto const testSource = [&](std::string_view src)
  {
    std::string input = R"({"type":"FeatureCollection","features":[{"type":"Feature","properties":{)";
    input += std::string(src);
    input += R"(},"geometry":{"type":"LineString","coordinates":[[8.0,47.0],[8.1,47.1]]}}]})";
    kml::FileData data = LoadGeojsonFromString(input);
    TEST_EQUAL(data.m_tracksData.size(), 1, ());
    TEST(data.m_tracksData[0].m_geometry.HasTimestamps(), ());
    auto const & ts = data.m_tracksData[0].m_geometry.m_timestamps[0];
    TEST_EQUAL(ts.size(), 2, ());
    TEST_EQUAL(ts[0], kT0, ());
    TEST_EQUAL(ts[1], kT1, ());
  };

  testSource(R"("times":["2020-01-01T00:00:00Z","2020-01-01T00:00:01Z"])");
  testSource(R"("coordinateProperties":{"times":["2020-01-01T00:00:00Z","2020-01-01T00:00:01Z"]})");
}

UNIT_TEST(GeoJson_Parse_Timestamps_CountMismatch)
{
  // When coordTimes count != coordinate count, timestamps must be discarded.
  std::string_view constexpr input = R"({
  "type": "FeatureCollection",
  "features": [
    {
      "type": "Feature",
      "properties": {
        "coordTimes": ["2026-05-31T12:34:56Z", "2026-05-31T12:35:01Z"]
      },
      "geometry": {
        "type": "LineString",
        "coordinates": [[8.0, 47.0], [8.1, 47.1], [8.2, 47.2]]
      }
    }
  ]
})";

  kml::FileData const data = LoadGeojsonFromString(input);
  TEST_EQUAL(data.m_tracksData.size(), 1, ());
  TEST(!data.m_tracksData[0].m_geometry.HasTimestamps(), ());
}

UNIT_TEST(GeoJson_Parse_Timestamps_MultiLine)
{
  std::string_view constexpr input = R"({
  "type": "FeatureCollection",
  "features": [
    {
      "type": "Feature",
      "properties": {
        "coordTimes": [
          ["2026-05-31T12:34:56Z", "2026-05-31T12:35:01Z"],
          ["2026-05-31T12:40:00Z", "2026-05-31T12:40:05Z", "2026-05-31T12:40:10Z"]
        ]
      },
      "geometry": {
        "type": "MultiLineString",
        "coordinates": [
          [[8.0, 47.0], [8.1, 47.1]],
          [[8.2, 47.2], [8.3, 47.3], [8.4, 47.4]]
        ]
      }
    }
  ]
})";

  kml::FileData const data = LoadGeojsonFromString(input);
  TEST_EQUAL(data.m_tracksData.size(), 1, ());
  auto const & geom = data.m_tracksData[0].m_geometry;
  TEST_EQUAL(geom.m_lines.size(), 2, ());
  TEST(geom.HasTimestampsFor(0), ());
  TEST(geom.HasTimestampsFor(1), ());
  TEST_EQUAL(geom.m_timestamps[0].size(), 2, ());
  TEST_EQUAL(geom.m_timestamps[1].size(), 3, ());
  TEST_EQUAL(geom.m_timestamps[0][0], base::StringToTimestamp("2026-05-31T12:34:56Z"), ());
  TEST_EQUAL(geom.m_timestamps[1][2], base::StringToTimestamp("2026-05-31T12:40:10Z"), ());
}

UNIT_TEST(GeoJson_Writer_Timestamps)
{
  std::string_view constexpr expected = R"({
  "type": "FeatureCollection",
  "features": [
    {
      "type": "Feature",
      "geometry": {
        "type": "LineString",
        "coordinates": [
          [
            8.5417,
            47.3769,
            410
          ],
          [
            8.5418,
            47.377,
            411
          ],
          [
            8.5419,
            47.3771,
            412
          ]
        ]
      },
      "properties": {
        "name": "Recorded track",
        "stroke": "red",
        "time": "2026-05-31T12:34:56Z",
        "coordTimes": [
          "2026-05-31T12:34:56Z",
          "2026-05-31T12:35:01Z",
          "2026-05-31T12:35:06Z"
        ]
      }
    }
  ]
})";

  kml::FileData fileData;
  kml::TrackData track;
  kml::SetDefaultStr(track.m_name, "Recorded track");
  track.m_layers = {{5.0, {kml::PredefinedColor::Red, 0}}};
  track.m_geometry.AddLine({{mercator::FromLatLon(47.3769, 8.5417), 410},
                            {mercator::FromLatLon(47.3770, 8.5418), 411},
                            {mercator::FromLatLon(47.3771, 8.5419), 412}});
  track.m_geometry.AddTimestamps({base::StringToTimestamp("2026-05-31T12:34:56Z"),
                                  base::StringToTimestamp("2026-05-31T12:35:01Z"),
                                  base::StringToTimestamp("2026-05-31T12:35:06Z")});
  fileData.m_tracksData.emplace_back(std::move(track));

  TEST_EQUAL(SaveToGeoJsonString(fileData), expected, ());
}

UNIT_TEST(GeoJson_Writer_Timestamps_MultiLine)
{
  std::string_view constexpr expected = R"({
  "type": "FeatureCollection",
  "features": [
    {
      "type": "Feature",
      "geometry": {
        "type": "MultiLineString",
        "coordinates": [
          [
            [
              8.5417,
              47.3769,
              410
            ],
            [
              8.5418,
              47.377,
              411
            ]
          ],
          [
            [
              8.5419,
              47.3771,
              412
            ],
            [
              8.542,
              47.3772,
              413
            ]
          ]
        ]
      },
      "properties": {
        "name": "Multi-segment track",
        "stroke": "red",
        "time": "2026-05-31T12:34:56Z",
        "coordTimes": [
          [
            "2026-05-31T12:34:56Z",
            "2026-05-31T12:35:01Z"
          ],
          [
            "2026-05-31T12:40:00Z",
            "2026-05-31T12:40:05Z"
          ]
        ]
      }
    }
  ]
})";

  kml::FileData fileData;
  kml::TrackData track;
  kml::SetDefaultStr(track.m_name, "Multi-segment track");
  track.m_layers = {{5.0, {kml::PredefinedColor::Red, 0}}};
  track.m_geometry.AddLine(
      {{mercator::FromLatLon(47.3769, 8.5417), 410}, {mercator::FromLatLon(47.3770, 8.5418), 411}});
  track.m_geometry.AddTimestamps(
      {base::StringToTimestamp("2026-05-31T12:34:56Z"), base::StringToTimestamp("2026-05-31T12:35:01Z")});
  track.m_geometry.AddLine(
      {{mercator::FromLatLon(47.3771, 8.5419), 412}, {mercator::FromLatLon(47.3772, 8.5420), 413}});
  track.m_geometry.AddTimestamps(
      {base::StringToTimestamp("2026-05-31T12:40:00Z"), base::StringToTimestamp("2026-05-31T12:40:05Z")});
  fileData.m_tracksData.emplace_back(std::move(track));

  TEST_EQUAL(SaveToGeoJsonString(fileData), expected, ());
}

UNIT_TEST(GeoJson_RoundTrip_Timestamps)
{
  // Import a track with elevation + timestamps, export, re-import — must be lossless.
  std::string_view constexpr input = R"({
  "type": "FeatureCollection",
  "features": [
    {
      "type": "Feature",
      "properties": {
        "name": "Round-trip track",
        "stroke": "red",
        "coordTimes": ["2026-05-31T12:34:56Z", "2026-05-31T12:35:01Z", "2026-05-31T12:35:06Z"]
      },
      "geometry": {
        "type": "LineString",
        "coordinates": [[8.5417, 47.3769, 410], [8.5418, 47.377, 411], [8.5419, 47.3771, 412]]
      }
    }
  ]
})";

  kml::FileData const first = LoadGeojsonFromString(input);
  auto const json = SaveToGeoJsonString(first);
  kml::FileData const second = LoadGeojsonFromString(json);

  TEST_EQUAL(second.m_tracksData.size(), 1, ());
  auto const & geom1 = first.m_tracksData[0].m_geometry;
  auto const & geom2 = second.m_tracksData[0].m_geometry;
  TEST_EQUAL(geom1.m_lines[0].size(), geom2.m_lines[0].size(), ());
  for (size_t i = 0; i < geom1.m_lines[0].size(); ++i)
    TEST_EQUAL(geom1.m_lines[0][i].GetAltitude(), geom2.m_lines[0][i].GetAltitude(), ());
  TEST_EQUAL(geom1.m_timestamps[0], geom2.m_timestamps[0], ());
}

UNIT_TEST(GeoJson_Parse_Timestamps_MultiLine_SkipsEmptySegments)
{
  // lineIdx must track position in the coordTimes array by geometry index,
  // not by the count of non-empty lines pushed. If the first geometry segment
  // is empty, the second segment's timestamps must come from coordTimes[1].
  std::string_view constexpr input = R"({
  "type": "FeatureCollection",
  "features": [
    {
      "type": "Feature",
      "properties": {
        "coordTimes": [
          [],
          ["2026-05-31T12:40:00Z", "2026-05-31T12:40:05Z"]
        ]
      },
      "geometry": {
        "type": "MultiLineString",
        "coordinates": [
          [],
          [[8.2, 47.2], [8.3, 47.3]]
        ]
      }
    }
  ]
})";

  kml::FileData const data = LoadGeojsonFromString(input);
  TEST_EQUAL(data.m_tracksData.size(), 1, ());
  auto const & geom = data.m_tracksData[0].m_geometry;
  TEST_EQUAL(geom.m_lines.size(), 1, ());  // empty segment skipped
  TEST(geom.HasTimestampsFor(0), ());
  TEST_EQUAL(geom.m_timestamps[0].size(), 2, ());
  TEST_EQUAL(geom.m_timestamps[0][0], base::StringToTimestamp("2026-05-31T12:40:00Z"), ());
  TEST_EQUAL(geom.m_timestamps[0][1], base::StringToTimestamp("2026-05-31T12:40:05Z"), ());
}

UNIT_TEST(GeoJson_Writer_Timestamps_InvalidTimestamp)
{
  // INVALID_TIME_STAMP entries must be emitted as JSON null, not "INVALID_TIME_STAMP".
  // The "time" property must be skipped when the first timestamp is invalid.
  kml::FileData fileData;
  kml::TrackData track;
  track.m_layers = {{5.0, {kml::PredefinedColor::Red, 0}}};
  track.m_geometry.AddLine({{mercator::FromLatLon(47.0, 8.0), geometry::kInvalidAltitude},
                            {mercator::FromLatLon(47.1, 8.1), geometry::kInvalidAltitude},
                            {mercator::FromLatLon(47.2, 8.2), geometry::kInvalidAltitude}});
  track.m_geometry.AddTimestamps(
      {base::INVALID_TIME_STAMP, base::StringToTimestamp("2026-05-31T12:35:01Z"), base::INVALID_TIME_STAMP});
  fileData.m_tracksData.emplace_back(std::move(track));

  auto const json = SaveToGeoJsonString(fileData, true);
  TEST(json.find("INVALID_TIME_STAMP") == std::string::npos, ());
  TEST(json.find("null") != std::string::npos, ());
  // "time" property must not appear since first timestamp is invalid.
  TEST(json.find("\"time\"") == std::string::npos, ());
  // The valid middle timestamp must still appear in coordTimes.
  TEST(json.find("2026-05-31T12:35:01Z") != std::string::npos, ());
}

UNIT_TEST(GeoJson_Writer_Timestamps_MultiLine_InvalidTimestamp)
{
  // Multiline: first valid timestamp is on line 1, not line 0.
  // "time" must be set to line 1's first valid timestamp via find_if.
  kml::FileData fileData;
  kml::TrackData track;
  track.m_layers = {{5.0, {kml::PredefinedColor::Red, 0}}};
  track.m_geometry.AddLine({{mercator::FromLatLon(47.0, 8.0), geometry::kInvalidAltitude},
                            {mercator::FromLatLon(47.1, 8.1), geometry::kInvalidAltitude}});
  track.m_geometry.AddTimestamps({base::INVALID_TIME_STAMP, base::INVALID_TIME_STAMP});
  track.m_geometry.AddLine({{mercator::FromLatLon(47.2, 8.2), geometry::kInvalidAltitude},
                            {mercator::FromLatLon(47.3, 8.3), geometry::kInvalidAltitude}});
  track.m_geometry.AddTimestamps(
      {base::StringToTimestamp("2026-05-31T13:00:00Z"), base::StringToTimestamp("2026-05-31T13:00:05Z")});
  fileData.m_tracksData.emplace_back(std::move(track));

  auto const json = SaveToGeoJsonString(fileData, true);
  TEST(json.find("INVALID_TIME_STAMP") == std::string::npos, ());
  // "time" comes from line 1's first valid timestamp.
  TEST(json.find("\"time\"") != std::string::npos, ());
  TEST(json.find("2026-05-31T13:00:00Z") != std::string::npos, ());
}

UNIT_TEST(GeoJson_RoundTrip_Timestamps_MultiLine)
{
  // Full round-trip for a MultiLineString with timestamps on both lines.
  std::string_view constexpr input = R"({
  "type": "FeatureCollection",
  "features": [
    {
      "type": "Feature",
      "properties": {
        "stroke": "blue",
        "coordTimes": [
          ["2026-05-31T10:00:00Z", "2026-05-31T10:00:05Z"],
          ["2026-05-31T11:00:00Z", "2026-05-31T11:00:10Z", "2026-05-31T11:00:20Z"]
        ]
      },
      "geometry": {
        "type": "MultiLineString",
        "coordinates": [
          [[8.0, 47.0, 400], [8.1, 47.1, 401]],
          [[8.2, 47.2, 500], [8.3, 47.3, 501], [8.4, 47.4, 502]]
        ]
      }
    }
  ]
})";

  kml::FileData const first = LoadGeojsonFromString(input);
  kml::FileData const second = LoadGeojsonFromString(SaveToGeoJsonString(first));

  TEST_EQUAL(second.m_tracksData.size(), 1, ());
  auto const & geom1 = first.m_tracksData[0].m_geometry;
  auto const & geom2 = second.m_tracksData[0].m_geometry;
  TEST_EQUAL(geom2.m_lines.size(), 2, ());
  TEST_EQUAL(geom2.m_lines[0].size(), 2, ());
  TEST_EQUAL(geom2.m_lines[1].size(), 3, ());
  TEST_EQUAL(geom2.m_lines[0][0].GetAltitude(), 400, ());
  TEST_EQUAL(geom2.m_lines[1][2].GetAltitude(), 502, ());
  TEST_EQUAL(geom1.m_timestamps[0], geom2.m_timestamps[0], ());
  TEST_EQUAL(geom1.m_timestamps[1], geom2.m_timestamps[1], ());
}

UNIT_TEST(GeoJson_Parse_Timestamps_NonMonotonic)
{
  // Non-monotonic coordTimes are dropped on import (downstream duration math assumes sorted),
  // mirroring KML import. The geometry is preserved.
  std::string_view constexpr input = R"({
  "type": "FeatureCollection",
  "features": [
    {
      "type": "Feature",
      "properties": {
        "coordTimes": ["2026-05-31T12:35:06Z", "2026-05-31T12:34:56Z", "2026-05-31T12:35:01Z"]
      },
      "geometry": {
        "type": "LineString",
        "coordinates": [[8.0, 47.0], [8.1, 47.1], [8.2, 47.2]]
      }
    }
  ]
})";

  kml::FileData const data = LoadGeojsonFromString(input);
  TEST_EQUAL(data.m_tracksData.size(), 1, ());
  TEST_EQUAL(data.m_tracksData[0].m_geometry.m_lines[0].size(), 3, ());  // geometry kept
  TEST(!data.m_tracksData[0].m_geometry.HasTimestamps(), ());            // timestamps dropped
}

UNIT_TEST(GeoJson_Parse_Timestamps_InvalidEntry)
{
  // coordTimes count matches the coordinates, but one entry is null: the whole line's
  // timestamps are dropped so no INVALID_TIME_STAMP reaches downstream writers/stats.
  std::string_view constexpr input = R"({
  "type": "FeatureCollection",
  "features": [
    {
      "type": "Feature",
      "properties": {
        "coordTimes": ["2026-05-31T12:34:56Z", null, "2026-05-31T12:35:06Z"]
      },
      "geometry": {
        "type": "LineString",
        "coordinates": [[8.0, 47.0], [8.1, 47.1], [8.2, 47.2]]
      }
    }
  ]
})";

  kml::FileData const data = LoadGeojsonFromString(input);
  TEST_EQUAL(data.m_tracksData.size(), 1, ());
  TEST_EQUAL(data.m_tracksData[0].m_geometry.m_lines[0].size(), 3, ());
  TEST(!data.m_tracksData[0].m_geometry.HasTimestamps(), ());
}

UNIT_TEST(GeoJson_Parse_Timestamps_MultiLine_PartialSanitized)
{
  // One line has valid sorted timestamps, the other is non-monotonic: only the bad line's
  // timestamps are dropped, leaving a track with timestamps on some lines only.
  std::string_view constexpr input = R"({
  "type": "FeatureCollection",
  "features": [
    {
      "type": "Feature",
      "properties": {
        "coordTimes": [
          ["2026-05-31T12:00:00Z", "2026-05-31T12:00:05Z"],
          ["2026-05-31T12:10:10Z", "2026-05-31T12:10:00Z"]
        ]
      },
      "geometry": {
        "type": "MultiLineString",
        "coordinates": [
          [[8.0, 47.0], [8.1, 47.1]],
          [[8.2, 47.2], [8.3, 47.3]]
        ]
      }
    }
  ]
})";

  kml::FileData const data = LoadGeojsonFromString(input);
  TEST_EQUAL(data.m_tracksData.size(), 1, ());
  auto const & geom = data.m_tracksData[0].m_geometry;
  TEST_EQUAL(geom.m_lines.size(), 2, ());
  TEST(geom.HasTimestampsFor(0), ());   // first line kept
  TEST(!geom.HasTimestampsFor(1), ());  // second line (non-monotonic) dropped
  TEST_EQUAL(geom.m_timestamps[0].size(), 2, ());
}

UNIT_TEST(GeoJson_Writer_Elevation_AllDefault)
{
  // A track whose points all carry the default sea-level altitude (0) has no real elevation,
  // so the writer must NOT emit a Z coordinate (otherwise every flat track would be bloated
  // with z=0). Proven by round-trip: a 2D export re-imports as kInvalidAltitude.
  kml::FileData fileData;
  kml::TrackData track;
  track.m_layers = {{5.0, {kml::PredefinedColor::Red, 0}}};
  track.m_geometry.AddLine({{mercator::FromLatLon(47.0, 8.0), geometry::kDefaultAltitudeMeters},
                            {mercator::FromLatLon(47.1, 8.1), geometry::kDefaultAltitudeMeters}});
  track.m_geometry.AddTimestamps({});
  fileData.m_tracksData.emplace_back(std::move(track));

  kml::FileData const parsed = LoadGeojsonFromString(SaveToGeoJsonString(fileData, true));
  TEST_EQUAL(parsed.m_tracksData.size(), 1, ());
  for (auto const & pt : parsed.m_tracksData[0].m_geometry.m_lines[0])
    TEST_EQUAL(pt.GetAltitude(), geometry::kInvalidAltitude, ());
}

}  // namespace geojson_tests
