#include "routing/edge_estimator.hpp"

#include "routing/geometry.hpp"
#include "routing/latlon_with_altitude.hpp"
#include "routing/routing_exceptions.hpp"
#include "routing/routing_helpers.hpp"
#include "routing/traffic_stash.hpp"

#include "traffic/speed_groups.hpp"

#include "geometry/distance_on_sphere.hpp"
#include "geometry/point_with_altitude.hpp"

#include "base/assert.hpp"

#include <algorithm>
#include <unordered_map>

namespace routing
{
using namespace routing;
using namespace std;
using namespace traffic;
using measurement_utils::KmphToMps;

namespace
{
geometry::Altitude constexpr kMountainSicknessAltitudeM = 2500;

double TimeBetweenSec(ms::LatLon const & from, ms::LatLon const & to, double speedMpS)
{
  CHECK_GREATER(speedMpS, 0.0, ("from:", from, "to:", to));

  double const distanceM = ms::DistanceOnEarth(from, to);
  return distanceM / speedMpS;
}

double CalcTrafficFactor(SpeedGroup speedGroup)
{
  if (speedGroup == SpeedGroup::TempBlock)
  {
    // impossible driving factor
    return 1.0E4;
  }

  double const percentage = 0.01 * kSpeedGroupThresholdPercentage[static_cast<size_t>(speedGroup)];
  CHECK_GREATER(percentage, 0.0, (speedGroup));
  return 1.0 / percentage;
}

template <typename GetClimbPenalty>
double CalcClimbSegment(EdgeEstimator::Purpose purpose, Segment const & segment,
                        RoadGeometry const & road, GetClimbPenalty && getClimbPenalty)
{
  LatLonWithAltitude const & from = road.GetJunction(segment.GetPointId(false /* front */));
  LatLonWithAltitude const & to = road.GetJunction(segment.GetPointId(true /* front */));
  SpeedKMpH const & speed = road.GetSpeed(segment.IsForward());

  double const distance = road.GetDistance(segment.GetSegmentIdx());
  double const speedMpS =
      KmphToMps(purpose == EdgeEstimator::Purpose::Weight ? speed.m_weight : speed.m_eta);
  CHECK_GREATER(speedMpS, 0.0, ("from:", from.GetLatLon(), "to:", to.GetLatLon(), "speed:", speed));
  double const timeSec = distance / speedMpS;

  if (base::AlmostEqualAbs(distance, 0.0, 0.1))
    return timeSec;

  double const altitudeDiff =
      static_cast<double>(to.GetAltitude()) - static_cast<double>(from.GetAltitude());
  return timeSec * getClimbPenalty(purpose, altitudeDiff / distance, to.GetAltitude());
}
}  // namespace

double GetPedestrianClimbPenalty(EdgeEstimator::Purpose purpose, double tangent,
                                 geometry::Altitude altitudeM)
{
  double constexpr kMinPenalty = 1.0;
  // Descent penalty is less then the ascent penalty.
  double const impact = tangent >= 0.0 ? 1.0 : 0.35;
  tangent = std::abs(tangent);

  if (altitudeM >= kMountainSicknessAltitudeM)
  {
    return kMinPenalty + (10.0 + (altitudeM - kMountainSicknessAltitudeM) * 10.0 / 1500.0) *
                             std::abs(tangent) * impact;
  }

  // ETA coefficients are calculated in https://github.com/mapsme/omim-scripts/pull/21
  auto const penalty = purpose == EdgeEstimator::Purpose::Weight
                           ? 5.0 * tangent + 7.0 * tangent * tangent
                           : 3.01 * tangent + 3.54 * tangent * tangent;

  return kMinPenalty + penalty * impact;
}

double GetBicycleClimbPenalty(EdgeEstimator::Purpose purpose, double tangent,
                              geometry::Altitude altitudeM)
{
  double constexpr kMinPenalty = 1.0;
  double const impact = tangent >= 0.0 ? 1.0 : 0.35;
  tangent = std::abs(tangent);

  if (altitudeM >= kMountainSicknessAltitudeM)
    return kMinPenalty + 50.0 * tangent * impact;

  // ETA coefficients are calculated in https://github.com/mapsme/omim-scripts/pull/22
  auto const penalty = purpose == EdgeEstimator::Purpose::Weight
                           ? 10.0 * tangent + 26.0 * tangent * tangent
                           : 8.8 * tangent + 6.51 * tangent * tangent;

  return kMinPenalty + penalty * impact;
}

double GetCarClimbPenalty(EdgeEstimator::Purpose /* purpose */, double /* tangent */,
                          geometry::Altitude /* altitudeM */)
{
  return 1.0;
}

// EdgeEstimator -----------------------------------------------------------------------------------
EdgeEstimator::EdgeEstimator(double maxWeightSpeedKMpH, SpeedKMpH const & offroadSpeedKMpH,
                             DataSource * /*dataSourcePtr*/, std::shared_ptr<NumMwmIds> /*numMwmIds*/)
  : m_maxWeightSpeedMpS(KmphToMps(maxWeightSpeedKMpH))
  , m_offroadSpeedKMpH(offroadSpeedKMpH)
  //, m_dataSourcePtr(dataSourcePtr)
  //, m_numMwmIds(numMwmIds)
{
  CHECK_GREATER(m_offroadSpeedKMpH.m_weight, 0.0, ());
  CHECK_GREATER(m_offroadSpeedKMpH.m_eta, 0.0, ());
  CHECK_GREATER_OR_EQUAL(m_maxWeightSpeedMpS, KmphToMps(m_offroadSpeedKMpH.m_weight), ());

  if (m_offroadSpeedKMpH.m_eta != kNotUsed)
    CHECK_GREATER_OR_EQUAL(m_maxWeightSpeedMpS, KmphToMps(m_offroadSpeedKMpH.m_eta), ());
}

double EdgeEstimator::CalcHeuristic(ms::LatLon const & from, ms::LatLon const & to) const
{
  return TimeBetweenSec(from, to, m_maxWeightSpeedMpS);
}

double EdgeEstimator::ComputeDefaultLeapWeightSpeed() const
{
  // 1.76 factor was computed as an average ratio of escape/enter speed to max MWM speed across all MWMs.
  //return m_maxWeightSpeedMpS / 1.76;

  /// @todo By VNG: Current m_maxWeightSpeedMpS is > 120 km/h, so estimating speed was > 60km/h
  /// for start/end fake edges by straight line! I strongly believe that this is very! optimistic.
  /// Set factor to 2.15:
  /// - lower bound Russia_MoscowDesnogorsk (https://github.com/organicmaps/organicmaps/issues/1071)
  /// - upper bound RussiaSmolenskRussiaMoscowTimeTest
  return m_maxWeightSpeedMpS / 2.15;
}

/*
double EdgeEstimator::LoadLeapWeightSpeed(NumMwmId mwmId)
{
  double leapWeightSpeed = ComputeDefaultLeapWeightSpeed();

  if (m_dataSourcePtr)
  {
    MwmSet::MwmHandle handle =
        m_dataSourcePtr->GetMwmHandleByCountryFile(m_numMwmIds->GetFile(mwmId));
    if (!handle.IsAlive())
      MYTHROW(RoutingException, ("Mwm", m_numMwmIds->GetFile(mwmId), "cannot be loaded."));

    if (handle.GetInfo())
      leapWeightSpeed = handle.GetInfo()->GetRegionData().GetLeapWeightSpeed(leapWeightSpeed);
  }

  if (leapWeightSpeed > m_maxWeightSpeedMpS)
    leapWeightSpeed = m_maxWeightSpeedMpS;

  return leapWeightSpeed;
}
*/

double EdgeEstimator::GetLeapWeightSpeed(NumMwmId /*mwmId*/)
{
  double defaultSpeed = ComputeDefaultLeapWeightSpeed();

  /// @todo By VNG: We don't have LEAP_SPEEDS_FILE to assign RegionData::SetLeapWeightSpeed
  /// unique for each MWM, so this is useless now. And what about possible races here?
//  if (mwmId != kFakeNumMwmId)
//  {
//    auto [speedIt, inserted] = m_leapWeightSpeedMpS.emplace(mwmId, defaultSpeed);
//    if (inserted)
//      speedIt->second = LoadLeapWeightSpeed(mwmId);
//    return speedIt->second;
//  }

  return defaultSpeed;
}

double EdgeEstimator::CalcLeapWeight(ms::LatLon const & from, ms::LatLon const & to, NumMwmId mwmId)
{
  return TimeBetweenSec(from, to, GetLeapWeightSpeed(mwmId));
}

double EdgeEstimator::GetMaxWeightSpeedMpS() const { return m_maxWeightSpeedMpS; }

double EdgeEstimator::CalcOffroad(ms::LatLon const & from, ms::LatLon const & to,
                                  Purpose purpose) const
{
  auto const offroadSpeedKMpH =
      purpose == Purpose::Weight ? m_offroadSpeedKMpH.m_weight : m_offroadSpeedKMpH.m_eta;
  if (offroadSpeedKMpH == kNotUsed)
    return 0.0;

  return TimeBetweenSec(from, to, KmphToMps(offroadSpeedKMpH));
}

// PedestrianEstimator -----------------------------------------------------------------------------
class PedestrianEstimator final : public EdgeEstimator
{
public:
  PedestrianEstimator(double maxWeightSpeedKMpH, SpeedKMpH const & offroadSpeedKMpH)
    : EdgeEstimator(maxWeightSpeedKMpH, offroadSpeedKMpH)
  {
  }

