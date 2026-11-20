#include "testing/testing.hpp"

#include "map/bookmark_helpers.hpp"
#include "map/track.hpp"

#include "kml/serdes_geojson.hpp"
#include "kml/serdes_gpx.hpp"

#include "coding/reader.hpp"
#include "coding/writer.hpp"

#include "geometry/mercator.hpp"

#include <algorithm>
#include <array>
#include <string>
#include <string_view>
#include <vector>

namespace track_import_tests
{
namespace
{
std::string Export(kml::FileData const & data, FileType type)
{
  std::string text;
  MemWriter writer(text);
  switch (type)
  {
  case FileType::Kml: TEST(SaveKmlData(data, writer, type), ()); break;
  case FileType::Gpx: kml::gpx::SerializerGpx(data).Serialize(writer); break;
  case FileType::GeoJson: kml::GeoJsonWriter(writer).Write(data, true /* minimizeOutput */); break;
  default: UNREACHABLE();
  }
  return text;
}
}  // namespace

UNIT_TEST(TrackImport_CompactsWrappedPositionsAndKeepsMeasurementsAligned)
{
  kml::FileData source;
  auto & track = source.m_tracksData.emplace_back();
  track.m_layers.emplace_back();
  track.m_geometry.AddLine(
      {{{-180.0, 0.0}, 100}, {{179.999999, 0.0}, geometry::kInvalidAltitude}, {{179.999980, 0.0}, 200}});
  track.m_geometry.AddTimestamps({10, 11, 20});

  for (auto const type : {FileType::Kml, FileType::Gpx, FileType::GeoJson})
  {
    auto imported = LoadKmlData(MemReader(Export(source, type)), type);
    TEST(imported, (type));
    TEST_EQUAL(imported->m_tracksData.size(), 1, (type));
    auto const & geom = imported->m_tracksData.front().m_geometry;
    TEST_EQUAL(geom.m_lines.size(), 1, (type));
    auto const & line = geom.m_lines.front();
    TEST_EQUAL(line.size(), 2, (type));
    TEST_EQUAL(line.front().GetAltitude(), 100, (type));
    TEST_EQUAL(line.back().GetAltitude(), 200, (type));
    TEST_ALMOST_EQUAL_ABS(line.back().GetPoint().x, 179.999980, 1e-9, (type));
    TEST_EQUAL(geom.m_timestamps.front(), (kml::MultiGeometry::TimeT{10, 20}), (type));

    auto const reloaded = LoadKmlData(MemReader(Export(*imported, FileType::Kml)), FileType::Kml);
    TEST(reloaded, (type));
    TEST_EQUAL(reloaded->m_tracksData.size(), 1, (type));
    TEST_EQUAL(reloaded->m_tracksData.front().m_geometry, geom, (type));
  }
}

UNIT_TEST(TrackImport_ValidatesEachMeasurementAfterCompaction)
{
  struct Case
  {
    FileType m_type;
    std::string_view m_text;
    std::array<geometry::Altitude, 4> m_altitudes;
    std::vector<kml::MultiGeometry::TimeT> m_timestamps;
    double m_duration;
  };
  auto constexpr kMissing = geometry::kInvalidAltitude;
  Case const cases[] = {
      {FileType::Kml,
       R"(<kml xmlns:gx="http://www.google.com/kml/ext/2.2"><Document><Placemark><MultiGeometry>
         <gx:Track>
           <when>1970-01-01T00:00:10Z</when><when>1970-01-01T00:00:20Z</when>
           <gx:coord>19 0 0</gx:coord><gx:coord>19.001 0 100</gx:coord>
         </gx:Track>
         <LineString><coordinates>20,0,0 20.001,0,0</coordinates></LineString>
       </MultiGeometry></Placemark></Document></kml>)",
       {0, 100, 0, 0},
       {{10, 20}, {}},
       10},
      {FileType::Gpx,
       R"(<gpx><trk>
         <trkseg>
           <trkpt lat="0" lon="19"/>
           <trkpt lat="0" lon="19.000001"><ele>100</ele><time>1970-01-01T00:00:10Z</time></trkpt>
           <trkpt lat="0" lon="19.001"><ele>115</ele><time>1970-01-01T00:00:20Z</time></trkpt>
         </trkseg>
         <trkseg>
           <trkpt lat="0" lon="20"><ele>130</ele><time>1970-01-01T00:00:30Z</time></trkpt>
           <trkpt lat="0" lon="20.001"><ele>140</ele><time>1970-01-01T00:00:40Z</time></trkpt>
         </trkseg>
       </trk></gpx>)",
       {100, 115, 130, 140},
       {{10, 20}, {30, 40}},
       20},
      {FileType::GeoJson,
       R"({"type":"Feature","properties":{"coordTimes":[[10,20],[30,40]]},"geometry":{
           "type":"MultiLineString","coordinates":[[[19,0,100],[19.001,0,null]],[[20,0,130],[20.001,0,140]]]}})",
       {100, kMissing, 130, 140},
       {{10, 20}, {30, 40}},
       20},
  };
  for (auto const & [type, text, altitudes, timestamps, duration] : cases)
  {
    auto const data = LoadKmlData(MemReader(text), type);
    TEST(data, (type));
    TEST_EQUAL(data->m_tracksData.size(), 1, (type));
    auto check = [&](kml::TrackData const & track)
    {
      auto const & geom = track.m_geometry;
      TEST_EQUAL(geom.m_lines.size(), 2, (type));
      size_t pointIndex = 0;
      for (auto const & line : geom.m_lines)
      {
        TEST_EQUAL(line.size(), 2, (type));
        for (auto const & sample : line)
        {
          TEST_EQUAL(sample.GetAltitude(), altitudes[pointIndex], (type, pointIndex));
          ++pointIndex;
        }
      }

      TEST_EQUAL(geom.m_timestamps, timestamps, (type));

      Track const runtime(kml::TrackData{track});
      TEST_EQUAL(runtime.GetElevationInfo() != nullptr, std::ranges::find(altitudes, kMissing) == altitudes.end(),
                 (type));
      TEST_EQUAL(runtime.GetDurationInSeconds(), duration, (type));
    };
    check(data->m_tracksData.front());

    for (auto const outputType : {FileType::Kml, FileType::Gpx, FileType::GeoJson})
    {
      auto const reloaded = LoadKmlData(MemReader(Export(*data, outputType)), outputType);
      TEST(reloaded, (type, outputType));
      TEST_EQUAL(reloaded->m_tracksData.size(), 1, (type, outputType));
      TEST_EQUAL(reloaded->m_tracksData.front().m_geometry, data->m_tracksData.front().m_geometry, (type, outputType));
    }
  }
}

