#include "testing/testing.hpp"

#include "kml/serdes.hpp"
#include "kml/serdes_binary.hpp"
#include "kml/serdes_gpx.hpp"

#include "indexer/classificator_loader.hpp"

#include "coding/reader.hpp"
#include "coding/text_storage.hpp"
#include "coding/writer.hpp"

#include <array>
#include <string>
#include <vector>

namespace
{
// Synthetic records following the observed wire-10 layout, independent of the
// production visitors. IDs, coordinates, timestamps and texts are invented.
// Category: id 42, visible, modified 1750000000789 ms, five string slots.
std::vector<uint8_t> const kCategory = {0x2a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x95,
                                        0xbe, 0x83, 0xa1, 0xf7, 0x32, 0x00, 0x05, 0x01, 0x00, 0x01, 0x01,
                                        0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00};

// Bookmark 7: type index 56 (natural-peak), blue, Mountain, scale 17, created
// 1700000000 seconds, quantized point (2^29, 2^29), visible, localized names.
std::vector<uint8_t> const kBookmarkSeconds = {
    0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x38, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00,
    0x00, 0x00, 0x0c, 0x00, 0x11, 0x80, 0xe2, 0xcf, 0xaa, 0x06, 0x80, 0x80, 0x80, 0x80, 0x02, 0x80,
    0x80, 0x80, 0x80, 0x02, 0x01, 0x00, 0x95, 0xbe, 0x83, 0xa1, 0xf7, 0x32, 0x05, 0x03, 0x00, 0x02,
    0x01, 0x06, 0x08, 0x07, 0x01, 0x00, 0x03, 0x01, 0x00, 0x04, 0x00, 0x01, 0x00, 0x00};

// Bookmark 8: no feature types, green, Park, scale 12, created 1700000123456 ms,
// quantized point (2^28, 3*2^28), hidden. Both extra timestamps are 1750000000789 ms.
std::vector<uint8_t> const kBookmarkMillis = {
    0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x00, 0x0f, 0x00, 0x0c, 0xc0, 0x94,
    0x9d, 0xff, 0xbc, 0x31, 0x80, 0x80, 0x80, 0x80, 0x01, 0x80, 0x80, 0x80, 0x80, 0x03, 0x00, 0x00, 0x95, 0xbe, 0x83,
    0xa1, 0xf7, 0x32, 0x05, 0x01, 0x00, 0x05, 0x01, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00};

struct V10Fixture
{
  std::vector<uint8_t> m_category = kCategory;
  std::vector<std::vector<uint8_t>> m_bookmarks = {kBookmarkSeconds, kBookmarkMillis};
  std::vector<uint8_t> m_tracks = {0};

  std::vector<uint8_t> Serialize() const
  {
    std::vector<uint8_t> buffer;
    MemWriter sink(buffer);
    // Actual wire version, not Version::V8MM (10) or Version::V10MM (12).
    WriteToSink(sink, uint8_t{10});
    WriteToSink(sink, uint8_t{30});
    WriteZeroesToSink(sink, 40);
    std::array<uint64_t, 5> offsets;
    offsets[0] = sink.Pos() - 2;
    sink.Write(m_category.data(), m_category.size());
    offsets[1] = sink.Pos() - 2;
    WriteVarUint(sink, static_cast<uint32_t>(m_bookmarks.size()));
    for (auto const & bookmark : m_bookmarks)
      sink.Write(bookmark.data(), bookmark.size());
    offsets[2] = sink.Pos() - 2;
    sink.Write(m_tracks.data(), m_tracks.size());
    offsets[3] = sink.Pos() - 2;
    {
      // The compressed string format is unchanged. Small blocks exercise block lookup.
      coding::BlockedTextStorageWriter strings(sink, 16);
      for (auto const * text :
           {"", "V10 bookmarks", "Summit", "A synthetic bookmark", "Lookout", "Lake", "Mountain", "Вершина"})
        strings.Append(text);
    }
    offsets[4] = sink.Pos() - 2;
    sink.Seek(2);
    for (auto offset : offsets)
      WriteToSink(sink, offset);
    return buffer;
  }
};

struct V10TrackFixture
{
  uint64_t m_id = 11;
  uint64_t m_created = 0;
  uint32_t m_name = 2;
  std::vector<uint64_t> m_timestampsMillis;

