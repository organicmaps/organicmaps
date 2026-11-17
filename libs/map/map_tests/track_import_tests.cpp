#include "testing/testing.hpp"

#include "kml/serdes_geojson.hpp"
#include "kml/serdes_gpx.hpp"
#include "map/bookmark_helpers.hpp"

#include "coding/reader.hpp"
#include "coding/writer.hpp"

#include <string>
#include <utility>

namespace track_import_tests
{
namespace
{
kml::FileData MakeCaptures(bool changeAltitude, bool withTimes, double extent = 0.001)
{
  kml::FileData data;
  auto & track = data.m_tracksData.emplace_back();
  kml::SetDefaultStr(track.m_name, "Moving");
  track.m_layers.emplace_back();
  auto const altitude = static_cast<geometry::Altitude>(changeAltitude ? 200 : 100);
  track.m_geometry.AddLine(
      {{{19.0, 0.0}, 100}, {{19.0, 0.0}, altitude}, {{19.0, 0.0}, altitude}, {{19.0 + extent, 0.0}, 300}});
  if (withTimes)
    track.m_geometry.AddTimestamps({1, 11, 11, 21});
  else
    track.m_geometry.AddTimestamps({});
  return data;
}

std::string Export(kml::FileData const & data, FileType type)
{
  std::string text;
  MemWriter writer(text);
  switch (type)
  {
  case FileType::Kml: TEST(SaveKmlData(data, writer, type), ()); break;
  case FileType::Gpx: kml::gpx::SerializerGpx(data).Serialize(writer); break;
  case FileType::GeoJson: kml::GeoJsonWriter(writer).Write(data, false /* minimizeOutput */); break;
  default: UNREACHABLE();
  }
  return text;
}
}  // namespace

UNIT_TEST(TrackImport_PreservesCoincidentMeasurements)
{
  for (auto const type : {FileType::Kml, FileType::Gpx, FileType::GeoJson})
    for (auto const [changeAltitude, withTimes] :
         {std::pair{true, false}, std::pair{false, true}, std::pair{true, true}})
    {
      auto const source = MakeCaptures(changeAltitude, withTimes);
      auto expected = source.m_tracksData.front().m_geometry;
      expected.m_lines.front().erase(expected.m_lines.front().begin() + 2);
      if (withTimes)
        expected.m_timestamps.front().erase(expected.m_timestamps.front().begin() + 2);

      auto imported = LoadKmlData(MemReader(Export(source, type)), type);
      TEST(imported, (type));
      TEST_EQUAL(imported->m_tracksData.size(), 1, (type));
      TEST_EQUAL(imported->m_tracksData.front().m_geometry, expected, (type, changeAltitude, withTimes));

      auto reloaded = LoadKmlData(MemReader(Export(*imported, FileType::Kml)), FileType::Kml);
      TEST(reloaded, (type));
      TEST_EQUAL(reloaded->m_tracksData.size(), 1, (type));
      TEST_EQUAL(reloaded->m_tracksData.front().m_geometry, expected, (type, changeAltitude, withTimes));
    }
}

UNIT_TEST(TrackImport_DropsStationaryMeasurements)
{
  for (auto const type : {FileType::Kml, FileType::Gpx, FileType::GeoJson})
    for (auto const extent : {0.0, 1e-7})
      for (bool const withTimes : {false, true})
      {
        auto source = MakeCaptures(true /* changeAltitude */, withTimes, extent);
        kml::SetDefaultStr(source.m_tracksData.front().m_name, "Stationary");
        source.m_tracksData.push_back(MakeCaptures(true /* changeAltitude */, withTimes).m_tracksData.front());
        source.m_tracksData.back().m_localId = 1;

        auto imported = LoadKmlData(MemReader(Export(source, type)), type);
        TEST(imported, (type));
        TEST_EQUAL(imported->m_tracksData.size(), 1, (type, extent, withTimes));
        TEST_EQUAL(kml::GetDefaultStr(imported->m_tracksData.front().m_name), "Moving", (type));
      }
}

UNIT_TEST(TrackImport_CaptureOnlySamplesDoNotHideMovement)
{
  for (auto const type : {FileType::Kml, FileType::Gpx, FileType::GeoJson})
    for (auto const [changeAltitude, withTimes] :
         {std::pair{true, false}, std::pair{false, true}, std::pair{true, true}})
      for (bool const crossesAntimeridian : {false, true})
      {
        kml::FileData source;
        auto & track = source.m_tracksData.emplace_back();
        track.m_layers.emplace_back();
        auto const altitude = static_cast<geometry::Altitude>(changeAltitude ? 200 : 100);
        track.m_geometry.AddLine({{{crossesAntimeridian ? -180.0 : 19.0, 0.0}, 100},
                                  {{crossesAntimeridian ? 179.999991 : 19.000009, 0.0}, altitude},
                                  {{crossesAntimeridian ? 179.999983 : 19.000017, 0.0}, altitude}});
        track.m_geometry.AddTimestamps({});
        if (withTimes)
          track.m_geometry.m_timestamps.front() = {1, 2, 2};
        TEST(track.m_geometry.IsValid(), ());

        auto imported = LoadKmlData(MemReader(Export(source, type)), type);
        TEST(imported, (type));
        TEST_EQUAL(imported->m_tracksData.size(), 1, (type, changeAltitude, withTimes, crossesAntimeridian));
        TEST_EQUAL(imported->m_tracksData.front().m_geometry, track.m_geometry, (type));
      }
}

UNIT_TEST(TrackImport_PreservesIntermediateCaptureTimes)
{
  for (auto const type : {FileType::Kml, FileType::Gpx, FileType::GeoJson})
  {
    auto source = MakeCaptures(false /* changeAltitude */, true /* withTimes */);
    source.m_tracksData.front().m_geometry.m_timestamps.front() = {1, 2, 3, 4};
    auto imported = LoadKmlData(MemReader(Export(source, type)), type);
    TEST(imported, (type));
    TEST_EQUAL(imported->m_tracksData.size(), 1, (type));
    TEST_EQUAL(imported->m_tracksData.front().m_geometry, source.m_tracksData.front().m_geometry, (type));
    auto reloaded = LoadKmlData(MemReader(Export(*imported, FileType::Kml)), FileType::Kml);
    TEST(reloaded, (type));
    TEST_EQUAL(reloaded->m_tracksData.size(), 1, (type));
    TEST_EQUAL(reloaded->m_tracksData.front().m_geometry, source.m_tracksData.front().m_geometry, (type));
  }
}
}  // namespace track_import_tests
