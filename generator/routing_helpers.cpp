#include "generator/routing_helpers.hpp"

#include "generator/utils.hpp"

#include "base/logging.hpp"

namespace routing
{
template <class ToDo>
static bool ForEachWayFromFile(std::string const & filename, ToDo && toDo)
{
  return generator::ForEachOsmId2FeatureId(
      filename, [&](auto const & compositeOsmId, auto featureId) {
        auto const osmId = compositeOsmId.m_mainId;
        if (osmId.GetType() == base::GeoObjectId::Type::ObsoleteOsmWay)
          toDo(featureId, osmId);
      });
}

void AddFeatureId(base::GeoObjectId osmId, uint32_t featureId,
                  OsmIdToFeatureIds & osmIdToFeatureIds)
{
  osmIdToFeatureIds[osmId].push_back(featureId);
}

bool ParseWaysOsmIdToFeatureIdMapping(std::string const & osmIdsToFeatureIdPath,
                                      OsmIdToFeatureIds & osmIdToFeatureIds)
{
  return ForEachWayFromFile(osmIdsToFeatureIdPath,
                             [&](uint32_t featureId, base::GeoObjectId osmId) {
                               AddFeatureId(osmId, featureId, osmIdToFeatureIds);
                             });
}

bool ParseWaysFeatureIdToOsmIdMapping(std::string const & osmIdsToFeatureIdPath,
                                      FeatureIdToOsmId & featureIdToOsmId)
{
  featureIdToOsmId.clear();
  bool idsAreOk = true;

  bool const readSuccess = ForEachWayFromFile(
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