  std::vector<uint8_t> Serialize() const
  {
    std::vector<uint8_t> buffer;
    MemWriter sink(buffer);
    WriteToSink(sink, m_id);
    WriteToSink(sink, uint8_t{2});  // Local ID.
    // One layer: width 5, custom RGBA 0x9c27b0ff (as in the samples).
    uint8_t const layer[] = {0x01, 0xb3, 0xe6, 0xcc, 0x19, 0x00, 0xff, 0xb0, 0x27, 0x9c};
    sink.Write(layer, sizeof(layer));
    WriteVarUint(sink, m_created);
    // Two independent segments, 2 and 3 points. Altitudes are missing in the
    // first segment and 100, 110, -5 in the second; its delta origin resets.
    uint8_t const geometry[] = {0x02, 0x02, 0x80, 0x80, 0x80, 0x80, 0x04, 0x80, 0x80, 0x80, 0x80,
                                0x04, 0xc8, 0x01, 0x90, 0x03, 0xff, 0xff, 0x03, 0x00, 0x03, 0x80,
                                0x80, 0x80, 0x80, 0x02, 0x80, 0x80, 0x80, 0x80, 0x06, 0x90, 0x03,
                                0xc8, 0x01, 0xd7, 0x04, 0x9f, 0x06, 0xc8, 0x01, 0x14, 0xe5, 0x01};
    sink.Write(geometry, sizeof(geometry));
    WriteToSink(sink, uint8_t{1});  // Visible.
    WriteZeroesToSink(sink, 2);
    WriteVarUint(sink, static_cast<uint32_t>(m_timestampsMillis.size()));
    for (auto const ms : m_timestampsMillis)
      WriteVarUint(sink, (ms << 7) | uint64_t{0x7f});
    WriteZeroesToSink(sink, 2);  // V10-only tail, absent in V9MM.
    // Name, description, and two unidentified empty slots.
    uint8_t const indexPrefix[] = {4, 1, 0};
    sink.Write(indexPrefix, sizeof(indexPrefix));
    WriteVarUint(sink, m_name);
    uint8_t const indexSuffix[] = {1, 0, 3, 0, 1, 0, 0};
    sink.Write(indexSuffix, sizeof(indexSuffix));
    return buffer;
  }
};

std::vector<uint8_t> MakeTracks(std::vector<V10TrackFixture> const & tracks)
{
  std::vector<uint8_t> buffer;
  MemWriter sink(buffer);
  WriteVarUint(sink, static_cast<uint32_t>(tracks.size()));
  for (auto const & track : tracks)
  {
    auto const record = track.Serialize();
    sink.Write(record.data(), record.size());
  }
  return buffer;
}

kml::FileData ReadKmb(std::vector<uint8_t> const & buffer)
{
  MemReader reader(buffer.data(), buffer.size());
  kml::FileData data;
  kml::binary::DeserializerKml(data).Deserialize(reader);
  return data;
}

void TestUnsupported(std::vector<uint8_t> const & buffer, std::string const & message)
{
  MemReader reader(buffer.data(), buffer.size());
  kml::FileData data;
  bool rejected = false;
  try
  {
    kml::binary::DeserializerKml(data).Deserialize(reader);
  }
  catch (kml::binary::DeserializerKml::DeserializeException const & ex)
  {
    rejected = true;
    TEST(ex.Msg().find(message) != std::string::npos, (ex.Msg(), message));
  }
  TEST(rejected, (message));
  TEST(data.m_bookmarksData.empty(), ("An unsupported file must not return a partial import"));
  TEST(data.m_tracksData.empty(), ());
  TEST(data.m_categoryData.m_name.empty(), ());
}
}  // namespace