UNIT_TEST(TrackExport_UsesSixDecimalPlaces)
{
  kml::FileData source;
  source.m_bookmarksData.emplace_back().m_point = mercator::FromLatLon(-0.123456789, 1.23456789e-5);
  auto & track = source.m_tracksData.emplace_back();
  track.m_layers.emplace_back();
  kml::TrackGeometry const line = {{mercator::FromLatLon(45.12345678, 123.12345678), 100},
                                   {mercator::FromLatLon(45.12345678, 123.12445678), 110}};
  track.m_geometry.m_lines = {line, line};
  track.m_geometry.m_timestamps = {{}, {10, 20}};

  auto const kmlText = Export(source, FileType::Kml);
  TEST(kmlText.contains("<coordinates>123.123457,45.123457,100 123.124457,45.123457,110</coordinates>"), (kmlText));
  TEST(kmlText.contains("<gx:coord>123.123457 45.123457 100</gx:coord>"), (kmlText));

  auto const gpxText = Export(source, FileType::Gpx);
  TEST(gpxText.contains(R"(<wpt lat="-0.123457" lon="0.000012">)"), (gpxText));
  TEST(gpxText.contains(R"(<trkpt lat="45.123457" lon="123.123457">)"), (gpxText));
}

UNIT_TEST(TrackImport_RoundTripsAltitudeAvailability)
{
  struct Format
  {
    FileType m_type;
    bool m_timed;
    std::string_view m_missing;
    std::string_view m_zero;
  };
  Format const formats[] = {
      {FileType::Kml, false, "19.001,0 19.002,0,110", "19.001,0,0"},
      {FileType::Kml, true, "<gx:coord>19.001 0</gx:coord>", "<gx:coord>19.001 0 0</gx:coord>"},
      {FileType::Gpx, true, "lon=\"19.001\">\n      <time>", "<ele>0</ele>"},
      {FileType::GeoJson, true, "[19.001,0]", "[19.001,0,0]"},
  };
  geometry::Altitude constexpr kMissing = geometry::kInvalidAltitude;
  std::array<geometry::Altitude, 3> const profiles[] = {
      {100, kMissing, 110}, {kMissing, kMissing, kMissing}, {100, 0, 110}, {0, 0, 0}};
  for (auto const & [type, timed, missing, zero] : formats)
  {
    for (auto const & altitudes : profiles)
    {
      kml::FileData source;
      auto & track = source.m_tracksData.emplace_back();
      track.m_layers.emplace_back();
      auto & geometry = track.m_geometry;
      geometry.AddLine({{{19, 0}, altitudes[0]}, {{19.001, 0}, altitudes[1]}, {{19.002, 0}, altitudes[2]}});
      geometry.m_timestamps.push_back(timed ? kml::MultiGeometry::TimeT{100, 160, 220} : kml::MultiGeometry::TimeT{});

      auto const text = Export(source, type);
      TEST(!text.contains("-32768"), (type, text));
      if (altitudes.front() == 100 && altitudes[1] == kMissing)
        TEST(text.contains(missing), (type, text));
      if (altitudes[1] == 0)
        TEST(text.contains(zero), (type, text));

      auto const data = LoadKmlData(MemReader(text), type);
      TEST(data, (type));
      TEST_EQUAL(data->m_tracksData.size(), 1, (type));
      TEST_EQUAL(data->m_tracksData.front().m_geometry, geometry, (type));
      Track const runtime(kml::TrackData{data->m_tracksData.front()});
      TEST_EQUAL(runtime.GetElevationInfo() != nullptr, altitudes.front() == 100 && altitudes[1] == 0, (type));
      TEST_EQUAL(runtime.GetDurationInSeconds(), timed ? 120.0 : 0.0, (type));
      TEST_ALMOST_EQUAL_ABS(runtime.GetLengthMeters(), 222.4, 1.0, (type));
    }
  }
}

