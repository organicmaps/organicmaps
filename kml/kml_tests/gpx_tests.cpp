#include "testing/testing.hpp"
#include "map/bookmark_helpers.hpp"
#include "kml/serdes_gpx.hpp"
#include "coding/string_utf8_multilang.hpp"
#include "geometry/mercator.hpp"
#include "platform/platform.hpp"

auto const kDefaultCode = StringUtf8Multilang::kDefaultCode;

kml::FileData loadGpxFromString(const std::string& content) {
  kml::FileData dataFromText;
  try
  {
    const char * input = content.c_str();
    kml::DeserializerGpx des(dataFromText);
    MemReader reader(input, strlen(input));
    des.Deserialize(reader);
    return dataFromText;
  }
  catch (kml::DeserializerGpx::DeserializeException const & exc)
  {
    TEST(false, ("Exception raised", exc.what()));
  }
}

kml::FileData loadGpxFromFile(std::string file) {
  auto fileName = GetPlatform().TestsDataPathForFile(file);
  std::ifstream t(fileName);
  std::stringstream buffer;
  buffer << t.rdbuf();
  return loadGpxFromString(buffer.str());
}

UNIT_TEST(Gpx_Test_Point)
{
  char const * input = R"(<?xml version="1.0" encoding="UTF-8"?>
<gpx version="1.0">
 <wpt lat="42.81025" lon="-1.65727">
  <time>2022-09-05T08:39:39.3700Z</time>
  <name>Waypoint 1</name>
 </wpt>
)";

  kml::FileData dataFromText = loadGpxFromString(input);

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
  char const * input = R"(<?xml version="1.0" encoding="UTF-8"?>
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

  kml::FileData dataFromText = loadGpxFromString(input);
  auto line = dataFromText.m_tracksData[0].m_geometry.m_lines[0];
  TEST_EQUAL(line.size(), 3, ());
  TEST_EQUAL(line[0], mercator::FromLatLon(54.23955053156179, 24.114990234375004), ());
}

UNIT_TEST(GoMap)
{
  kml::FileData dataFromFile = loadGpxFromFile("gpx_test_data/go_map.gpx");
  auto line = dataFromFile.m_tracksData[0].m_geometry.m_lines[0];
  TEST_EQUAL(line.size(), 101, ());
}

UNIT_TEST(GpxStudio)
{
  kml::FileData dataFromFile = loadGpxFromFile("gpx_test_data/gpx_studio.gpx");
  auto line = dataFromFile.m_tracksData[0].m_geometry.m_lines[0];
  TEST_EQUAL(line.size(), 328, ());
}

UNIT_TEST(OsmTrack)
{
  kml::FileData dataFromFile = loadGpxFromFile("gpx_test_data/osm_track.gpx");
  auto line = dataFromFile.m_tracksData[0].m_geometry.m_lines[0];
  TEST_EQUAL(line.size(), 182, ());
}

UNIT_TEST(TowerCollector)
{
  kml::FileData dataFromFile = loadGpxFromFile("gpx_test_data/tower_collector.gpx");
  auto line = dataFromFile.m_tracksData[0].m_geometry.m_lines[0];
  TEST_EQUAL(line.size(), 35, ());
}