UNIT_TEST(Kml_V10MM_Bookmarks)
{
  classificator::Load();
  auto const data = ReadKmb(V10Fixture().Serialize());
  TEST(data.m_deviceId.empty(), ());
  TEST(data.m_tracksData.empty(), ());
  auto const & category = data.m_categoryData;
  TEST_EQUAL(category.m_id, 42, ());
  TEST_EQUAL(kml::GetDefaultStr(category.m_name), "V10 bookmarks", ());
  TEST(category.m_visible, ());
  TEST_EQUAL(kml::ToSecondsSinceEpoch(category.m_lastModified), 1750000000, ());
  TEST(category.m_description.empty(), ());
  TEST(category.m_properties.empty(), ());

  TEST_EQUAL(data.m_bookmarksData.size(), 2, ());
  auto const & first = data.m_bookmarksData[0];
  TEST_EQUAL(first.m_id, 7, ());
  TEST_EQUAL(kml::GetDefaultStr(first.m_name), "Summit", ());
  TEST_EQUAL(first.m_name.at(1), "Mountain", ());
  TEST_EQUAL(first.m_name.at(8), "Вершина", ());
  TEST_EQUAL(kml::GetDefaultStr(first.m_description), "A synthetic bookmark", ());
  TEST_EQUAL(kml::GetDefaultStr(first.m_customName), "Lookout", ());
  TEST_EQUAL(first.m_featureTypes.size(), 1, ());
  TEST_EQUAL(first.m_featureTypes[0], classif().GetTypeByPath({"natural", "peak"}), ());
  TEST_EQUAL(first.m_color.m_predefinedColor, kml::PredefinedColor::Blue, ());
  TEST_EQUAL(first.m_icon, kml::BookmarkIcon::Mountain, ());
  TEST_EQUAL(first.m_viewportScale, 17, ());
  TEST_EQUAL(kml::ToSecondsSinceEpoch(first.m_timestamp), 1700000000, ());
  TEST_ALMOST_EQUAL_ABS(first.m_point.x, 0.0, 1e-6, ());
  TEST_ALMOST_EQUAL_ABS(first.m_point.y, 0.0, 1e-6, ());
  TEST(first.m_visible, ());
  TEST(first.m_boundTracks.empty(), ());
  TEST(first.m_nearestToponym.empty(), ());
  TEST(first.m_properties.empty(), ());

  auto const & second = data.m_bookmarksData[1];
  TEST_EQUAL(second.m_id, 8, ());
  TEST_EQUAL(kml::GetDefaultStr(second.m_name), "Lake", ());
  TEST(second.m_featureTypes.empty(), ());
  TEST_EQUAL(second.m_color.m_predefinedColor, kml::PredefinedColor::Green, ());
  TEST_EQUAL(second.m_icon, kml::BookmarkIcon::Park, ());
  TEST_EQUAL(second.m_viewportScale, 12, ());
  TEST_EQUAL(kml::ToSecondsSinceEpoch(second.m_timestamp), 1700000123, ());
  TEST_ALMOST_EQUAL_ABS(second.m_point.x, -90.0, 1e-6, ());
  TEST_ALMOST_EQUAL_ABS(second.m_point.y, 90.0, 1e-6, ());
  TEST(!second.m_visible, ());

  std::vector<uint8_t> saved;
  MemWriter sink(saved);
  kml::binary::SerializerKml(data).Serialize(sink);
  TEST_EQUAL(saved.front(), 9, ("OM must keep writing the existing format"));
  TEST_EQUAL(ReadKmb(saved), data, ());
}

UNIT_TEST(Kml_V10MM_BookmarkCounts)
{
  classificator::Load();
  V10Fixture fixture;
  fixture.m_bookmarks.clear();
  TEST(ReadKmb(fixture.Serialize()).m_bookmarksData.empty(), ());

  fixture.m_bookmarks.assign(129, kBookmarkSeconds);
  // A different final record catches a count/alignment error after the first 128.
  fixture.m_bookmarks.back() = kBookmarkMillis;
  auto const data = ReadKmb(fixture.Serialize());
  TEST_EQUAL(data.m_bookmarksData.size(), 129, ());
  TEST_EQUAL(kml::GetDefaultStr(data.m_bookmarksData[127].m_name), "Summit", ());
  TEST_EQUAL(kml::GetDefaultStr(data.m_bookmarksData.back().m_name), "Lake", ());
}

