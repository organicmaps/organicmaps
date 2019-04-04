#include "openlr/helpers.hpp"

#include "openlr/road_info_getter.hpp"

#include "routing/features_road_graph.hpp"

#include "geometry/mercator.hpp"

#include <algorithm>
#include <sstream>
#include <string>
#include <type_traits>

namespace
{
using namespace openlr;
using namespace std;

openlr::FunctionalRoadClass HighwayClassToFunctionalRoadClass(ftypes::HighwayClass const & hwClass)
{
  switch (hwClass)
  {
  case ftypes::HighwayClass::Trunk: return openlr::FunctionalRoadClass::FRC0;
  case ftypes::HighwayClass::Primary: return openlr::FunctionalRoadClass::FRC1;
  case ftypes::HighwayClass::Secondary: return openlr::FunctionalRoadClass::FRC2;
  case ftypes::HighwayClass::Tertiary: return openlr::FunctionalRoadClass::FRC3;
  case ftypes::HighwayClass::LivingStreet: return openlr::FunctionalRoadClass::FRC4;
  case ftypes::HighwayClass::Service: return openlr::FunctionalRoadClass::FRC5;
  default: return openlr::FunctionalRoadClass::FRC7;
  }
}

/// \returns true if edge |e| conforms |functionalRoadClass| and false otherwise.
/// \param score If returns true |score| are filled with an appropriate score.
bool ConformFrc(Graph::Edge const & e, FunctionalRoadClass functionalRoadClass,
                RoadInfoGetter & infoGetter, Score & score)
{
  CHECK(!e.IsFake(), ());
  Score constexpr kMaxScoreForFrc = 25;
  score = 0;

  if (functionalRoadClass == FunctionalRoadClass::NotAValue)
    return false;

  auto const hwClass = infoGetter.Get(e.GetFeatureId()).m_hwClass;

  switch (functionalRoadClass)
  {
  case FunctionalRoadClass::FRC0:
    score = kMaxScoreForFrc;
    // Note. HighwayClass::Trunk means mororway, motorway_link, trunk or trunk_link.
    return hwClass == ftypes::HighwayClass::Trunk;

  case FunctionalRoadClass::FRC1:
    score = kMaxScoreForFrc;
    return hwClass == ftypes::HighwayClass::Trunk || hwClass == ftypes::HighwayClass::Primary;

  case FunctionalRoadClass::FRC2:
  case FunctionalRoadClass::FRC3:
    if (hwClass == ftypes::HighwayClass::Secondary || hwClass == ftypes::HighwayClass::Tertiary)
      score = kMaxScoreForFrc;

    return hwClass == ftypes::HighwayClass::Primary || hwClass == ftypes::HighwayClass::Secondary ||
        hwClass == ftypes::HighwayClass::Tertiary ||
        hwClass == ftypes::HighwayClass::LivingStreet;

  case FunctionalRoadClass::FRC4:
    if (hwClass == ftypes::HighwayClass::LivingStreet || hwClass == ftypes::HighwayClass::Service)
      score = kMaxScoreForFrc;

    return hwClass == ftypes::HighwayClass::Tertiary ||
        hwClass == ftypes::HighwayClass::LivingStreet ||
        hwClass == ftypes::HighwayClass::Service;

  case FunctionalRoadClass::FRC5:
  case FunctionalRoadClass::FRC6:
  case FunctionalRoadClass::FRC7:
    score = kMaxScoreForFrc;
    return hwClass == ftypes::HighwayClass::LivingStreet ||
        hwClass == ftypes::HighwayClass::Service;

  case FunctionalRoadClass::NotAValue:
    UNREACHABLE();
  }
  UNREACHABLE();
}
}  // namespace

