#pragma once

#include "routing/road_graph.hpp"

#include "routing_common/maxspeed_conversion.hpp"
#include "routing_common/vehicle_model.hpp"

#include "indexer/altitude_loader.hpp"
#include "indexer/feature_data.hpp"
#include "indexer/mwm_set.hpp"

#include "geometry/point2d.hpp"

#include "base/cache.hpp"

#include "std/map.hpp"
#include "std/unique_ptr.hpp"
#include "std/vector.hpp"

class DataSource;
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
    VehicleModelInterface::SpeedKMpH GetSpeed(FeatureType & f,
                                              SpeedParams const & speedParams) const override;
    double GetMaxWeightSpeed() const override { return m_maxSpeed; };
    double GetOffroadSpeed() const override;
    bool IsOneWay(FeatureType & f) const override;
    bool IsRoad(FeatureType & f) const override;
    bool IsPassThroughAllowed(FeatureType & f) const override;

    void Clear();

  private:
    VehicleModelInterface * GetVehicleModel(FeatureID const & featureId) const;

    shared_ptr<VehicleModelFactoryInterface> const m_vehicleModelFactory;
    double const m_maxSpeed;
    double const m_offroadSpeedKMpH;

    mutable map<MwmSet::MwmId, shared_ptr<VehicleModelInterface>> m_cache;
  };

  class RoadInfoCache
  {
  public:
    RoadInfo & Find(FeatureID const & featureId, bool & found);

    void Clear();

  private:
    using TMwmFeatureCache = base::Cache<uint32_t, RoadInfo>;
    map<MwmSet::MwmId, TMwmFeatureCache> m_cache;
  };

public:
  FeaturesRoadGraph(DataSource const & dataSource, IRoadGraph::Mode mode,
                    shared_ptr<VehicleModelFactoryInterface> vehicleModelFactory);

  static int GetStreetReadScale();

  // IRoadGraph overrides:
  RoadInfo GetRoadInfo(FeatureID const & featureId, SpeedParams const & speedParams) const override;
  double GetSpeedKMpH(FeatureID const & featureId, SpeedParams const & speedParams) const override;
  double GetMaxSpeedKMpH() const override;
  void ForEachFeatureClosestToCross(m2::PointD const & cross,
                                    ICrossEdgesLoader & edgesLoader) const override;
  void FindClosestEdges(m2::PointD const & point, uint32_t count,
                        vector<pair<Edge, Junction>> & vicinities) const override;
  void GetFeatureTypes(FeatureID const & featureId, feature::TypesHolder & types) const override;
  void GetJunctionTypes(Junction const & junction, feature::TypesHolder & types) const override;
  IRoadGraph::Mode GetMode() const override;
  void ClearState() override;

  bool IsRoad(FeatureType & ft) const;

private:
  friend class CrossFeaturesLoader;

  struct Value
  {
    Value() = default;
    Value(DataSource const & dataSource, MwmSet::MwmHandle handle);

    bool IsAlive() const { return m_mwmHandle.IsAlive(); }

    MwmSet::MwmHandle m_mwmHandle;
    unique_ptr<feature::AltitudeLoader> m_altitudeLoader;
  };

  bool IsOneWay(FeatureType & ft) const;
  double GetSpeedKMpHFromFt(FeatureType & ft, SpeedParams const & speedParams) const;

  // Searches a feature RoadInfo in the cache, and if does not find then
  // loads feature from the index and takes speed for the feature from the vehicle model.
  RoadInfo const & GetCachedRoadInfo(FeatureID const & featureId, SpeedParams const & speedParams) const;
  // Searches a feature RoadInfo in the cache, and if does not find then takes passed feature and speed.
  // This version is used to prevent redundant feature loading when feature speed is known.
  RoadInfo const & GetCachedRoadInfo(FeatureID const & featureId, FeatureType & ft,
                                     double speedKMPH) const;
  void ExtractRoadInfo(FeatureID const & featureId, FeatureType & ft, double speedKMpH,
                       RoadInfo & ri) const;

  Value const & LockMwm(MwmSet::MwmId const & mwmId) const;

  DataSource const & m_dataSource;
  IRoadGraph::Mode const m_mode;
  mutable RoadInfoCache m_cache;
  mutable CrossCountryVehicleModel m_vehicleModel;
  mutable map<MwmSet::MwmId, Value> m_mwmLocks;
};

// @returns a distance d such as that for a given point p any edge
// with start point s such as that |s - p| < d, and edge is considered outgouing from p.
// Symmetrically for ingoing edges.
double GetRoadCrossingRadiusMeters();

}  // namespace routing