UNIT_TEST(Kml_V10MM_UnsupportedContent)
{
  classificator::Load();
  V10Fixture fixture;
  for (auto const offset : {9, 16})
  {
    fixture = {};
    fixture.m_category[offset] = 1;
    TestUnsupported(fixture.Serialize(), "Unsupported nonzero field");
  }
  fixture = {};
  fixture.m_bookmarks[1][34] = 1;
  TestUnsupported(fixture.Serialize(), "Unsupported nonzero field");

  for (auto const offset : {23, 26, 30})
  {
    fixture = {};
    fixture.m_category[offset] = 2;
    TestUnsupported(fixture.Serialize(), "Unsupported metadata");
  }
  fixture = {};
  // Populate the fourth category string slot, originally an empty subindex.
  fixture.m_category[27] = 1;
  fixture.m_category.insert(fixture.m_category.begin() + 28, {0, 2});
  TestUnsupported(fixture.Serialize(), "Unsupported metadata");

  fixture = {};
  fixture.m_bookmarks[1].back() = 2;
  TestUnsupported(fixture.Serialize(), "Unsupported metadata");
  fixture = {};
  fixture.m_bookmarks[1][51] = 1;
  fixture.m_bookmarks[1].insert(fixture.m_bookmarks[1].begin() + 52, {0, 2});
  TestUnsupported(fixture.Serialize(), "Unsupported metadata");
}

UNIT_TEST(Kml_V10MM_SectionConsumption)
{
  classificator::Load();
  V10Fixture fixture;
  fixture.m_category.push_back(0);
  TestUnsupported(fixture.Serialize(), "Unsupported trailing data");
  fixture = {};
  fixture.m_bookmarks.back().push_back(0);
  TestUnsupported(fixture.Serialize(), "Unsupported trailing data");
  fixture = {};
  fixture.m_tracks.push_back(0);
  TestUnsupported(fixture.Serialize(), "Unsupported trailing data");

  fixture = {};
  fixture.m_category[17] = 4;
  TestUnsupported(fixture.Serialize(), "Unsupported string index");
  fixture = {};
  fixture.m_bookmarks[1][41] = 4;
  TestUnsupported(fixture.Serialize(), "Unsupported string index");

  auto buffer = V10Fixture().Serialize();
  buffer[2] = 48;
  TestUnsupported(buffer, "Unsupported section layout");
}

UNIT_TEST(Kml_Rejects_Unknown_Wire_Versions)
{
  for (uint8_t version : {0, 1, 11, 12, 255})
    TestUnsupported({version}, "Incorrect file version");
}

