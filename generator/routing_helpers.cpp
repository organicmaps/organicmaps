#include "generator/routing_helpers.hpp"

#include "generator/gen_mwm_info.hpp"

#include "coding/file_reader.hpp"
#include "coding/reader.hpp"

#include "base/logging.hpp"

using std::map;
using std::string;

namespace
{
template <class ToDo>
bool ForEachRoadFromFile(string const & filename, ToDo && toDo)
{
  gen::OsmID2FeatureID osmIdsToFeatureIds;
  try
  {
    FileReader reader(filename);
    ReaderSource<FileReader> src(reader);
    osmIdsToFeatureIds.Read(src);
  }
  catch (FileReader::Exception const & e)
  {
    LOG(LERROR, ("Exception while reading file:", filename, ". Msg:", e.Msg()));
    return false;
  }

  osmIdsToFeatureIds.ForEach([&](gen::OsmID2FeatureID::ValueT const & p) {
    if (p.first.IsWay())
      toDo(p.second /* feature id */, p.first.OsmId());
  });

  return true;
}
}  // namespace

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
  return ForEachRoadFromFile(osmIdsToFeatureIdPath, [&](uint32_t featureId, uint64_t osmId) {
    AddFeatureId(osmIdToFeatureId, featureId, osmId);
  });
}

bool ParseFeatureIdToOsmIdMapping(string const & osmIdsToFeatureIdPath,
                                  map<uint32_t, uint64_t> & featureIdToOsmId)
{
  bool result = true;

  result &= ForEachRoadFromFile(osmIdsToFeatureIdPath, [&](uint32_t featureId, uint64_t osmId) {
    auto const emplaced = featureIdToOsmId.emplace(featureId, osmId);
    if (emplaced.second)
      return;

    result = false;
    LOG(LERROR,
        ("Feature id", featureId, "is included in two osm ids:", emplaced.first->second, osmId));
  });

  if (result)
    return true;

  LOG(LERROR, ("Can't load osm id mapping from", osmIdsToFeatureIdPath));
  featureIdToOsmId.clear();
  return false;
}
}  // namespace routing
