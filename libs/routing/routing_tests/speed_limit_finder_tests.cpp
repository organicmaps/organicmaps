#include "testing/testing.hpp"

#include "routing/speed_limit_finder.hpp"

#include "routing_common/maxspeed_conversion.hpp"

#include "platform/measurement_utils.hpp"

#include "geometry/mercator.hpp"

#include "base/math.hpp"

#include <cmath>
#include <optional>

namespace speed_limit_finder_tests
{
using namespace measurement_utils;
using namespace routing;

namespace
{
m2::PointD BearingDir(double bearingDeg)
{
  double const rad = math::DegToRad(bearingDeg);
  return {std::sin(rad), std::cos(rad)};
}
}  // namespace

UNIT_TEST(SpeedLimitToMps_ConversionsAndSpecialValues)
{
  // Invalid maxspeed -> -1.0.
  TEST_LESS(SpeedLimitToMps(Maxspeed(), true /* isForward */, 0), 0.0, ());

  // Metric single-direction speed applies to both directions.
  Maxspeed const metric50(Units::Metric, 50, kInvalidSpeed);
  TEST_ALMOST_EQUAL_ABS(SpeedLimitToMps(metric50, true, 0), KmphToMps(50.0), 1e-6, ());
  TEST_ALMOST_EQUAL_ABS(SpeedLimitToMps(metric50, false, 0), KmphToMps(50.0), 1e-6, ());

  // Bidirectional speeds select forward vs backward accurately.
  Maxspeed const bidir(Units::Metric, 80, 60);
  TEST_ALMOST_EQUAL_ABS(SpeedLimitToMps(bidir, true, 0), KmphToMps(80.0), 1e-6, ());
  TEST_ALMOST_EQUAL_ABS(SpeedLimitToMps(bidir, false, 0), KmphToMps(60.0), 1e-6, ());

  // Imperial speed preserves double precision for round-trip formatting.
  Maxspeed const mph65(Units::Imperial, 65, kInvalidSpeed);
  double const mph65Mps = SpeedLimitToMps(mph65, true, 0);
  TEST_ALMOST_EQUAL_ABS(KmphToMiph(MpsToKmph(mph65Mps)), 65.0, 1e-6, ());
  TEST_EQUAL(FormatSpeed(mph65Mps, Units::Imperial), 65, ());
  TEST_EQUAL(FormatSpeed(SpeedLimitToMps(metric50, true, 0), Units::Metric), 50, ());

  // Unlimited ("none") maps to 0.0, matching RoutingSession / FollowingInfo.
  Maxspeed const unlimited(Units::Metric, kNoneMaxSpeed, kInvalidSpeed);
  TEST_EQUAL(SpeedLimitToMps(unlimited, true, 0), 0.0, ());

  // Walk speed is non-numeric and maps to < 0.0.
  Maxspeed const walk(Units::Metric, kWalkMaxSpeed, kInvalidSpeed);
  TEST_LESS(SpeedLimitToMps(walk, true, 0), 0.0, ());
}

UNIT_TEST(MatchPointToSegment_DistanceThresholdAndDirection)
{
  // North-South road along lon = 10.0, from lat = 45.0000 to 45.0020.
  m2::PointD const p0 = mercator::FromLatLon(45.0000, 10.0000);
  m2::PointD const p1 = mercator::FromLatLon(45.0020, 10.0000);

  // Point ~10 m east of the road.
  m2::PointD const nearPt = mercator::FromLatLon(45.0010, 10.00013);
  auto const headingNorth = MatchPointToSegment(nearPt, p0, p1, BearingDir(0.0), false /* isOneWay */);
  TEST(headingNorth.has_value(), ());
  TEST(headingNorth->m_isForward, ());
  TEST_LESS(headingNorth->m_distanceMeters, 15.0, ());

  auto const headingSouth = MatchPointToSegment(nearPt, p0, p1, BearingDir(180.0), false /* isOneWay */);
  TEST(headingSouth.has_value(), ());
  TEST(!headingSouth->m_isForward, ());
  TEST_ALMOST_EQUAL_ABS(headingSouth->m_score, headingNorth->m_score, 1e-3, ());

  // Point ~50 m away exceeds 35 m threshold -> nullopt.
  m2::PointD const farPt = mercator::FromLatLon(45.0010, 10.00070);
  auto const farMatch = MatchPointToSegment(farPt, p0, p1, BearingDir(0.0), false /* isOneWay */);
  TEST(!farMatch.has_value(), ());
}

UNIT_TEST(MatchPointToSegment_IntersectionPrefersAlignedRoad)
{
  // North-South road (x = 10.0) and East-West cross street (y = 45.001).
  m2::PointD const ns0 = mercator::FromLatLon(45.0000, 10.0000);
  m2::PointD const ns1 = mercator::FromLatLon(45.0020, 10.0000);
  m2::PointD const ew0 = mercator::FromLatLon(45.0010, 9.9990);
  m2::PointD const ew1 = mercator::FromLatLon(45.0010, 10.0010);

  // Rider is 10 m off NS road and 4 m off EW cross street, heading North (0 deg).
  m2::PointD const rider = mercator::FromLatLon(45.001036, 10.00013);
  auto const nsMatch = MatchPointToSegment(rider, ns0, ns1, BearingDir(0.0), false);
  auto const ewMatch = MatchPointToSegment(rider, ew0, ew1, BearingDir(0.0), false);

  TEST(nsMatch.has_value() && ewMatch.has_value(), ());
  TEST_LESS(nsMatch->m_score, ewMatch->m_score, ());
}

UNIT_TEST(MatchPointToSegment_OneWayPrefersCorrectCarriageway)
{
  // Divided highway: northbound carriageway (east) and southbound carriageway (west).
  m2::PointD const nb0 = mercator::FromLatLon(45.0000, 10.00015);
  m2::PointD const nb1 = mercator::FromLatLon(45.0020, 10.00015);
  m2::PointD const sb0 = mercator::FromLatLon(45.0020, 10.00000);
  m2::PointD const sb1 = mercator::FromLatLon(45.0000, 10.00000);

  // GPS jitter puts rider slightly closer to southbound carriageway while actually heading North.
  m2::PointD const rider = mercator::FromLatLon(45.0010, 10.00005);
  auto const nbMatch = MatchPointToSegment(rider, nb0, nb1, BearingDir(0.0), true /* isOneWay */);
  auto const sbMatch = MatchPointToSegment(rider, sb0, sb1, BearingDir(0.0), true /* isOneWay */);

  TEST(nbMatch.has_value() && sbMatch.has_value(), ());
  TEST(nbMatch->m_isForward, ());
  TEST(!sbMatch->m_isForward, ());
  TEST_LESS(nbMatch->m_score, sbMatch->m_score, ());
}
}  // namespace speed_limit_finder_tests