UNIT_TEST(Kml_V10MM_Tracks)
{
  classificator::Load();
  V10Fixture fixture;
  V10TrackFixture timed;
  timed.m_timestampsMillis = {1750000000123, 1750000001123, 1750000002123, 1750000003123, 1750000004123};
  V10TrackFixture untimed;
  untimed.m_id = 12;
  untimed.m_name = 0;
  fixture.m_tracks = MakeTracks({timed, untimed});
  auto const data = ReadKmb(fixture.Serialize());
  TEST_EQUAL(data.m_bookmarksData.size(), 2, ());
  TEST_EQUAL(data.m_tracksData.size(), 2, ());
  auto const & first = data.m_tracksData[0];
  auto const & second = data.m_tracksData[1];
  TEST_EQUAL(first.m_id, 11, ());
  TEST_EQUAL(second.m_id, 12, ());
  TEST_EQUAL(first.m_localId, 2, ());
  TEST_EQUAL(kml::GetDefaultStr(first.m_name), "Summit", ());
  TEST_EQUAL(kml::GetDefaultStr(first.m_description), "A synthetic bookmark", ());
  TEST(second.m_name.empty(), ("Allow the application to name unnamed tracks"));
  TEST(first.m_visible, ());
  TEST_EQUAL(first.m_layers.size(), 1, ());
  TEST_ALMOST_EQUAL_ABS(first.m_layers[0].m_lineWidth, 5.0, 1e-5, ());
  TEST_EQUAL(first.m_layers[0].m_color.m_rgba, 0x9c27b0ff, ());

  auto const & geometry = first.m_geometry;
  TEST(geometry.IsValid(), ());
  TEST_EQUAL(geometry.m_lines.size(), 2, ());
  TEST_EQUAL(geometry.m_lines[0].size(), 2, ());
  TEST_EQUAL(geometry.m_lines[1].size(), 3, ());
  TEST_ALMOST_EQUAL_ABS(geometry.m_lines[0][0].GetPoint().x, 0.0, 1e-6, ());
  TEST_ALMOST_EQUAL_ABS(geometry.m_lines[1][0].GetPoint().x, -90.0, 1e-6, ());
  TEST_ALMOST_EQUAL_ABS(geometry.m_lines[1][0].GetPoint().y, 90.0, 1e-6, ());
  TEST_EQUAL(geometry.m_lines[0][0].GetAltitude(), geometry::kInvalidAltitude, ());
  TEST_EQUAL(geometry.m_lines[0][1].GetAltitude(), geometry::kInvalidAltitude, ());
  TEST_EQUAL(geometry.m_lines[1][0].GetAltitude(), 100, ());
  TEST_EQUAL(geometry.m_lines[1][1].GetAltitude(), 110, ());
  TEST_EQUAL(geometry.m_lines[1][2].GetAltitude(), -5, ());
  TEST_EQUAL(geometry.m_timestamps[0], kml::MultiGeometry::TimeT({1750000000, 1750000001}), ());
  TEST_EQUAL(geometry.m_timestamps[1], kml::MultiGeometry::TimeT({1750000002, 1750000003, 1750000004}), ());
  TEST(second.m_geometry.IsValid(), ());
  TEST(!second.m_geometry.HasTimestamps(), ());
  TEST_EQUAL(second.m_geometry.m_timestamps.size(), 2, ());

  // The application saves imported KMB as text KML. Both that path and GPX
  // export must retain segments, missing altitudes and per-point times.
  for (bool const gpx : {false, true})
  {
    std::string text;
    MemWriter sink(text);
    kml::FileData reloaded;
    if (gpx)
    {
      kml::gpx::SerializerGpx(data).Serialize(sink);
      kml::DeserializerGpx(reloaded).Deserialize(MemReader(text));
    }
    else
    {
      kml::SerializerKml(data).Serialize(sink);
      kml::DeserializerKml(reloaded).Deserialize(MemReader(text));
    }
    TEST_EQUAL(reloaded.m_tracksData.size(), 2, (gpx));
    for (size_t t = 0; t < data.m_tracksData.size(); ++t)
    {
      auto const & before = data.m_tracksData[t].m_geometry;
      auto const & after = reloaded.m_tracksData[t].m_geometry;
      TEST_EQUAL(after.m_timestamps, before.m_timestamps, (gpx, t));
      TEST_EQUAL(after.m_lines.size(), before.m_lines.size(), (gpx, t));
      for (size_t i = 0; i < before.m_lines.size(); ++i)
      {
        TEST_EQUAL(after.m_lines[i].size(), before.m_lines[i].size(), (gpx, t, i));
        for (size_t j = 0; j < before.m_lines[i].size(); ++j)
        {
          auto const & expected = before.m_lines[i][j];
          auto const & actual = after.m_lines[i][j];
          TEST_EQUAL(actual.GetAltitude(), expected.GetAltitude(), (gpx, t, i, j));
          TEST_ALMOST_EQUAL_ABS(actual.GetPoint().x, expected.GetPoint().x, 2e-6, (gpx, t, i, j));
          TEST_ALMOST_EQUAL_ABS(actual.GetPoint().y, expected.GetPoint().y, 2e-6, (gpx, t, i, j));
        }
      }
    }
  }
}

