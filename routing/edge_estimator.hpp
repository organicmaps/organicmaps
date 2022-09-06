#pragma once

#include "routing/routing_options.hpp"
#include "routing/segment.hpp"
#include "routing/vehicle_mask.hpp"

#include "routing_common/num_mwm_id.hpp"
#include "routing_common/vehicle_model.hpp"

#include "geometry/latlon.hpp"
#include "geometry/point2d.hpp"
#include "geometry/point_with_altitude.hpp"

#include <memory>

class DataSource;

namespace routing
{
class RoadGeometry;
class TrafficStash;

class EdgeEstimator
{
public:
  enum class Purpose
  {
    Weight,
    ETA
  };

  enum class Strategy
  {
    Fastest,
    Shortest,
    FewerTurns
  };

  EdgeEstimator(double maxWeightSpeedKMpH, SpeedKMpH const & offroadSpeedKMpH,
                DataSource * dataSourcePtr = nullptr, std::shared_ptr<NumMwmIds> numMwmIds = nullptr);
  virtual ~EdgeEstimator() = default;

  double CalcHeuristic(ms::LatLon const & from, ms::LatLon const & to) const;
  // Estimates time in seconds it takes to go from point |from| to point |to| along a leap (fake)
  // edge |from|-|to| using real features.
  // Note 1. The result of the method should be used if it's necessary to add a leap (fake) edge
  // (|from|, |to|) in road graph.
  // Note 2. The result of the method should be less or equal to CalcHeuristic(|from|, |to|).
  // Note 3. It's assumed here that CalcLeapWeight(p1, p2) == CalcLeapWeight(p2, p1).
  double CalcLeapWeight(ms::LatLon const & from, ms::LatLon const & to, NumMwmId mwmId = kFakeNumMwmId);

  double GetMaxWeightSpeedMpS() const;

  // Estimates time in seconds it takes to go from point |from| to point |to| along direct fake edge.
  double CalcOffroad(ms::LatLon const & from, ms::LatLon const & to, Purpose purpose) const;

  RoutingOptions GetAvoidRoutingOptions() const;
  void SetAvoidRoutingOptions(RoutingOptions::RoadType options);

  Strategy GetStrategy() const;
  void SetStrategy(Strategy strategy);

  virtual double CalcSegmentWeight(Segment const & segment, RoadGeometry const & road,
                                   Purpose purpose) const = 0;
  virtual double GetUTurnPenalty(Purpose purpose) const = 0;
  virtual double GetTurnPenalty(Purpose purpose) const = 0;
  virtual double GetFerryLandingPenalty(Purpose purpose) const = 0;

  static std::shared_ptr<EdgeEstimator> Create(VehicleType vehicleType, double maxWeighSpeedKMpH,
                                               SpeedKMpH const & offroadSpeedKMpH,
                                               std::shared_ptr<TrafficStash> trafficStash,
                                               DataSource * dataSourcePtr,
                                               std::shared_ptr<NumMwmIds> numMwmIds);

  static std::shared_ptr<EdgeEstimator> Create(VehicleType vehicleType,
                                               VehicleModelInterface const & vehicleModel,
                                               std::shared_ptr<TrafficStash> trafficStash,
                                               DataSource * dataSourcePtr,
                                               std::shared_ptr<NumMwmIds> numMwmIds);

private:
  double const m_maxWeightSpeedMpS;
  SpeedKMpH const m_offroadSpeedKMpH;
  RoutingOptions m_avoidRoutingOptions;
  Strategy m_strategy;

  //DataSource * m_dataSourcePtr;
  //std::shared_ptr<NumMwmIds> m_numMwmIds;
  //std::unordered_map<NumMwmId, double> m_leapWeightSpeedMpS;

  double ComputeDefaultLeapWeightSpeed() const;
  double GetLeapWeightSpeed(NumMwmId mwmId);
  //double LoadLeapWeightSpeed(NumMwmId mwmId);
};

double GetPedestrianClimbPenalty(EdgeEstimator::Purpose purpose, double tangent,
                                 geometry::Altitude altitudeM);
double GetBicycleClimbPenalty(EdgeEstimator::Purpose purpose, double tangent,
                              geometry::Altitude altitudeM);
double GetCarClimbPenalty(EdgeEstimator::Purpose purpose, double tangent,
                          geometry::Altitude altitudeM);

}  // namespace routing
