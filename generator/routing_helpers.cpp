#include "generator/routing_helpers.hpp"

#include "generator/utils.hpp"

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
  return generator::ForEachOsmId2FeatureId(filename,
                                           [&](osm::Id const & osmId, uint32_t const featureId) {
                                             if (osmId.IsWay())
                                               toDo(featureId, osmId);
                                           });
}
}  // namespace

namespace routing
{
void AddFeatureId(osm::Id osmId, uint32_t featureId, map<osm::Id, uint32_t> &osmIdToFeatureId)
{
  // Failing to insert here usually means that two features were created
  // from one osm id, for example an area and its boundary.
  osmIdToFeatureId.insert(make_pair(osmId, featureId));
}

bool ParseOsmIdToFeatureIdMapping(string const & osmIdsToFeatureIdPath,
                                  map<osm::Id, uint32_t> & osmIdToFeatureId)
{
  return ForEachRoadFromFile(osmIdsToFeatureIdPath, [&](uint32_t featureId, osm::Id osmId) {
    AddFeatureId(osmId, featureId, osmIdToFeatureId);
  });
}

bool ParseFeatureIdToOsmIdMapping(string const & osmIdsToFeatureIdPath,
                                  map<uint32_t, connector::OsmId> & featureIdToOsmId)
{
  featureIdToOsmId.clear();
  bool idsAreOk = true;

  bool const readSuccess =
      ForEachRoadFromFile(osmIdsToFeatureIdPath, [&](uint32_t featureId, osm::Id osmId) {
        auto const emplaced = featureIdToOsmId.emplace(featureId, connector::OsmId(osmId.OsmId()));
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
