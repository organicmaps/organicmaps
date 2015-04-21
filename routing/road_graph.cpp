#include "routing/road_graph.hpp"
#include "routing/road_graph_router.hpp"

#include "routing/route.hpp"

#include "indexer/mercator.hpp"

#include "geometry/distance_on_sphere.hpp"

#include "base/assert.hpp"

#include "std/limits.hpp"
#include "std/sstream.hpp"

namespace routing
{
namespace
{
double const KMPH2MPS = 1000.0 / (60 * 60);
double const MAX_SPEED_MPS = 5000.0 / (60 * 60);

double CalcDistanceMeters(m2::PointD const & p1, m2::PointD const & p2)
{
  return MercatorBounds::DistanceOnEarth(p1, p2);
}

double TimeBetweenSec(m2::PointD const & p1, m2::PointD const & p2)
{
  return CalcDistanceMeters(p1, p2) / MAX_SPEED_MPS;
}
}  // namespace

RoadPos::RoadPos(uint32_t featureId, bool forward, size_t segId, m2::PointD const & p)
    : m_featureId((featureId << 1) + (forward ? 1 : 0)), m_segId(segId), m_segEndpoint(p)
{
  ASSERT_LESS(featureId, 1U << 31, ());
  ASSERT_LESS(segId, numeric_limits<uint32_t>::max(), ());
}

string DebugPrint(RoadPos const & r)
{
  ostringstream ss;
  ss << "{ featureId: " << r.GetFeatureId() << ", isForward: " << r.IsForward()
     << ", segId:" << r.m_segId << ", segEndpoint:" << DebugPrint(r.m_segEndpoint) << "}";
  return ss.str();
}

// IRoadGraph ------------------------------------------------------------------

IRoadGraph::CrossTurnsLoader::CrossTurnsLoader(m2::PointD const & cross, TurnsVectorT & turns)
    : m_cross(cross), m_turns(turns)
{
}

void IRoadGraph::CrossTurnsLoader::operator()(uint32_t featureId, RoadInfo const & roadInfo)
{
  size_t const numPoints = roadInfo.m_points.size();

  if (!numPoints)
    return;

  PossibleTurn turn;
  turn.m_speedKMPH = roadInfo.m_speedKMPH;
  turn.m_startPoint = roadInfo.m_points[0];
  turn.m_endPoint = roadInfo.m_points[numPoints - 1];

  for (size_t i = 0; i < numPoints; ++i)
  {
    m2::PointD const & p = roadInfo.m_points[i];

    /// @todo Is this a correct way to compare?
    if (!m2::AlmostEqual(m_cross, p))
      continue;

    if (i > 0)
    {
      turn.m_pos = RoadPos(featureId, true /* forward */, i - 1, p);
      m_turns.push_back(turn);
    }

    if (i < numPoints - 1)
    {
      turn.m_pos = RoadPos(featureId, false /* forward */, i, p);
      m_turns.push_back(turn);
    }
  }
}

void IRoadGraph::ReconstructPath(RoadPosVectorT const & positions, Route & route)
{
  CHECK(!positions.empty(), ("Can't reconstruct path from an empty list of positions."));

  vector<m2::PointD> path;
  path.reserve(positions.size());

  double trackTimeSec = 0.0;
  m2::PointD prevPoint = positions[0].GetSegEndpoint();
  path.push_back(prevPoint);
  for (size_t i = 1; i < positions.size(); ++i)
  {
    m2::PointD curPoint = positions[i].GetSegEndpoint();

    // By some reason there're two adjacent positions on a road with
    // the same end-points. This could happen, for example, when
    // direction on a road was changed.  But it doesn't matter since
    // this code reconstructs only geometry of a route.
    if (curPoint == prevPoint)
      continue;

    path.push_back(curPoint);

    double const lengthM = CalcDistanceMeters(prevPoint, curPoint);
    trackTimeSec += lengthM / (GetSpeedKMPH(positions[i - 1].GetFeatureId()) * KMPH2MPS);
    prevPoint = curPoint;
  }

  if (path.size() == 1)
  {
    m2::PointD point = path.front();
    path.push_back(point);
  }

  ASSERT_GREATER_OR_EQUAL(path.size(), 2, ());

  // TODO: investigate whether it worth to reconstruct detailed turns
  // and times.
  Route::TimesT times;
  times.emplace_back(path.size() - 1, trackTimeSec);

  Route::TurnsT turnsDir;
  turnsDir.emplace_back(path.size() - 1, turns::ReachedYourDestination);

  route.SetGeometry(path.begin(), path.end());
  route.SetTurnInstructions(turnsDir);
  route.SetSectionTimes(times);
}

void IRoadGraph::GetNearestTurns(RoadPos const & pos, TurnsVectorT & turns)
{
  uint32_t const featureId = pos.GetFeatureId();

  if (featureId == RoadPos::kFakeStartFeatureId)
  {
    turns.insert(turns.end(), m_startVicinityTurns.begin(), m_startVicinityTurns.end());
    return;
  }
  if (featureId == RoadPos::kFakeFinalFeatureId)
  {
    turns.insert(turns.end(), m_finalVicinityTurns.begin(), m_finalVicinityTurns.end());
    return;
  }

  RoadInfo const roadInfo = GetRoadInfo(featureId);

  if (roadInfo.m_speedKMPH <= 0.0)
    return;

  ASSERT_GREATER_OR_EQUAL(roadInfo.m_points.size(), 2,
                          ("Incorrect road - only", roadInfo.m_points.size(), "point(s)."));

  m2::PointD const & cross = roadInfo.m_points[pos.GetSegStartPointId()];

  CrossTurnsLoader loader(cross, turns);
  ForEachClosestToCrossFeature(cross, loader);
}

void IRoadGraph::SetFakeTurns(RoadPos const & rp, vector<RoadPos> const & vicinity)
{
  vector<PossibleTurn> * turns = nullptr;
  uint32_t const featureId = rp.GetFeatureId();
  switch (featureId)
  {
    case RoadPos::kFakeStartFeatureId:
      turns = &m_startVicinityTurns;
      break;
    case RoadPos::kFakeFinalFeatureId:
      turns = &m_finalVicinityTurns;
      break;
    default:
      CHECK(false, ("Can't add fake turns from a valid road:", featureId));
  }
  turns->clear();
  for (RoadPos const & vrp : vicinity)
  {
    PossibleTurn turn;
    turn.m_pos = vrp;
    /// @todo Do we need other fields? Do we even need m_secondsCovered?
    turn.m_secondsCovered = TimeBetweenSec(rp.GetSegEndpoint(), vrp.GetSegEndpoint());
    turns->push_back(turn);
  }
}

// RoadGraph -------------------------------------------------------------------

RoadGraph::RoadGraph(IRoadGraph & roadGraph) : m_roadGraph(roadGraph) {}

void RoadGraph::GetAdjacencyListImpl(RoadPos const & v, vector<RoadEdge> & adj) const
{
  IRoadGraph::TurnsVectorT turns;
  m_roadGraph.GetNearestTurns(v, turns);
  for (PossibleTurn const & turn : turns)
  {
    RoadPos const & w = turn.m_pos;
    adj.emplace_back(w, TimeBetweenSec(v.GetSegEndpoint(), w.GetSegEndpoint()));
  }
}

double RoadGraph::HeuristicCostEstimateImpl(RoadPos const & v, RoadPos const & w) const
{
  return TimeBetweenSec(v.GetSegEndpoint(), w.GetSegEndpoint());
}

}  // namespace routing
