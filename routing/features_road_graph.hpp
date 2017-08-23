#pragma once

#include "routing/road_graph.hpp"

#include "routing_common/vehicle_model.hpp"

#include "indexer/altitude_loader.hpp"
#include "indexer/feature_data.hpp"
#include "indexer/index.hpp"
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
  class CrossCountryVehicleModel : public VehicleModelInterface
  {
  public:
    CrossCountryVehicleModel(shared_ptr<VehicleModelFactoryInterface> vehicleModelFactory);

    // VehicleModelInterface overrides:
    double GetSpeed(FeatureType const & f) const override;
    double GetMaxSpeed() const override;
    bool IsOneWay(FeatureType const & f) const override;
    bool IsRoad(FeatureType const & f) const override;
    bool IsTransitAllowed(FeatureType const & f) const override;

    void Clear();

  private:
    VehicleModelInterface * GetVehicleModel(FeatureID const & featureId) const;

    shared_ptr<VehicleModelFactoryInterface> const m_vehicleModelFactory;
    double const m_maxSpeedKMPH;

    mutable map<MwmSet::MwmId, shared_ptr<VehicleModelInterface>> m_cache;
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
                    shared_ptr<VehicleModelFactoryInterface> vehicleModelFactory);

  static int GetStreetReadScale();

  // IRoadGraph overrides:
  RoadInfo GetRoadInfo(FeatureID const & featureId) const override;
  double GetSpeedKMPH(FeatureID const & featureId) const override;
  double GetMaxSpeedKMPH() const override;
  void ForEachFeatureClosestToCross(m2::PointD const & cross,
                                    ICrossEdgesLoader & edgesLoader) const override;
  void FindClosestEdges(m2::PointD const & point, uint32_t count,
                        vector<pair<Edge, Junction>> & vicinities) const override;
  void GetFeatureTypes(FeatureID const & featureId, feature::TypesHolder & types) const override;
  void GetJunctionTypes(Junction const & junction, feature::TypesHolder & types) const override;
  IRoadGraph::Mode GetMode() const override;
  void ClearState() override;

  bool IsRoad(FeatureType const & ft) const;

private:
  friend class CrossFeaturesLoader;

  struct Value
  {
    Value() = default;
    Value(Index const & index, MwmSet::MwmHandle handle);

    bool IsAlive() const { return m_mwmHandle.IsAlive(); }

    MwmSet::MwmHandle m_mwmHandle;
    unique_ptr<feature::AltitudeLoader> m_altitudeLoader;
  };

  bool IsOneWay(FeatureType const & ft) const;
  double GetSpeedKMPHFromFt(FeatureType const & ft) const;

  // Searches a feature RoadInfo in the cache, and if does not find then
  // loads feature from the index and takes speed for the feature from the vehicle model.
  RoadInfo const & GetCachedRoadInfo(FeatureID const & featureId) const;
  // Searches a feature RoadInfo in the cache, and if does not find then takes passed feature and speed.
  // This version is used to prevent redundant feature loading when feature speed is known.
  RoadInfo const & GetCachedRoadInfo(FeatureID const & featureId, FeatureType const & ft,
                                     double speedKMPH) const;
  void ExtractRoadInfo(FeatureID const & featureId, FeatureType const & ft, double speedKMPH,
                       RoadInfo & ri) const;

  Value const & LockMwm(MwmSet::MwmId const & mwmId) const;

  Index const & m_index;
  IRoadGraph::Mode const m_mode;
  mutable RoadInfoCache m_cache;
  mutable CrossCountryVehicleModel m_vehicleModel;
  mutable map<MwmSet::MwmId, Value> m_mwmLocks;
};

}  // namespace routing
