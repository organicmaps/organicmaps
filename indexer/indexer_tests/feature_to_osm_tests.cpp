#include "testing/testing.hpp"

#include "indexer/feature_decl.hpp"
#include "indexer/feature_to_osm.hpp"

#include "platform/mwm_version.hpp"

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

using namespace indexer;
using namespace std;

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
}  // namespace

UNIT_TEST(FeatureIdToGeoObjectIdBimap_Smoke)
{
  FrozenDataSource dataSource;

  FeatureIdToGeoObjectIdBimapBuilder origM;
  origM.Add(0, base::MakeOsmWay(123));

  vector<uint8_t> buf;
  {
    MemWriter<decltype(buf)> writer(buf);
    FeatureIdToGeoObjectIdSerDes::Serialize(writer, origM);
  }

  FeatureIdToGeoObjectIdBimap deserM(dataSource);
  {
    MemReader reader(buf.data(), buf.size());
    ReaderSource<MemReader> src(reader);
    FeatureIdToGeoObjectIdSerDes::Deserialize(src, deserM);
  }

  Entries expectedEntries = GetEntries(origM);
  Entries actualEntries = GetEntries(deserM);
  TEST(!expectedEntries.empty(), ());
  TEST_EQUAL(expectedEntries, actualEntries, ());
}
