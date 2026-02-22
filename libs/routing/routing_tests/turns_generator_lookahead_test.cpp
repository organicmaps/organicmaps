#include "testing/testing.hpp"

#include "routing/car_directions.hpp"
#include "routing/loaded_path_segment.hpp"
#include "routing/routing_result_graph.hpp"
#include "routing/routing_settings.hpp"
#include "routing/turns_generator.hpp"
#include "routing/turns_generator_utils.hpp"
#include "routing/vehicle_mask.hpp"

#include "indexer/ftypes_matcher.hpp"

#include "geometry/mercator.hpp"
#include "geometry/point2d.hpp"
#include "geometry/point_with_altitude.hpp"

#include "base/macros.hpp"

#include <vector>

namespace turns_generator_lookahead_test
{
using namespace routing;
using namespace std;
using namespace turns;

// Mock class similar to the one in turns_generator_test.cpp
class RoutingResultMock : public IRoutingResult
{
public:
  explicit RoutingResultMock(TUnpackedPathSegments const & segments) : m_segments(segments) {}

  TUnpackedPathSegments const & GetSegments() const override { return m_segments; }

  void GetPossibleTurns(SegmentRange const & segmentRange, m2::PointD const & junctionPoint, size_t & ingoingCount,
                        TurnCandidates & outgoingTurns) const override
  {
    outgoingTurns.candidates.emplace_back(0.0, Segment(), ftypes::HighwayClass::Tertiary, false);
    outgoingTurns.isCandidatesAngleValid = false;
  }

  double GetPathLength() const override { return 0.0; }
  geometry::PointWithAltitude GetStartPoint() const override { return {}; }
  geometry::PointWithAltitude GetEndPoint() const override { return {}; }

private:
  TUnpackedPathSegments m_segments;
};

UNIT_TEST(TestLookAheadForShortSegments)
{
  // Geometry setup:
  // 1. Ingoing segment: goes North to (0,0)
  // 2. Short segment: goes slight Right (NE) for 2 meters.
  // 3. Main outgoing segment: goes sharp Left (West) from there.

  m2::PointD const origin = mercator::FromLatLon(0, 0);

  // Ingoing: ~100 meters South (0.0009 degrees lat)
  m2::PointD const p1 = mercator::FromLatLon(-0.0009, 0.0);

  // Short segment: ~2 meters North-East (0.000013 degrees lat/lon)
  m2::PointD const p2 = mercator::FromLatLon(0.000013, 0.000013);

  // Long segment: ~50 meters West from p2 (subtract 0.000463 from lon)
  m2::PointD const p3 = mercator::FromLatLon(0.000013, -0.00045);

  // Construct segments
  TUnpackedPathSegments pathSegments;

  // Segment 0: p1 -> origin
  // Note: path points are normally stored locally but here we use global points.
  LoadedPathSegment seg0;
  seg0.m_path = {geometry::PointWithAltitude(p1, 0), geometry::PointWithAltitude(origin, 0)};
  seg0.m_highwayClass = ftypes::HighwayClass::Secondary;
  pathSegments.push_back(seg0);

  // Segment 1: origin -> p2 (Short)
  LoadedPathSegment seg1;
  seg1.m_path = {geometry::PointWithAltitude(origin, 0), geometry::PointWithAltitude(p2, 0)};
  seg1.m_highwayClass = ftypes::HighwayClass::Secondary;
  pathSegments.push_back(seg1);

  // Segment 2: p2 -> p3 (Long)
  LoadedPathSegment seg2;
  seg2.m_path = {geometry::PointWithAltitude(p2, 0), geometry::PointWithAltitude(p3, 0)};
  seg2.m_highwayClass = ftypes::HighwayClass::Secondary;
  pathSegments.push_back(seg2);

  RoutingResultMock result(pathSegments);
  NumMwmIds numMwmIds;  // Dummy

  // We are at the junction at the end of seg0. Outgoing segment starts at index 1.
  size_t const outgoingSegmentIndex = 1;

  // Test GetPointForTurn
  // We want to look forward.
  // Using parameters similar to car settings:
  // minIngoingDist = 100, maxIngoingPoints = 2
  // minOutgoingDist = 100, maxOutgoingPoints = 2 (in vehicleSettings)

  // But GetPointForTurn takes direct params.
  // kMinTurnPointDistMeters is 3.0 internally.

  size_t const maxPointsCount = 5;
  double const maxDistMeters = 20.0;
  bool const forward = true;

  m2::PointD const resultPoint =
      GetPointForTurn(result, outgoingSegmentIndex, numMwmIds, maxPointsCount, maxDistMeters, forward);

  // Expectation:
  // Since seg1 is 2.0m long < 3.0m, GetPointForTurn should treat it as "not far enough" and continue to seg2.
  // So it should return p3 (or a point on seg2).
  // resultPoint should be p3 because seg2 is long (50m) and loop will exit when cumulative dist > maxDistMeters (20m)
  // OR count >= maxPoints. Distance to p2 is 2m. Next point is p3. Distance p2->p3 is 50m. Total 52m. 52 > 3.0
  // (farEnough = true). 52 > 20.0 (condition -> return nextPoint). So it returns p3.

  // If lookahead failed (i.e. if it stopped at p2), equal to p2.

  TEST_NOT_EQUAL(resultPoint, p2, ("Should skip short segment point p2"));
  TEST(mercator::DistanceOnEarth(resultPoint, p3) < 1.0,
       ("Should reach p3, actual distance to p3:", mercator::DistanceOnEarth(resultPoint, p3)));

  // Additionally, verification of the Angle can be done via manual calculation or CalcTurnAngle if we could mock more.
  // But verifying the point is sufficient to prove lookahead.
}
}  // namespace turns_generator_lookahead_test
