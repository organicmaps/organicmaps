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

using namespace indexer;
using namespace std;
using namespace generator::tests_support;

namespace
{
using Entries = vector<pair<uint32_t, base::GeoObjectId>>;

template <typename Cont>
Entries GetEntries(Cont const & cont)
{
  Entries res;
  cont.ForEachEntry([&res](uint32_t const fid, base::GeoObjectId const & gid) {
    res.emplace_back(make_pair(fid, gid));
  });
  sort(res.begin(), res.end());
  return res;
};

class FeatureIdToGeoObjectIdBimapTest : public TestWithCustomMwms
{
public:
  DataSource const & GetDataSource() const { return m_dataSource; }
};
}  // namespace

UNIT_CLASS_TEST(FeatureIdToGeoObjectIdBimapTest, Smoke)
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
  auto const testWorldPath = testWorldId.GetInfo()->GetLocalFile().GetPath(MapOptions::Map);

  vector<uint8_t> buf;
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
    FeatureIdToGeoObjectIdSerDes::Deserialize(reader, deserMem);
  }

  indexer::FeatureIdToGeoObjectIdBimap deserM(GetDataSource());
  TEST(deserM.Load(), ());

  Entries actualEntriesMem = GetEntries(deserMem);
  Entries actualEntries = GetEntries(deserM);
  TEST_EQUAL(kEntries, actualEntriesMem, ());
  TEST_EQUAL(kEntries, actualEntries, ());

  for (auto const & entry : kEntries)
  {
    base::GeoObjectId gid;
    TEST(deserM.GetGeoObjectId(FeatureID(testWorldId, entry.first), gid), ());
    TEST_EQUAL(entry.second, gid, ());
  }

  for (auto const & entry : kEntries)
  {
    FeatureID fid;
    TEST(deserM.GetFeatureID(entry.second, fid), ());
    TEST_EQUAL(entry.first, fid.m_index, ());
  }
}
