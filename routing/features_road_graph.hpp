#pragma once
#include "routing/road_graph.hpp"
#include "routing/vehicle_model.hpp"

#include "indexer/feature_data.hpp"
#include "indexer/mwm_set.hpp"

#include "geometry/point2d.hpp"

#include "base/cache.hpp"

#include "std/map.hpp"
#include "std/unique_ptr.hpp"
#include "std/vector.hpp"

class Index;
class FeatureType;

namespace routing
{

class FeaturesRoadGraph : public IRoadGraph
{
private:
  class CrossCountryVehicleModel : public IVehicleModel
  {
  public:
    CrossCountryVehicleModel(unique_ptr<IVehicleModelFactory> && vehicleModelFactory);

    // IVehicleModel overrides:
    double GetSpeed(FeatureType const & f) const override;
    double GetMaxSpeed() const override;
    bool IsOneWay(FeatureType const & f) const override;
    bool IsRoad(FeatureType const & f) const override;

    void Clear();

  private:
    IVehicleModel * GetVehicleModel(FeatureID const & featureId) const;

    unique_ptr<IVehicleModelFactory> const m_vehicleModelFactory;
    double const m_maxSpeedKMPH;

    mutable map<MwmSet::MwmId, shared_ptr<IVehicleModel>> m_cache;
  };

  class RoadInfoCache
  {
  public:
    RoadInfo & Find(FeatureID const & featureId, bool & found);

    void Clear();

  private:
    using TMwmFeatureCache = my::Cache<uint32_t, RoadInfo>;
    map<MwmSet::MwmId, TMwmFeatureCache> m_cache;
  };

public:
  FeaturesRoadGraph(Index const & index, IRoadGraph::Mode mode,
                    unique_ptr<IVehicleModelFactory> && vehicleModelFactory);

  static uint32_t GetStreetReadScale();

  // IRoadGraph overrides:
  RoadInfo GetRoadInfo(FeatureID const & featureId) const override;
  double GetSpeedKMPH(FeatureID const & featureId) const override;
  double GetMaxSpeedKMPH() const override;
  void ForEachFeatureClosestToCross(m2::PointD const & cross,
                                    ICrossEdgesLoader & edgesLoader) const override;
  void FindClosestEdges(m2::PointD const & point, uint32_t count,
                        vector<pair<Edge, m2::PointD>> & vicinities) const override;
  void GetFeatureTypes(FeatureID const & featureId, feature::TypesHolder & types) const override;
  void GetJunctionTypes(Junction const & junction, feature::TypesHolder & types) const override;
  IRoadGraph::Mode GetMode() const override;
  void ClearState() override;

private:
  friend class CrossFeaturesLoader;

  bool IsRoad(FeatureType const & ft) const;
  bool IsOneWay(FeatureType const & ft) const;
  double GetSpeedKMPHFromFt(FeatureType const & ft) const;

  // Searches a feature RoadInfo in the cache, and if does not find then
  // loads feature from the index and takes speed for the feature from the vehicle model.
  RoadInfo const & GetCachedRoadInfo(FeatureID const & featureId) const;
  // Searches a feature RoadInfo in the cache, and if does not find then takes passed feature and speed.
  // This version is used to prevent redundant feature loading when feature speed is known.
  RoadInfo const & GetCachedRoadInfo(FeatureID const & featureId,
                                     FeatureType & ft,
                                     double speedKMPH) const;

  void LockFeatureMwm(FeatureID const & featureId) const;

  Index const & m_index;
  IRoadGraph::Mode const m_mode;
  mutable RoadInfoCache m_cache;
  mutable CrossCountryVehicleModel m_vehicleModel;
  mutable map<MwmSet::MwmId, MwmSet::MwmHandle> m_mwmLocks;
};

}  // namespace routing
