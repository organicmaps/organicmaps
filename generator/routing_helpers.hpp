#pragma once

#include "geometry/point2d.hpp"

#include "base/assert.hpp"
#include "base/geo_object_id.hpp"

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace feature
{
class FeatureBuilder;
}

namespace routing
{
using OsmIdToFeatureIds = std::map<base::GeoObjectId, std::vector<uint32_t>>;
/// @todo Make vector as FeatureID is continuous.
using FeatureIdToOsmId = std::map<uint32_t, base::GeoObjectId>;

// Adds |featureId| and corresponding |osmId| to |osmIdToFeatureIds|.
// Note. In general, one |featureId| may correspond to several osm ids.
// Or on the contrary one osm id may correspond to several feature ids. It may happens for example
// when an area and its boundary may correspond to the same osm id.
/// @note As for road features a road |osmId| may correspond to several feature ids if
/// the |osmId| is split by a mini_roundabout or a turning_loop.
void AddFeatureId(base::GeoObjectId osmId, uint32_t featureId, OsmIdToFeatureIds & osmIdToFeatureIds);

// These methods fill |osmIdToFeatureIds| and |featureIdToOsmId| with features of
// type base::GeoObjectId::Type::ObsoleteOsmWay. This type contains all the roads
// and besides that other linear objects like streams and fences.
/// @note One osm id can be mapped on several feature ids, as described above in AddFeatureId.
void ParseWaysOsmIdToFeatureIdMapping(std::string const & osmIdsToFeatureIdPath, OsmIdToFeatureIds & osmIdToFeatureIds);
void ParseWaysFeatureIdToOsmIdMapping(std::string const & osmIdsToFeatureIdPath, FeatureIdToOsmId & featureIdToOsmId);

class OsmWay2FeaturePoint
{
public:
  virtual ~OsmWay2FeaturePoint() = default;

  virtual void ForEachFeature(uint64_t wayID, std::function<void(uint32_t)> const & fn) = 0;
  /// Pass (featureID, nodeIdx) for the correspondent point |pt| in OSM |wayID|.
  /// @param[in]  candidateIdx  Used as a hint (like it was in original OSM way).
  virtual void ForEachNodeIdx(uint64_t wayID, uint32_t candidateIdx, m2::PointU pt,
                              std::function<void(uint32_t, uint32_t)> const & fn) = 0;
};

std::unique_ptr<OsmWay2FeaturePoint> CreateWay2FeatureMapper(std::string const & dataFilePath,
                                                             std::string const & osmIdsToFeatureIdsPath);

bool IsRoadWay(feature::FeatureBuilder const & fb);
}  // namespace routing
