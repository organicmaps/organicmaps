#include "routing/edge_estimator.hpp"

#include "routing/geometry.hpp"
#include "routing/latlon_with_altitude.hpp"
#include "routing/routing_helpers.hpp"
#include "routing/traffic_stash.hpp"

#include "traffic/speed_groups.hpp"

#include "geometry/angles.hpp"
#include "geometry/distance_on_sphere.hpp"
#include "geometry/mercator.hpp"
#include "geometry/point_with_altitude.hpp"

#include "base/assert.hpp"
#include "base/math.hpp"

#include <algorithm>
#include <cmath>

namespace routing
{
using namespace traffic;
using measurement_utils::KmphToMps;

namespace
{
geometry::Altitude constexpr kMountainSicknessAltitudeM = 2500;

double TimeBetweenSec(ms::LatLon const & from, ms::LatLon const & to, double speedMpS)
{
  ASSERT_GREATER(speedMpS, 0.0, ("from:", from, "to:", to));

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
  ASSERT_GREATER(percentage, 0.0, (speedGroup));
  return 1.0 / percentage;
}

double GetSpeedMpS(EdgeEstimator::Purpose purpose, Segment const & segment, RoadGeometry const & road,
                   time_t arrivalTime = 0)
{
  SpeedKMpH const & speed = road.GetSpeed(segment.IsForward(), arrivalTime);
  double const speedMpS = KmphToMps(purpose == EdgeEstimator::Purpose::Weight ? speed.m_weight : speed.m_eta);
  ASSERT_GREATER(speedMpS, 0.0, (segment));
  return speedMpS;
}

bool IsTransit(std::optional<HighwayType> type)
{
  return type && (type == HighwayType::RouteFerry || type == HighwayType::RouteShuttleTrain);
}

// Simplified cycling power model, see Martin et al. (1998), "Validation of a Mathematical Model for
// Road Cycling Power". A rider holds a roughly constant power, and the wind changes the aerodynamic
// term only, so the ground speed drops much less than the headwind itself:
//   P = F_other * v + kDragFactor * v * (v + headwind) * |v + headwind|
// The drag keeps the sign of the airspeed: a tailwind faster than the rider pushes them along
// instead of resisting.
double constexpr kAirDensityKgPerM3 = 1.225;  // at sea level, 15 C
double constexpr kDragAreaM2 = 0.4;           // CdA of an upright rider
double constexpr kDragFactor = 0.5 * kAirDensityKgPerM3 * kDragAreaM2;
double constexpr kRollingForceN = 4.0;  // rolling resistance and drivetrain losses of a 80 kg rider + bicycle

/// \returns the power implied by |cruisingSpeedMpS| on flat pavement in still air: 64 W at 20 km/h,
/// 175 W at 30 km/h.
double RiderPowerW(double cruisingSpeedMpS)
{
  return kRollingForceN * cruisingSpeedMpS + kDragFactor * math::Pow2(cruisingSpeedMpS) * cruisingSpeedMpS;
}

/// \returns the ground speed of a rider holding |powerW| against |headwindMpS| (negative for a
/// tailwind) on a segment where they would ride |stillSpeedMpS| in still air.
double SpeedWithHeadwindMpS(double stillSpeedMpS, double headwindMpS, double powerW)
{
  ASSERT_GREATER(stillSpeedMpS, 0.0, ());

  // Everything that does not depend on the wind (rolling resistance, gravity, surface), recovered
  // from the speed the routing profile predicts for this segment. Climbs and dirt roads are slow
  // for reasons the wind does not change, so the wind barely slows them down further.
  double const otherForceN = powerW / stillSpeedMpS - kDragFactor * math::Pow2(stillSpeedMpS);

  auto const excessPowerW = [&](double speedMpS)
  {
    double const airSpeedMpS = speedMpS + headwindMpS;
    return otherForceN * speedMpS + kDragFactor * speedMpS * airSpeedMpS * std::fabs(airSpeedMpS) - powerW;
  };

  // The root is bracketed and unique: excessPowerW() is -powerW at 0 and powerW * |headwind| /
  // stillSpeed at the upper bound, and it crosses zero only once in between.
  double lo = 0.0;
  double hi = stillSpeedMpS + std::fabs(headwindMpS);
  for (size_t i = 0; i < 32; ++i)
  {
    double const mid = 0.5 * (lo + hi);
    (excessPowerW(mid) < 0.0 ? lo : hi) = mid;
  }
  return 0.5 * (lo + hi);
}

template <class CalcSpeed>
double CalcClimbSegment(EdgeEstimator::Purpose purpose, Segment const & segment, RoadGeometry const & road,
                        CalcSpeed && calcSpeed)
{
  double const distance = road.GetDistance(segment.GetSegmentIdx());
  double speedMpS = GetSpeedMpS(purpose, segment, road);

  static double constexpr kSmallDistanceM = 1;  // we have altitude threshold is 0.5m
  if (distance > kSmallDistanceM && !IsTransit(road.GetHighwayType()))
  {
    LatLonWithAltitude const & from = road.GetJunction(segment.GetPointId(false /* front */));
    LatLonWithAltitude const & to = road.GetJunction(segment.GetPointId(true /* front */));

    ASSERT(to.GetAltitude() != geometry::kInvalidAltitude && from.GetAltitude() != geometry::kInvalidAltitude, ());
    auto const altitudeDiff = to.GetAltitude() - from.GetAltitude();

    if (altitudeDiff != 0)
    {
      speedMpS = calcSpeed(speedMpS, altitudeDiff / distance, to.GetAltitude());
      ASSERT_GREATER(speedMpS, 0.0, (segment));
    }
  }

  return distance / speedMpS;
}
}  // namespace

double GetPedestrianClimbPenalty(EdgeEstimator::Purpose purpose, double tangent, geometry::Altitude altitudeM)
{
  double constexpr kMinPenalty = 1.0;
  // Descent penalty is less then the ascent penalty.
  double const impact = tangent >= 0.0 ? 1.0 : 0.35;

  if (altitudeM >= kMountainSicknessAltitudeM)
    return kMinPenalty + (10.0 + (altitudeM - kMountainSicknessAltitudeM) * 10.0 / 1500.0) * fabs(tangent) * impact;

  // Use magic constant from this table: https://en.wikipedia.org/wiki/Tobler's_hiking_function#Sample_values
  // Tobler's returns unusually big values for bigger tangent.
  // See Australia_Mountains_Downlhill test.
  if (purpose == EdgeEstimator::Purpose::Weight || fabs(tangent) > 1.19)
  {
    tangent = fabs(tangent);
    // Some thoughts about gradient and foot walking: https://gre-kow.livejournal.com/26916.html
    // 3cm diff with avg foot length 60cm is imperceptible (see Hungary_UseFootways).
    double constexpr kTangentThreshold = 3.0 / 60.0;
    if (tangent < kTangentThreshold)
      return kMinPenalty;

    // ETA coefficients are calculated in https://github.com/mapsme/omim-scripts/pull/21
    auto const penalty = purpose == EdgeEstimator::Purpose::Weight ? 5.0 * tangent + 7.0 * tangent * tangent
                                                                   : 3.01 * tangent + 3.54 * tangent * tangent;

    return kMinPenalty + penalty * impact;
  }
  else
  {
    // Use Tobler’s Hiking Function for ETA like more comprehensive. See France_Uphill_Downlhill test.
    // Why not in Weight? See Crimea_Altitude_Mountains test.
    // https://mtntactical.com/research/yet-calculating-movement-uneven-terrain/
    // Returns factor: W(0) / W(tangent).
    return exp(-3.5 * (0.05 - fabs(tangent + 0.05)));
  }
}

double GetBicycleClimbPenalty(EdgeEstimator::Purpose purpose, double tangent, geometry::Altitude altitudeM)
{
  double constexpr kMinPenalty = 1.0;
  double const impact = tangent >= 0.0 ? 1.0 : 0.35;

  if (altitudeM >= kMountainSicknessAltitudeM)
    return kMinPenalty + 50.0 * fabs(tangent) * impact;

  // By VNG: This approach is strange at least because it always returns penalty > 1 (even for downhill)
  /*
  tangent = fabs(tangent);
  // ETA coefficients are calculated in https://github.com/mapsme/omim-scripts/pull/22
  auto const penalty = purpose == EdgeEstimator::Purpose::Weight
                           ? 10.0 * tangent + 26.0 * tangent * tangent
                           : 8.8 * tangent + 6.51 * tangent * tangent;

  return kMinPenalty + penalty * impact;
  */

  // https://web.tecnico.ulisboa.pt/~rosamfelix/gis/declives/SpeedSlopeFactor.html
  double const slope = tangent * 100;

  double factor;
  if (slope < -30)
    factor = 1.5;
  else if (slope < 0)
  {
    // Min factor (max speed) will be at slope = -13.
    factor = 1 + 2 * 0.7 / 13.0 * slope + 0.7 / 169 * slope * slope;
  }
  else if (slope <= 20)
    factor = 1 + slope * slope / 49;
  else
    factor = 10.0;
  return factor;
}

double GetCarClimbPenalty(EdgeEstimator::Purpose, double, geometry::Altitude)
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
  // For the correct A*, we should use maximum _possible_ speed here, including:
  // default model, feature stored, unlimited autobahn, ferry or rail transit.
  return TimeBetweenSec(from, to, m_maxWeightSpeedMpS);
}

