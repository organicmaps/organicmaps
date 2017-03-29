#include "generator/routing_helpers.hpp"

#include "generator/gen_mwm_info.hpp"

#include "coding/file_reader.hpp"
#include "coding/reader.hpp"

#include "base/logging.hpp"

using std::map;
using std::string;

namespace routing
{
void AddFeatureId(map<uint64_t, uint32_t> & osmIdToFeatureId, uint32_t featureId, uint64_t osmId)
{
  auto const result = osmIdToFeatureId.insert(make_pair(osmId, featureId));
  if (!result.second)
  {
    LOG(LERROR, ("Osm id", osmId, "is included in two feature ids:", featureId,
                 osmIdToFeatureId.find(osmId)->second));
  }
}

bool ParseOsmIdToFeatureIdMapping(string const & osmIdsToFeatureIdPath,
                                  map<uint64_t, uint32_t> & osmIdToFeatureId)
{
  gen::OsmID2FeatureID osmIdsToFeatureIds;
  try
  {
    FileReader reader(osmIdsToFeatureIdPath);
    ReaderSource<FileReader> src(reader);
    osmIdsToFeatureIds.Read(src);
  }
  catch (FileReader::Exception const & e)
  {
    LOG(LWARNING, ("Exception while reading file:", osmIdsToFeatureIdPath, ". Msg:", e.Msg()));
    return false;
  }

  osmIdsToFeatureIds.ForEach([&](gen::OsmID2FeatureID::ValueT const & p) {
    if (p.first.IsWay())
    {
      AddFeatureId(osmIdToFeatureId, p.second /* feature id */, p.first.OsmId() /* osm id */);
    }
  });

  return true;
}
}  // namespace routing
