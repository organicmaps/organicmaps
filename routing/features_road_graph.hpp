#pragma once
#include "routing/road_graph.hpp"
#include "routing/vehicle_model.hpp"

#include "indexer/feature_data.hpp"

#include "geometry/point2d.hpp"

#include "base/cache.hpp"

#include "std/unique_ptr.hpp"
#include "std/vector.hpp"


class Index;
class FeatureType;

namespace routing
{
class FeaturesRoadGraph : public IRoadGraph
{
  // TODO (@gorshenin): ft is not set when feature is not loaded from
  // cache, investigate how to fix this.
  RoadInfo const & GetCachedRoadInfo(uint32_t const ftId, FeatureType & ft, bool fullLoad);

public:
  FeaturesRoadGraph(Index const * pIndex, size_t mwmID);

  static uint32_t GetStreetReadScale();

  inline size_t GetMwmID() const { return m_mwmID; }

  double GetCacheMiss() const
  {
    if (m_cacheAccess == 0)
      return 0.0;
    return (double)m_cacheMiss / (double)m_cacheAccess;
  }

protected:
  // IRoadGraph overrides:
  RoadInfo GetRoadInfo(uint32_t featureId) override;
  double GetSpeedKMPH(uint32_t featureId) override;
  void ForEachFeatureClosestToCross(m2::PointD const & cross,
                                    CrossTurnsLoader & turnsLoader) override;

private:
  friend class CrossFeaturesLoader;

  bool IsOneWay(FeatureType const & ft) const;

  double GetSpeedKMPHFromFt(FeatureType const & ft) const;

  void LoadFeature(uint32_t featureId, FeatureType & ft);

  Index const * m_pIndex;
  size_t m_mwmID;
  unique_ptr<IVehicleModel> m_vehicleModel;
  my::Cache<uint32_t, RoadInfo> m_cache;

  uint32_t m_cacheMiss;
  uint32_t m_cacheAccess;
};
} // namespace routing
