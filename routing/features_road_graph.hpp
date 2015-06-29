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

  inline MwmSet::MwmId const & GetMwmID() const { return m_mwmID; }

  double GetCacheMiss() const { return m_cache.GetCacheMiss(); }

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

  // Searches a feature RoadInfo in the cache, and if does not find then
  // loads feature from the index and takes speed for the feature from the vehicle model.
  RoadInfo const & GetCachedRoadInfo(uint32_t featureId) const;
  // Searches a feature RoadInfo in the cache, and if does not find then takes passed feature and speed.
  // This version is used to prevent redundant feature loading when feature speed is known.
  RoadInfo const & GetCachedRoadInfo(uint32_t featureId,
                                     FeatureType & ft,
                                     double speedKMPH) const;

  Index const & m_index;
  MwmSet::MwmId const m_mwmID;
  IVehicleModel const & m_vehicleModel;
  
  mutable my::CacheWithStat<uint32_t, RoadInfo> m_cache;
};

}  // namespace routing