UNIT_TEST(TrackImport_KmlKeepsHeightsAndTimesWhenACoordinateIsSkipped)
{
  std::string_view constexpr kSkippedCoordinates[] = {"<gx:coord/>",
                                                      "<gx:coord>19.0012 0 100\n19.0013 0 200</gx:coord>"};
  for (auto const skippedCoordinate : kSkippedCoordinates)
  {
    std::string input = R"(<kml xmlns:gx="http://www.google.com/kml/ext/2.2"><Placemark><gx:Track>
      <when>1970-01-01T00:00:10Z</when><when>1970-01-01T00:00:20Z</when>
      <when>1970-01-01T00:00:30Z</when><when>1970-01-01T00:00:40Z</when>
      <gx:coord>19 0 100</gx:coord><gx:coord>19.001&#9;0</gx:coord>)";
    input += skippedCoordinate;
    input += "<gx:coord>19.002 0 0</gx:coord></gx:Track></Placemark></kml>";

    auto const data = LoadKmlData(MemReader(input), FileType::Kml);
    TEST(data, (skippedCoordinate));
    TEST_EQUAL(data->m_tracksData.size(), 1, (skippedCoordinate));
    auto const & geometry = data->m_tracksData.front().m_geometry;
    TEST_EQUAL(geometry.m_timestamps.front(), (kml::MultiGeometry::TimeT{10, 20, 40}), (skippedCoordinate));
    auto const & line = geometry.m_lines.front();
    TEST_EQUAL(line.size(), 3, (skippedCoordinate));
    TEST_EQUAL(line[0].GetAltitude(), 100, (skippedCoordinate));
    TEST_EQUAL(line[1].GetAltitude(), geometry::kInvalidAltitude, (skippedCoordinate));
    TEST_EQUAL(line[2].GetAltitude(), 0, (skippedCoordinate));
    TEST_ALMOST_EQUAL_ABS(line[1].GetPoint().x, 19.001, 1e-9, (skippedCoordinate));

    auto const reloaded = LoadKmlData(MemReader(Export(*data, FileType::Kml)), FileType::Kml);
    TEST(reloaded, (skippedCoordinate));
    TEST_EQUAL(reloaded->m_tracksData.size(), 1, (skippedCoordinate));
    TEST_EQUAL(reloaded->m_tracksData.front().m_geometry, geometry, (skippedCoordinate));
  }
}
}  // namespace track_import_tests