UNIT_TEST(Kml_V10MM_TrackMetadataTime)
{
  classificator::Load();
  V10Fixture fixture;
  V10TrackFixture track;
  track.m_name = 0;
  track.m_timestampsMillis = {1750000000123};
  fixture.m_tracks = MakeTracks({track});
  auto data = ReadKmb(fixture.Serialize());
  TEST_EQUAL(kml::GetDefaultStr(data.m_tracksData[0].m_name), "V10 bookmarks", ());
  TEST_EQUAL(kml::ToSecondsSinceEpoch(data.m_tracksData[0].m_timestamp), 1750000000, ());
  TEST(!data.m_tracksData[0].m_geometry.HasTimestamps(), ());

  fixture.m_category[20] = 0;  // Category name also references the empty string.
  data = ReadKmb(fixture.Serialize());
  TEST(data.m_categoryData.m_name.empty(), ());
  TEST(data.m_tracksData[0].m_name.empty(), ());
  fixture.m_category = kCategory;

  // A metadata time must not replace an explicitly stored creation time.
  track.m_created = 1700000000456;
  fixture.m_tracks = MakeTracks({track});
  data = ReadKmb(fixture.Serialize());
  TEST_EQUAL(kml::ToSecondsSinceEpoch(data.m_tracksData[0].m_timestamp), 1700000000, ());
  TEST(!data.m_tracksData[0].m_geometry.HasTimestamps(), ());

  // Unmatched arrays cannot safely be assigned to points, but retain geometry.
  track.m_timestampsMillis = {1750000000123, 1750000001123};
  fixture.m_tracks = MakeTracks({track});
  data = ReadKmb(fixture.Serialize());
  TEST(!data.m_tracksData[0].m_geometry.HasTimestamps(), ());
  TEST_EQUAL(data.m_tracksData[0].m_geometry.m_lines[1].size(), 3, ());
}

UNIT_TEST(Kml_V10MM_TrackThroughV9Binary)
{
  classificator::Load();
  V10Fixture fixture;
  fixture.m_tracks = MakeTracks({V10TrackFixture{}});
  auto imported = ReadKmb(fixture.Serialize());

  auto rejects = [&]()
  {
    std::vector<uint8_t> output;
    bool rejected = false;
    try
    {
      MemWriter sink(output);
      kml::binary::SerializerKml(imported).Serialize(sink);
    }
    catch (kml::binary::SerializerKml::SerializeException const &)
    {
      rejected = true;
    }
    TEST(rejected, ());
    TEST(output.empty(), ("Unsupported geometry must not produce a partial KMB"));
  };
  rejects();  // Two track lines cannot fit in V9 KMB.

  auto & geometryToSave = imported.m_tracksData[0].m_geometry;
  geometryToSave.m_lines.resize(1);
  geometryToSave.m_timestamps.resize(1);
  geometryToSave.m_timestamps[0] = {1750000000, 1750000001};
  rejects();  // V9 KMB has no per-point timestamps.
  geometryToSave.m_timestamps[0].clear();

  std::vector<uint8_t> saved;
  MemWriter binarySink(saved);
  kml::binary::SerializerKml(imported).Serialize(binarySink);
  auto const reloaded = ReadKmb(saved);
  TEST_EQUAL(reloaded.m_tracksData.size(), 1, ());
  auto const & geometry = reloaded.m_tracksData[0].m_geometry;
  TEST_EQUAL(geometry.m_lines.size(), 1, ());
  TEST_EQUAL(geometry.m_timestamps.size(), geometry.m_lines.size(), ());
  TEST(geometry.m_timestamps[0].empty(), ());

  std::string text;
  MemWriter textSink(text);
  TEST_NO_THROW({ kml::SerializerKml(reloaded).Serialize(textSink); }, ());
}

UNIT_TEST(Kml_V10MM_UnsupportedTrackContent)
{
  classificator::Load();
  V10Fixture fixture;
  fixture.m_tracks = MakeTracks({V10TrackFixture{}, V10TrackFixture{}});
  fixture.m_tracks.back() = 1;
  TestUnsupported(fixture.Serialize(), "Unsupported metadata");
  fixture.m_tracks = MakeTracks({V10TrackFixture{}});
  // The penultimate unknown string slot is an empty subindex.
  fixture.m_tracks[fixture.m_tracks.size() - 4] = 1;
  fixture.m_tracks.insert(fixture.m_tracks.end() - 3, {0, 2});
  TestUnsupported(fixture.Serialize(), "Unsupported metadata");

  fixture.m_tracks = MakeTracks({V10TrackFixture{}});
  fixture.m_tracks.push_back(0);
  TestUnsupported(fixture.Serialize(), "Unsupported trailing data");
  // Two existing zero fields and the two V10-only fields after timestamps.
  for (size_t offset : {66, 67, 69, 70})
  {
    fixture.m_tracks = MakeTracks({V10TrackFixture{}});
    fixture.m_tracks[offset] = 1;
    TestUnsupported(fixture.Serialize(), "Unsupported nonzero field");
  }
}