double EdgeEstimator::ComputeDefaultLeapWeightSpeed() const
{
  // 1.76 factor was computed as an average ratio of escape/enter speed to max MWM speed across all MWMs.
  // return m_maxWeightSpeedMpS / 1.76;

  // By VNG: Current m_maxWeightSpeedMpS is > 120 km/h, so estimating speed was > 60km/h
  // for start/end fake edges by straight line! I strongly believe that this is very! optimistic.
  // Set speed to 57.5km/h (16m/s):
  // - lower bound Russia_MoscowDesnogorsk (https://github.com/organicmaps/organicmaps/issues/1071)
  // - upper bound RussiaSmolenskRussiaMoscowTimeTest
  return 16.0;
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

double EdgeEstimator::GetMaxWeightSpeedMpS() const
{
  return m_maxWeightSpeedMpS;
}

void EdgeEstimator::SetRouteSpeedSettings(VehicleType vehicleType, RouteSpeedSettings const & settings)
{
  if (!IsRouteSpeedSupported(vehicleType) || settings.m_cruisingSpeedKMpH <= 0.0)
    return;

  // The routing profile already predicts the default cruising speed on flat pavement, so a user
  // riding faster than that covers every segment proportionally faster.
  m_etaSpeedFactor = settings.m_cruisingSpeedKMpH / GetCruisingSpeedRange(vehicleType).m_default;

  if (!IsWindSupported(vehicleType))
    return;
  m_windSpeedMpS = settings.m_windSpeedMpS;
  m_windFromDirectionRad = math::DegToRad(static_cast<double>(settings.m_windDirectionDegrees));
  m_riderPowerW = RiderPowerW(KmphToMps(settings.m_cruisingSpeedKMpH));
}

double EdgeEstimator::ApplyEtaSpeedFactor(double timeSec, Purpose purpose) const
{
  return purpose == Purpose::ETA ? timeSec / m_etaSpeedFactor : timeSec;
}

double EdgeEstimator::ApplyEtaWind(double timeSec, Purpose purpose, ms::LatLon const & from,
                                   ms::LatLon const & to) const
{
  timeSec = ApplyEtaSpeedFactor(timeSec, purpose);
  if (purpose != Purpose::ETA || m_windSpeedMpS == 0.0 || timeSec == 0.0)
    return timeSec;

  double const distanceM = ms::DistanceOnEarth(from, to);
  if (distanceM == 0.0)
    return timeSec;

  // Along-track wind component, positive for a headwind.
  double const course = ang::Azimuth(mercator::FromLatLon(from), mercator::FromLatLon(to));
  double const headwindMpS = m_windSpeedMpS * std::cos(course - m_windFromDirectionRad);
  return distanceM / SpeedWithHeadwindMpS(distanceM / timeSec, headwindMpS, m_riderPowerW);
}

double EdgeEstimator::CalcOffroad(ms::LatLon const & from, ms::LatLon const & to, Purpose purpose) const
{
  auto const offroadSpeedKMpH = purpose == Purpose::Weight ? m_offroadSpeedKMpH.m_weight : m_offroadSpeedKMpH.m_eta;
  if (offroadSpeedKMpH == kNotUsed)
    return 0.0;

  double const time = TimeBetweenSec(from, to, KmphToMps(offroadSpeedKMpH));
  return ApplyEtaSpeedFactor(purpose == Purpose::Weight ? time * m_transitWalkWeightFactor : time, purpose);
}

// PedestrianEstimator -----------------------------------------------------------------------------
class PedestrianEstimator final : public EdgeEstimator
{
public:
  PedestrianEstimator(double maxWeightSpeedKMpH, SpeedKMpH const & offroadSpeedKMpH)
    : EdgeEstimator(maxWeightSpeedKMpH, offroadSpeedKMpH)
  {}

  // EdgeEstimator overrides:
  double GetUTurnPenalty(Purpose /* purpose */) const override { return 0.0 /* seconds */; }
  double GetFerryLandingPenalty(Purpose purpose) const override
  {
    switch (purpose)
    {
    case Purpose::Weight: return 10 * 60;  // seconds
    case Purpose::ETA: return 8 * 60;      // seconds
    }
    UNREACHABLE();
  }

  double CalcSegmentWeight(Segment const & segment, RoadGeometry const & road, Purpose purpose,
                           time_t arrivalTime) const override
  {
    if (purpose == Purpose::Weight && GetStrategy() == Strategy::Shortest)
      return road.GetDistance(segment.GetSegmentIdx()) / GetMaxWeightSpeedMpS();

    double const weight =
        CalcClimbSegment(purpose, segment, road, [purpose](double speedMpS, double tangent, geometry::Altitude altitude)
    { return speedMpS / GetPedestrianClimbPenalty(purpose, tangent, altitude); });
    // Bias the transit alternative away from walking (see SetTransitAltFactors). No-op (factor 1.0)
    // for standalone pedestrian routing and for the primary transit route.
    double const adjustedWeight = purpose == Purpose::Weight ? weight * GetTransitWalkWeightFactor() : weight;
    // A ferry does not sail faster for a faster walker.
    if (IsTransit(road.GetHighwayType()))
      return adjustedWeight;
    return ApplyEtaSpeedFactor(adjustedWeight, purpose);
  }
};

// BicycleEstimator --------------------------------------------------------------------------------
class BicycleEstimator final : public EdgeEstimator
{
public:
  BicycleEstimator(double maxWeightSpeedKMpH, SpeedKMpH const & offroadSpeedKMpH)
    : EdgeEstimator(maxWeightSpeedKMpH, offroadSpeedKMpH)
  {}

  // EdgeEstimator overrides:
  double GetUTurnPenalty(Purpose /* purpose */) const override { return 20.0 /* seconds */; }
  double GetFerryLandingPenalty(Purpose purpose) const override
  {
    switch (purpose)
    {
    case Purpose::Weight: return 10 * 60;  // seconds
    case Purpose::ETA: return 8 * 60;      // seconds
    }
    UNREACHABLE();
  }

  double CalcSegmentWeight(Segment const & segment, RoadGeometry const & road, Purpose purpose,
                           time_t arrivalTime) const override
  {
    if (purpose == Purpose::Weight && GetStrategy() == Strategy::Shortest)
      return road.GetDistance(segment.GetSegmentIdx()) / GetMaxWeightSpeedMpS();

    double const time = CalcClimbSegment(purpose, segment, road,
                                         [purpose, this](double speedMpS, double tangent, geometry::Altitude altitude)
    {
      auto const factor = GetBicycleClimbPenalty(purpose, tangent, altitude);
      ASSERT_GREATER(factor, 0.0, ());

      /// @todo Take out "bad" bicycle road (path, track, footway, ...) check into BicycleModel?
      static double constexpr badBicycleRoadSpeed = KmphToMps(9);
      if (speedMpS <= badBicycleRoadSpeed)
      {
        if (factor > 1)
          speedMpS /= factor;
      }
      else if (factor > 1)
      {
        // Calculate uphill speed according to the average bicycle speed, because "good-roads" like
        // residential, secondary, cycleway are "equal-low-speed" uphill and road type doesn't matter.
        static double constexpr avgBicycleSpeed = KmphToMps(20);
        double const upperBound = avgBicycleSpeed / factor;
        if (speedMpS > upperBound)
        {
          // Add small weight to distinguish roads by class (10 is a max factor value).
          speedMpS = upperBound + (purpose == Purpose::Weight ? speedMpS / (10 * avgBicycleSpeed) : 0);
        }
      }
      else
        speedMpS /= factor;

      return std::min(speedMpS, GetMaxWeightSpeedMpS());
    });
    // A ferry does not sail faster for a faster rider, and its deck is sheltered from the wind.
    if (IsTransit(road.GetHighwayType()))
      return time;
    auto const & from = road.GetJunction(segment.GetPointId(false /* front */)).GetLatLon();
    auto const & to = road.GetJunction(segment.GetPointId(true /* front */)).GetLatLon();
    return ApplyEtaWind(time, purpose, from, to);
  }
};

// CarEstimator ------------------------------------------------------------------------------------
class CarEstimator final : public EdgeEstimator
{
#ifdef DEBUG
  mutable double m_lastETASpeed = -1;
#endif

public:
  CarEstimator(DataSource * dataSourcePtr, std::shared_ptr<NumMwmIds> numMwmIds,
               std::shared_ptr<TrafficStash> trafficStash, double maxWeightSpeedKMpH,
               SpeedKMpH const & offroadSpeedKMpH)
    : EdgeEstimator(maxWeightSpeedKMpH, offroadSpeedKMpH, dataSourcePtr, numMwmIds)
    , m_trafficStash(std::move(trafficStash))
  {}

  // EdgeEstimator overrides:
  double CalcSegmentWeight(Segment const & segment, RoadGeometry const & road, Purpose purpose,
                           time_t arrivalTime) const override;
  double GetUTurnPenalty(Purpose /* purpose */) const override
  {
    // Adds 2 minutes penalty for U-turn. The value is quite arbitrary
    // and needs to be properly selected after a number of real-world
    // experiments.
    return 2 * 60;  // seconds
  }

  double GetFerryLandingPenalty(Purpose purpose) const override
  {
    switch (purpose)
    {
    case Purpose::Weight: return 20 * 60;  // seconds
    case Purpose::ETA: return 20 * 60;     // seconds
    }
    UNREACHABLE();
  }

private:
  std::shared_ptr<TrafficStash> m_trafficStash;
};

double CarEstimator::CalcSegmentWeight(Segment const & segment, RoadGeometry const & road, Purpose purpose,
                                       time_t arrivalTime) const
{
  if (purpose == Purpose::Weight && GetStrategy() == Strategy::Shortest)
    return road.GetDistance(segment.GetSegmentIdx()) / GetMaxWeightSpeedMpS();

  double const speed = GetSpeedMpS(purpose, segment, road, arrivalTime);

  // Debug log ETA calculated speed.
#ifdef DEBUG
  if (purpose == Purpose::ETA && fabs(speed - m_lastETASpeed) > 0.25)  // diff >= 1km/h
  {
    LOG(LDEBUG, ("[ETA] speed =", speed * 3.6, "starting from:", road.GetPoint(segment.GetPointId(true /* front */))));
    m_lastETASpeed = speed;
  }
#endif

  double result = road.GetDistance(segment.GetSegmentIdx()) / speed;

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
      // Add some penalty to make estimation more realistic.
      /// @todo Make accurate tuning, remove penalty.
      result *= 1.8;
    }
  }

  return result;
}

