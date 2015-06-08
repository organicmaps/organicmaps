#pragma once
#include "routing/road_graph.hpp"
#include "routing/vehicle_model.hpp"

#include "indexer/feature_data.hpp"
#include "indexer/mwm_set.hpp"

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
public:
  FeaturesRoadGraph(IVehicleModel const & vehicleModel, Index const & index, MwmSet::MwmId const & mwmID);

  static uint32_t GetStreetReadScale();

  inline MwmSet::MwmId GetMwmID() const { return m_mwmID; }

  double GetCacheMiss() const
  {
    if (m_cacheAccess == 0)
      return 0.0;
    return (double)m_cacheMiss / (double)m_cacheAccess;
  }

protected:
  // IRoadGraph overrides:
  RoadInfo GetRoadInfo(uint32_t featureId) const override;
  double GetSpeedKMPH(uint32_t featureId) const override;
  double GetMaxSpeedKMPH() const override;
  void ForEachFeatureClosestToCross(m2::PointD const & cross,
                                    CrossEdgesLoader & edgesLoader) const override;

private:
  friend class CrossFeaturesLoader;

  bool IsOneWay(FeatureType const & ft) const;
  double GetSpeedKMPHFromFt(FeatureType const & ft) const;
  void LoadFeature(uint32_t featureId, FeatureType & ft) const;

  // ft must be set if fullLoad set to false (optimization)
  RoadInfo const & GetCachedRoadInfo(uint32_t featureId, FeatureType & ft, bool fullLoad) const;

  Index const & m_index;
  MwmSet::MwmId const m_mwmID;
  IVehicleModel const & m_vehicleModel;
  
  mutable my::Cache<uint32_t, RoadInfo> m_cache;
  mutable uint32_t m_cacheMiss;
  mutable uint32_t m_cacheAccess;
};

}  // namespace routing
