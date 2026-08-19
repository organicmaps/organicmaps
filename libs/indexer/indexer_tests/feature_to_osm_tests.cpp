#include "testing/testing.hpp"

#include "generator/generator_tests_support/test_with_custom_mwms.hpp"

#include "indexer/feature_decl.hpp"
#include "indexer/feature_to_osm.hpp"

#include "coding/file_writer.hpp"
#include "coding/reader.hpp"
#include "coding/writer.hpp"

#include "base/geo_object_id.hpp"
#include "base/macros.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "defines.hpp"

namespace feature_to_osm_tests
{
using namespace indexer;
using namespace generator::tests_support;

using Entries = std::vector<std::pair<uint32_t, base::GeoObjectId>>;

template <typename Cont>
Entries GetEntries(Cont const & cont)
{
  Entries res;
  cont.ForEachEntry([&res](uint32_t const fid, base::GeoObjectId const & gid)
  { res.emplace_back(std::make_pair(fid, gid)); });
  std::sort(res.begin(), res.end());
  return res;
}

class FeatureIdToGeoObjectIdTest : public TestWithCustomMwms
{
public:
  DataSource const & GetDataSource() const { return m_dataSource; }
};

UNIT_TEST(FeatureIdToGeoObjectId_BadHeader)
{
  using SerDes = FeatureIdToGeoObjectIdSerDes;

  auto const deserialize = [](std::vector<uint8_t> const & buf)
  {
    FeatureIdToGeoObjectIdBimapMem map;
    MemReader reader(buf.data(), buf.size());
    return SerDes::Deserialize(reader, map);
  };

  // Too short to hold the header.
  TEST(!deserialize(std::vector<uint8_t>(SerDes::kHeaderOffset - 1, 0)), ());

  // Long enough, but the magic does not match.
  TEST(!deserialize(std::vector<uint8_t>(2 * SerDes::kHeaderOffset, 0)), ());

  // Valid magic, unknown version.
  std::vector<uint8_t> buf(2 * SerDes::kHeaderOffset, 0);
  std::copy(SerDes::kHeaderMagic.begin(), SerDes::kHeaderMagic.end(), buf.begin());
  buf[SerDes::kHeaderMagic.size()] = 0xFF;
  TEST(!deserialize(buf), ());
}

UNIT_CLASS_TEST(FeatureIdToGeoObjectIdTest, Smoke)
{
  Entries const kEntries = {
      {0, base::MakeOsmNode(123)},
      {1, base::MakeOsmWay(456)},
      {2, base::MakeOsmRelation(789)},
  };

  FeatureIdToGeoObjectIdBimapMem origM;
  for (auto const & e : kEntries)
    origM.Add(e.first, e.second);

  // TestMwmBuilder will create the section but we will rewrite it right away.
  auto testWorldId = BuildWorld([&](TestMwmBuilder & builder) {});
  auto const testWorldPath = testWorldId.GetInfo()->GetLocalFile().GetPath(MapFileType::Map);

  std::vector<uint8_t> buf;
  {
    MemWriter<decltype(buf)> writer(buf);
    FeatureIdToGeoObjectIdSerDes::Serialize(writer, origM);
  }

  {
    FilesContainerW writer(testWorldPath, FileWriter::OP_WRITE_EXISTING);
    writer.Write(buf, FEATURE_TO_OSM_FILE_TAG);
  }

  FeatureIdToGeoObjectIdBimapMem deserMem;
  {
    MemReader reader(buf.data(), buf.size());
    TEST(FeatureIdToGeoObjectIdSerDes::Deserialize(reader, deserMem), ());
  }

  indexer::FeatureIdToGeoObjectIdOneWay deserOneWay(GetDataSource());
  TEST(deserOneWay.Load(), ());

  indexer::FeatureIdToGeoObjectIdTwoWay deserTwoWay(GetDataSource());
  TEST(deserTwoWay.Load(), ());

  Entries actualEntriesMem = GetEntries(deserMem);
  Entries actualEntriesOneWay = GetEntries(deserOneWay);
  Entries actualEntriesTwoWay = GetEntries(deserTwoWay);
  TEST_EQUAL(kEntries, actualEntriesMem, ());
  TEST_EQUAL(kEntries, actualEntriesOneWay, ());
  TEST_EQUAL(kEntries, actualEntriesTwoWay, ());

  for (auto const & entry : kEntries)
  {
    base::GeoObjectId gid;
    TEST(deserOneWay.GetGeoObjectId(FeatureID(testWorldId, entry.first), gid), ());
    TEST_EQUAL(entry.second, gid, ());

    TEST(deserTwoWay.GetGeoObjectId(FeatureID(testWorldId, entry.first), gid), ());
    TEST_EQUAL(entry.second, gid, ());
  }

  for (auto const & entry : kEntries)
  {
    FeatureID fid;
    TEST(deserTwoWay.GetFeatureID(entry.second, fid), ());
    TEST_EQUAL(entry.first, fid.m_index, ());
  }
}
}  // namespace feature_to_osm_tests
