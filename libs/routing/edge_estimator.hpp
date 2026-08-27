#pragma once

#include "routing/segment.hpp"
#include "routing/vehicle_mask.hpp"

#include "routing_common/num_mwm_id.hpp"
#include "routing_common/vehicle_model.hpp"

#include "geometry/latlon.hpp"
#include "geometry/point_with_altitude.hpp"

#include <algorithm>
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

  /// \brief Calculation strategy. Normal — full road model (speed, traffic, penalties, climb).
  /// DistanceBiased — roads whose Normal weight speed is at or above the per-vehicle
  /// |distanceBiasCapSpeedKMpH| are ranked purely by distance (weight = distance / cap), while
  /// slower roads keep their Normal weight, so the search yields the shortest chain of reasonable
  /// roads without gaining from segments the model considers unreasonably slow (tracks, footways,
  /// steps, bad surfaces — see issue #12953). Weights never drop below distance / cap and thus
  /// below distance / GetMaxWeightSpeedMpS(), so CalcHeuristic() stays admissible and may even
  /// divide by the cap where no faster-priced edges exist (see |tightAllowed| there).
  /// Non-segment penalties applied at the graph layer (u-turn, ferry, etc. in
  /// IndexGraph::CalculateEdgeWeight) still take effect. Used to compute a reasonable,
  /// distance-biased alternative alongside the normal route.
  enum class Strategy
  {
    Normal,
    DistanceBiased
  };

  EdgeEstimator(double maxWeightSpeedKMpH, double distanceBiasCapSpeedKMpH, SpeedKMpH const & offroadSpeedKMpH,
                DataSource * dataSourcePtr = nullptr, std::shared_ptr<NumMwmIds> numMwmIds = nullptr);
  virtual ~EdgeEstimator() = default;

  /// |tightHeuristicAllowed| == false keeps CalcHeuristic() on the loose max-speed bound even under
  /// DistanceBiased. Needed when guides are attached: guides edges are priced at max speed
  /// (see GuidesGraph::CalcSegmentWeight), i.e. cheaper per meter than the cap.
  void SetStrategy(Strategy strategy, bool tightHeuristicAllowed = true)
  {
    m_strategy = strategy;
    m_tightHeuristicAllowed = tightHeuristicAllowed;
  }
  Strategy GetStrategy() const { return m_strategy; }

  /// \brief Transit alternative route bias. Scales the routing weight (Purpose::Weight only, ETA
  /// stays the real time) of walking legs and of the transit transfer/boarding penalty, to favour
  /// an alternative with less walking and fewer transfers (e.g. a direct bus over subway + walk).
  /// Both default to 1.0 (no bias).
  void SetTransitAltFactors(double walkWeightFactor, double transferPenaltyFactor)
  {
    m_transitWalkWeightFactor = walkWeightFactor;
    m_transitTransferFactor = transferPenaltyFactor;
  }
  double GetTransitWalkWeightFactor() const { return m_transitWalkWeightFactor; }
  double GetTransitTransferFactor() const { return m_transitTransferFactor; }

  /// A* heuristic: straight-line time at the maximum possible speed. Under Strategy::DistanceBiased
  /// every estimator weight is >= distance / cap, so when the caller guarantees the search sees no
  /// edges priced by other models (leap edges and precomputed cross-mwm weights in
  /// WorldGraphMode::LeapsOnly, guides edges — see SetStrategy), |tightAllowed| == true divides by
  /// the cap instead, keeping the heuristic admissible while much tighter.
  double CalcHeuristic(ms::LatLon const & from, ms::LatLon const & to, bool tightAllowed = false) const;
  // Estimates time in seconds it takes to go from point |from| to point |to| along a leap (fake)
  // edge |from|-|to| using real features.
  // Note 1. The result of the method should be used if it's necessary to add a leap (fake) edge
  // (|from|, |to|) in road graph.
  // Note 2. In WorldGraphMode::LeapsOnly, CalcHeuristic(|from|, |to|) must be less than or equal to
  // this result. The loose max-speed heuristic used in that mode provides this bound.
  // Note 3. It's assumed here that CalcLeapWeight(p1, p2) == CalcLeapWeight(p2, p1).
  double CalcLeapWeight(ms::LatLon const & from, ms::LatLon const & to, NumMwmId mwmId = kFakeNumMwmId);

  double GetMaxWeightSpeedMpS() const;

  // Estimates time in seconds it takes to go from point |from| to point |to| along direct fake edge.
  double CalcOffroad(ms::LatLon const & from, ms::LatLon const & to, Purpose purpose) const;

  virtual double CalcSegmentWeight(Segment const & segment, RoadGeometry const & road, Purpose purpose,
                                   time_t arrivalTime = 0) const = 0;
  virtual double GetUTurnPenalty(Purpose purpose) const = 0;
  virtual double GetFerryLandingPenalty(Purpose purpose) const = 0;

  static std::shared_ptr<EdgeEstimator> Create(VehicleType vehicleType, double maxWeighSpeedKMpH,
                                               SpeedKMpH const & offroadSpeedKMpH,
                                               std::shared_ptr<TrafficStash> trafficStash, DataSource * dataSourcePtr,
                                               std::shared_ptr<NumMwmIds> numMwmIds);

  static std::shared_ptr<EdgeEstimator> Create(VehicleType vehicleType, VehicleModelInterface const & vehicleModel,
                                               std::shared_ptr<TrafficStash> trafficStash, DataSource * dataSourcePtr,
                                               std::shared_ptr<NumMwmIds> numMwmIds);

protected:
  /// Applies Strategy::DistanceBiased to a Normal |weight|: roads not slower than the cap tie by
  /// pure distance, slower ones keep their full cost. No-op for Purpose::ETA and Strategy::Normal.
  double ApplyStrategy(Purpose purpose, double weight, double distanceM) const
  {
    if (purpose == Purpose::Weight && m_strategy == Strategy::DistanceBiased)
      return std::max(weight, distanceM * m_distanceBiasSecPerM);
    return weight;
  }

private:
  double const m_maxWeightSpeedMpS;
  double const m_distanceBiasSecPerM;
  SpeedKMpH const m_offroadSpeedKMpH;
  Strategy m_strategy = Strategy::Normal;
  bool m_tightHeuristicAllowed = true;
  double m_transitWalkWeightFactor = 1.0;
  double m_transitTransferFactor = 1.0;

  // DataSource * m_dataSourcePtr;
  // std::shared_ptr<NumMwmIds> m_numMwmIds;
  // std::unordered_map<NumMwmId, double> m_leapWeightSpeedMpS;

  double ComputeDefaultLeapWeightSpeed() const;
  double GetLeapWeightSpeed(NumMwmId mwmId);
  // double LoadLeapWeightSpeed(NumMwmId mwmId);
};

double GetPedestrianClimbPenalty(EdgeEstimator::Purpose purpose, double tangent, geometry::Altitude altitudeM);
double GetBicycleClimbPenalty(EdgeEstimator::Purpose purpose, double tangent, geometry::Altitude altitudeM);
double GetCarClimbPenalty(EdgeEstimator::Purpose purpose, double tangent, geometry::Altitude altitudeM);

}  // namespace routing