namespace openlr
{
// BearingPointsSelector ---------------------------------------------------------------------------
BearingPointsSelector::BearingPointsSelector(uint32_t bearDistM, bool isLastPoint)
    : m_bearDistM(bearDistM), m_isLastPoint(isLastPoint)
{
}

m2::PointD BearingPointsSelector::GetBearingStartPoint(Graph::Edge const & e) const
{
  return m_isLastPoint ? e.GetEndPoint() : e.GetStartPoint();
}

m2::PointD BearingPointsSelector::GetBearingEndPoint(Graph::Edge const & e, double const distanceM)
{
  if (distanceM < m_bearDistM && m_bearDistM <= distanceM + EdgeLength(e))
  {
    auto const edgeLen = EdgeLength(e);
    auto const edgeBearDist = min(m_bearDistM - distanceM, edgeLen);
    CHECK_LESS_OR_EQUAL(edgeBearDist, edgeLen, ());
    return m_isLastPoint ? PointAtSegmentM(e.GetEndPoint(), e.GetStartPoint(),
                                           static_cast<double>(edgeBearDist))
                         : PointAtSegmentM(e.GetStartPoint(), e.GetEndPoint(),
                                           static_cast<double>(edgeBearDist));
  }
  return m_isLastPoint ? e.GetStartPoint() : e.GetEndPoint();
}

bool PointsAreClose(m2::PointD const & p1, m2::PointD const & p2)
{
  double const kMwmRoadCrossingRadiusMeters = routing::GetRoadCrossingRadiusMeters();
  return MercatorBounds::DistanceOnEarth(p1, p2) < kMwmRoadCrossingRadiusMeters;
}

double EdgeLength(Graph::Edge const & e)
{
  return MercatorBounds::DistanceOnEarth(e.GetStartPoint(), e.GetEndPoint());
}

bool EdgesAreAlmostEqual(Graph::Edge const & e1, Graph::Edge const & e2)
{
  // TODO(mgsergio): Do I need to check fields other than points?
  return PointsAreClose(e1.GetStartPoint(), e2.GetStartPoint()) &&
         PointsAreClose(e1.GetEndPoint(), e2.GetEndPoint());
}

string LogAs2GisPath(Graph::EdgeVector const & path)
{
  CHECK(!path.empty(), ("Paths should not be empty"));

  ostringstream ost;
  ost << "https://2gis.ru/moscow?queryState=";

  auto ll = MercatorBounds::ToLatLon(path.front().GetStartPoint());
  ost << "center%2F" << ll.lon << "%2C" << ll.lat << "%2F";
  ost << "zoom%2F" << 17 << "%2F";
  ost << "ruler%2Fpoints%2F";
  for (auto const & e : path)
  {
    ll = MercatorBounds::ToLatLon(e.GetStartPoint());
    ost << ll.lon << "%20" << ll.lat << "%2C";
  }
  ll = MercatorBounds::ToLatLon(path.back().GetEndPoint());
  ost << ll.lon << "%20" << ll.lat;

  return ost.str();
}

string LogAs2GisPath(Graph::Edge const & e) { return LogAs2GisPath(Graph::EdgeVector({e})); }

bool PassesRestriction(Graph::Edge const & e, FunctionalRoadClass restriction, FormOfWay fow,
                       int frcThreshold, RoadInfoGetter & infoGetter)
{
  if (e.IsFake() || restriction == FunctionalRoadClass::NotAValue)
    return true;

  auto const frc = HighwayClassToFunctionalRoadClass(infoGetter.Get(e.GetFeatureId()).m_hwClass);
  return static_cast<int>(frc) <= static_cast<int>(restriction) + frcThreshold;
}

bool PassesRestrictionV3(Graph::Edge const & e, FunctionalRoadClass restriction, FormOfWay fow,
                         RoadInfoGetter & infoGetter, Score & score)
{
  CHECK(!e.IsFake(), ("Edges should not be fake:", e));
  if (!ConformFrc(e, restriction, infoGetter, score))
    return false;

  Score constexpr kScoreForFow = 25; // Score for form of way.
  if (fow == FormOfWay::Roundabout && infoGetter.Get(e.GetFeatureId()).m_isRoundabout)
    score += kScoreForFow;

  return true;
}

bool ConformLfrcnp(Graph::Edge const & e, FunctionalRoadClass lowestFrcToNextPoint,
                   int frcThreshold, RoadInfoGetter & infoGetter)
{
  if (e.IsFake() || lowestFrcToNextPoint == FunctionalRoadClass::NotAValue)
    return true;

  auto const frc = HighwayClassToFunctionalRoadClass(infoGetter.Get(e.GetFeatureId()).m_hwClass);
  return static_cast<int>(frc) <= static_cast<int>(lowestFrcToNextPoint) + frcThreshold;
}

bool ConformLfrcnpV3(Graph::Edge const & e, FunctionalRoadClass lowestFrcToNextPoint,
                     RoadInfoGetter & infoGetter)
{
  Score score;
  return ConformFrc(e, lowestFrcToNextPoint, infoGetter, score);
}

size_t IntersectionLen(Graph::EdgeVector a, Graph::EdgeVector b)
{
  sort(a.begin(), a.end());
  sort(b.begin(), b.end());
  return set_intersection(a.begin(), a.end(), b.begin(), b.end(), CounterIterator()).GetCount();
}

bool PrefEqualsSuff(Graph::EdgeVector const & a, Graph::EdgeVector const & b, size_t len)
{
  CHECK_LESS_OR_EQUAL(len, a.size(), ());
  CHECK_LESS_OR_EQUAL(len, b.size(), ());
  return equal(a.end() - len, a.end(), b.begin());
}

// Returns a length of the longest suffix of |a| that matches any prefix of |b|.
// Neither |a| nor |b| can contain several repetitions of any edge.
// Returns -1 if |a| intersection |b| is not equal to some suffix of |a| and some prefix of |b|.
int32_t PathOverlappingLen(Graph::EdgeVector const & a, Graph::EdgeVector const & b)
{
  auto const len = IntersectionLen(a, b);
  if (PrefEqualsSuff(a, b, len))
    return base::checked_cast<int32_t>(len);
  return -1;
}

m2::PointD PointAtSegmentM(m2::PointD const & p1, m2::PointD const & p2, double const distanceM)
{
  auto const v = p2 - p1;
  auto const l = v.Length();
  auto const L = MercatorBounds::DistanceOnEarth(p1, p2);
  auto const delta = distanceM * l / L;
  return PointAtSegment(p1, p2, delta);
}
}  // namespace openlr
