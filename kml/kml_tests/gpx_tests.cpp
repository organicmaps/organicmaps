#include "testing/testing.hpp"
#include "map/bookmark_helpers.hpp"
#include "kml/serdes_gpx.hpp"
#include "coding/string_utf8_multilang.hpp"
#include "geometry/mercator.hpp"
#include "platform/platform.hpp"

auto constexpr kDefaultCode = StringUtf8Multilang::kDefaultCode;

kml::FileData loadGpxFromString(std::string const & content) {
  kml::FileData dataFromText;
  try
  {
    const char * input = content.c_str();
    kml::DeserializerGpx des(dataFromText);
    MemReader const reader(input, strlen(input));
    des.Deserialize(reader);
    return dataFromText;
  }
  catch (kml::DeserializerGpx::DeserializeException const & exc)
  {
    TEST(false, ("Exception raised", exc.what()));
  }
}

kml::FileData loadGpxFromFile(std::string const & file) {
  auto const fileName = GetPlatform().TestsDataPathForFile(file);
  std::string text;
  FileReader(fileName).ReadAsString(text);
  return loadGpxFromString(text);
}

UNIT_TEST(Gpx_Test_Point)
{
  std::string const input = R"(<?xml version="1.0" encoding="UTF-8"?>
<gpx version="1.0">
 <wpt lat="42.81025" lon="-1.65727">
  <time>2022-09-05T08:39:39.3700Z</time>
  <name>Waypoint 1</name>
 </wpt>
)";

  kml::FileData const dataFromText = loadGpxFromString(input);

  kml::FileData data;
  kml::BookmarkData bookmarkData;
  bookmarkData.m_name[kDefaultCode] = "Waypoint 1";
  bookmarkData.m_point = mercator::FromLatLon(42.81025, -1.65727);
  bookmarkData.m_customName[kDefaultCode] = "Waypoint 1";
  bookmarkData.m_color = {kml::PredefinedColor::Red, 0};
  data.m_bookmarksData.emplace_back(std::move(bookmarkData));

  TEST_EQUAL(dataFromText, data, ());
}


UNIT_TEST(Gpx_Test_Route)
{
  std::string const input = R"(<?xml version="1.0" encoding="UTF-8"?>
<gpx version="1.0">
<trk>
    <name>new</name>
    <type>Cycling</type>
    <trkseg>
      <trkpt lat="54.23955053156179" lon="24.114990234375004">
          <ele>130.5</ele>
      </trkpt>
      <trkpt lat="54.32933804825253" lon="25.136718750000004">
          <ele>0.0</ele>
      </trkpt>
      <trkpt lat="54.05293900056246" lon="25.72998046875">
          <ele>0.0</ele>
      </trkpt>
    </trkseg>
</trk>
</gpx>
)";

  kml::FileData const dataFromText = loadGpxFromString(input);
  auto line = dataFromText.m_tracksData[0].m_geometry.m_lines[0];
  TEST_EQUAL(line.size(), 3, ());
  TEST_EQUAL(line[0], mercator::FromLatLon(54.23955053156179, 24.114990234375004), ());
}

UNIT_TEST(GoMap)
{
  kml::FileData const dataFromFile = loadGpxFromFile("gpx_test_data/go_map.gpx");
  auto line = dataFromFile.m_tracksData[0].m_geometry.m_lines[0];
  TEST_EQUAL(line.size(), 101, ());
}

UNIT_TEST(GpxStudio)
{
  kml::FileData const dataFromFile = loadGpxFromFile("gpx_test_data/gpx_studio.gpx");
  auto line = dataFromFile.m_tracksData[0].m_geometry.m_lines[0];
  TEST_EQUAL(line.size(), 328, ());
}

UNIT_TEST(OsmTrack)
{
  kml::FileData const dataFromFile = loadGpxFromFile("gpx_test_data/osm_track.gpx");
  auto line = dataFromFile.m_tracksData[0].m_geometry.m_lines[0];
  TEST_EQUAL(line.size(), 182, ());
}

UNIT_TEST(TowerCollector)
{
  kml::FileData const dataFromFile = loadGpxFromFile("gpx_test_data/tower_collector.gpx");
  auto line = dataFromFile.m_tracksData[0].m_geometry.m_lines[0];
  TEST_EQUAL(line.size(), 35, ());
}

UNIT_TEST(PointsOnly)
{
  kml::FileData const dataFromFile = loadGpxFromFile("gpx_test_data/points.gpx");
  auto bookmarks = dataFromFile.m_bookmarksData;
  TEST_EQUAL(bookmarks.size(), 3, ());
  TEST_EQUAL("Point 1", bookmarks[0].m_name[kDefaultCode], ());
  TEST_EQUAL(bookmarks[0].m_point, mercator::FromLatLon(48.20984622935899, 16.376023292541507), ());
}

UNIT_TEST(Route)
{
  kml::FileData dataFromFile = loadGpxFromFile("gpx_test_data/route.gpx");
  auto line = dataFromFile.m_tracksData[0].m_geometry.m_lines[0];
  TEST_EQUAL(line.size(), 2, ());
  TEST_EQUAL(dataFromFile.m_categoryData.m_name[kDefaultCode], "Some random route", ());
  TEST_EQUAL(line[0], mercator::FromLatLon(48.20984622935899, 16.376023292541507), ());
  TEST_EQUAL(line[1], mercator::FromLatLon(48.209503040543545, 16.381065845489506), ());
}

UNIT_TEST(Color)
{
  kml::FileData const dataFromFile = loadGpxFromFile("gpx_test_data/color.gpx");
  TEST_EQUAL(4278190335, dataFromFile.m_tracksData[0].m_layers[0].m_color.m_rgba, ());
  TEST_EQUAL(65535, dataFromFile.m_tracksData[1].m_layers[0].m_color.m_rgba, ());
}

UNIT_TEST(MultiTrackNames)
{
  kml::FileData dataFromFile = loadGpxFromFile("gpx_test_data/color.gpx");
  TEST_EQUAL("new", dataFromFile.m_categoryData.m_name[kml::kDefaultLang], ());
  TEST_EQUAL("Short description", dataFromFile.m_categoryData.m_description[kml::kDefaultLang], ());
  TEST_EQUAL("new red", dataFromFile.m_tracksData[0].m_name[kml::kDefaultLang], ());
  TEST_EQUAL("description 1", dataFromFile.m_tracksData[0].m_description[kml::kDefaultLang], ());
  TEST_EQUAL("new blue", dataFromFile.m_tracksData[1].m_name[kml::kDefaultLang], ());
  TEST_EQUAL("description 2", dataFromFile.m_tracksData[1].m_description[kml::kDefaultLang], ());
}

UNIT_TEST(Empty)
{
  kml::FileData dataFromFile = loadGpxFromFile("gpx_test_data/empty.gpx");
  TEST_EQUAL("new", dataFromFile.m_categoryData.m_name[kml::kDefaultLang], ());
  TEST_EQUAL(0, dataFromFile.m_tracksData.size(), ());
}


