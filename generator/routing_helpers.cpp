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
  return generator::ForEachOsmId2FeatureId(
      filename, [&](base::GeoObjectId const & osmId, uint32_t const featureId) {
        if (osmId.GetType() == base::GeoObjectId::Type::ObsoleteOsmWay)
          toDo(featureId, osmId);
      });
}
}  // namespace

namespace routing
{
void AddFeatureId(base::GeoObjectId osmId, uint32_t featureId,
                  map<base::GeoObjectId, uint32_t> & osmIdToFeatureId)
{
  // Failing to insert here usually means that two features were created
  // from one osm id, for example an area and its boundary.
  osmIdToFeatureId.emplace(osmId, featureId);
}

bool ParseOsmIdToFeatureIdMapping(string const & osmIdsToFeatureIdPath,
                                  map<base::GeoObjectId, uint32_t> & osmIdToFeatureId)
{
  return ForEachRoadFromFile(osmIdsToFeatureIdPath,
                             [&](uint32_t featureId, base::GeoObjectId osmId) {
                               AddFeatureId(osmId, featureId, osmIdToFeatureId);
                             });
}

bool ParseFeatureIdToOsmIdMapping(string const & osmIdsToFeatureIdPath,
                                  map<uint32_t, base::GeoObjectId> & featureIdToOsmId)
{
  featureIdToOsmId.clear();
  bool idsAreOk = true;

  bool const readSuccess = ForEachRoadFromFile(
      osmIdsToFeatureIdPath, [&](uint32_t featureId, base::GeoObjectId const & osmId) {
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