// EdgeEstimator -----------------------------------------------------------------------------------
// static
std::shared_ptr<EdgeEstimator> EdgeEstimator::Create(VehicleType vehicleType, double maxWeighSpeedKMpH,
                                                     SpeedKMpH const & offroadSpeedKMpH,
                                                     std::shared_ptr<TrafficStash> trafficStash,
                                                     DataSource * dataSourcePtr, std::shared_ptr<NumMwmIds> numMwmIds)
{
  switch (vehicleType)
  {
  case VehicleType::Pedestrian:
  case VehicleType::Transit: return std::make_shared<PedestrianEstimator>(maxWeighSpeedKMpH, offroadSpeedKMpH);
  case VehicleType::Bicycle: return std::make_shared<BicycleEstimator>(maxWeighSpeedKMpH, offroadSpeedKMpH);
  case VehicleType::Car:
    return std::make_shared<CarEstimator>(dataSourcePtr, numMwmIds, trafficStash, maxWeighSpeedKMpH, offroadSpeedKMpH);
  case VehicleType::Count: CHECK(false, ("Can't create EdgeEstimator for", vehicleType)); return nullptr;
  }
  UNREACHABLE();
}

// static
std::shared_ptr<EdgeEstimator> EdgeEstimator::Create(VehicleType vehicleType,
                                                     VehicleModelInterface const & vehicleModel,
                                                     std::shared_ptr<TrafficStash> trafficStash,
                                                     DataSource * dataSourcePtr, std::shared_ptr<NumMwmIds> numMwmIds)
{
  return Create(vehicleType, vehicleModel.GetMaxWeightSpeed(), vehicleModel.GetOffroadSpeed(), trafficStash,
                dataSourcePtr, numMwmIds);
}
}  // namespace routing