  // EdgeEstimator overrides:
  double GetUTurnPenalty(Purpose /* purpose */) const override { return 0.0 /* seconds */; }
  // Based on: https://confluence.mail.ru/display/MAPSME/Ferries
  double GetFerryLandingPenalty(Purpose purpose) const override
  {
    switch (purpose)
    {
    case Purpose::Weight: return 20.0 * 60.0;  // seconds
    case Purpose::ETA: return 8.0 * 60.0;      // seconds
    }
    UNREACHABLE();
  }

  double CalcSegmentWeight(Segment const & segment, RoadGeometry const & road,
                           Purpose purpose) const override
  {
    return CalcClimbSegment(purpose, segment, road, GetPedestrianClimbPenalty);
  }
};

// BicycleEstimator --------------------------------------------------------------------------------
class BicycleEstimator final : public EdgeEstimator
{
public:
  BicycleEstimator(double maxWeightSpeedKMpH, SpeedKMpH const & offroadSpeedKMpH)
    : EdgeEstimator(maxWeightSpeedKMpH, offroadSpeedKMpH)
  {
  }

  // EdgeEstimator overrides:
  double GetUTurnPenalty(Purpose /* purpose */) const override { return 20.0 /* seconds */; }
  // Based on: https://confluence.mail.ru/display/MAPSME/Ferries
  double GetFerryLandingPenalty(Purpose purpose) const override
  {
    switch (purpose)
    {
    case Purpose::Weight: return 20 * 60;  // seconds
    case Purpose::ETA: return 8 * 60;      // seconds
    }
    UNREACHABLE();
  }

