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
      toDo(p.second /* feature id */, p.first /* osm id */);
  });

  return true;
}
}  // namespace

namespace routing
{
void AddFeatureId(osm::Id osmId, uint32_t featureId, map<osm::Id, uint32_t> &osmIdToFeatureId)
{
  auto const result = osmIdToFeatureId.insert(make_pair(osmId, featureId));
  if (!result.second)
  {
    LOG(LERROR, ("Osm id", osmId, "is included in two feature ids:", featureId,
                 osmIdToFeatureId.find(osmId)->second));
  }
}

bool ParseOsmIdToFeatureIdMapping(string const & osmIdsToFeatureIdPath,
                                  map<osm::Id, uint32_t> & osmIdToFeatureId)
{
  return ForEachRoadFromFile(osmIdsToFeatureIdPath, [&](uint32_t featureId, osm::Id osmId) {
    AddFeatureId(osmId, featureId, osmIdToFeatureId);
  });
}

bool ParseFeatureIdToOsmIdMapping(string const & osmIdsToFeatureIdPath,
                                  map<uint32_t, osm::Id> & featureIdToOsmId)
{
  featureIdToOsmId.clear();
  bool idsAreOk = true;

  bool const readSuccess =
      ForEachRoadFromFile(osmIdsToFeatureIdPath, [&](uint32_t featureId, osm::Id osmId) {
        auto const emplaced = featureIdToOsmId.emplace(featureId, osmId);
        if (emplaced.second)
          return;

        idsAreOk = false;
        LOG(LERROR, ("Feature id", featureId, "is included in two osm ids:", emplaced.first->second,
                     osmId));
      });

  if (readSuccess && idsAreOk)
    return true;

  LOG(LERROR, ("Can't load osm id mapping from", osmIdsToFeatureIdPath));
  featureIdToOsmId.clear();
  return false;
}
}  // namespace routing