  double CalcSegmentWeight(Segment const & segment, RoadGeometry const & road,
                           Purpose purpose) const override
  {
    return CalcClimbSegment(purpose, segment, road, GetBicycleClimbPenalty);
  }
};

// CarEstimator ------------------------------------------------------------------------------------
class CarEstimator final : public EdgeEstimator
{
public:
  CarEstimator(DataSource * dataSourcePtr, std::shared_ptr<NumMwmIds> numMwmIds,
               shared_ptr<TrafficStash> trafficStash, double maxWeightSpeedKMpH,
               SpeedKMpH const & offroadSpeedKMpH);

  // EdgeEstimator overrides:
  double CalcSegmentWeight(Segment const & segment, RoadGeometry const & road, Purpose purpose) const override;
  double GetUTurnPenalty(Purpose /* purpose */) const override;
  double GetFerryLandingPenalty(Purpose purpose) const override;

private:
  shared_ptr<TrafficStash> m_trafficStash;
};

CarEstimator::CarEstimator(DataSource * dataSourcePtr, std::shared_ptr<NumMwmIds> numMwmIds,
                           shared_ptr<TrafficStash> trafficStash, double maxWeightSpeedKMpH,
                           SpeedKMpH const & offroadSpeedKMpH)
  : EdgeEstimator(maxWeightSpeedKMpH, offroadSpeedKMpH, dataSourcePtr, numMwmIds)
  , m_trafficStash(move(trafficStash))
{
}

double CarEstimator::GetUTurnPenalty(Purpose /* purpose */) const
{
  // Adds 2 minutes penalty for U-turn. The value is quite arbitrary
  // and needs to be properly selected after a number of real-world
  // experiments.
  return 2 * 60;  // seconds
}

double CarEstimator::GetFerryLandingPenalty(Purpose purpose) const
{
  switch (purpose)
  {
  case Purpose::Weight: return 40 * 60;  // seconds
  // Based on https://confluence.mail.ru/display/MAPSME/Ferries
  case Purpose::ETA: return 20 * 60;  // seconds
  }
  UNREACHABLE();
}

double CarEstimator::CalcSegmentWeight(Segment const & segment, RoadGeometry const & road, Purpose purpose) const
{
  double result = CalcClimbSegment(purpose, segment, road, GetCarClimbPenalty);

  if (m_trafficStash)
  {
    SpeedGroup const speedGroup = m_trafficStash->GetSpeedGroup(segment);
    ASSERT_LESS(speedGroup, SpeedGroup::Count, ());
    double const trafficFactor = CalcTrafficFactor(speedGroup);
    result *= trafficFactor;
    if (speedGroup != SpeedGroup::Unknown && speedGroup != SpeedGroup::G5)
    {
      // Current time estimation are too optimistic.
      // Need more accurate tuning: traffic lights, traffic jams, road models and so on.
      // Add some penalty to make estimation of a more realistic.
      /// @todo Make accurate tuning, remove penalty.
      result *= 1.8;
    }
  }

  return result;
}

// EdgeEstimator -----------------------------------------------------------------------------------
// static
shared_ptr<EdgeEstimator> EdgeEstimator::Create(VehicleType vehicleType, double maxWeighSpeedKMpH,
                                                SpeedKMpH const & offroadSpeedKMpH,
                                                shared_ptr<TrafficStash> trafficStash,
                                                DataSource * dataSourcePtr,
                                                std::shared_ptr<NumMwmIds> numMwmIds)
{
  switch (vehicleType)
  {
  case VehicleType::Pedestrian:
  case VehicleType::Transit:
    return make_shared<PedestrianEstimator>(maxWeighSpeedKMpH, offroadSpeedKMpH);
  case VehicleType::Bicycle:
    return make_shared<BicycleEstimator>(maxWeighSpeedKMpH, offroadSpeedKMpH);
  case VehicleType::Car:
    return make_shared<CarEstimator>(dataSourcePtr, numMwmIds, trafficStash, maxWeighSpeedKMpH,
                                     offroadSpeedKMpH);
  case VehicleType::Count:
    CHECK(false, ("Can't create EdgeEstimator for", vehicleType));
    return nullptr;
  }
  UNREACHABLE();
}

// static
shared_ptr<EdgeEstimator> EdgeEstimator::Create(VehicleType vehicleType,
                                                VehicleModelInterface const & vehicleModel,
                                                shared_ptr<TrafficStash> trafficStash,
                                                DataSource * dataSourcePtr,
                                                std::shared_ptr<NumMwmIds> numMwmIds)
{
  return Create(vehicleType, vehicleModel.GetMaxWeightSpeed(), vehicleModel.GetOffroadSpeed(),
                trafficStash, dataSourcePtr, numMwmIds);
}
}  // namespace routing
