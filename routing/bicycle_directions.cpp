#include "routing/bicycle_directions.hpp"
#include "routing/car_model.hpp"
#include "routing/router_delegate.hpp"
#include "routing/routing_result_graph.hpp"
#include "routing/turns_generator.hpp"

#include "geometry/point2d.hpp"

#include "indexer/index.hpp"
#include "indexer/scales.hpp"

namespace
{
using namespace routing;
using namespace routing::turns;

/*!
 * \brief The Point2Geometry class is responsable for looking for all adjacent to junctionPoint
 * road network edges. Including the current edge.
 */
class Point2Geometry
{
  m2::PointD const m_junctionPoint, m_ingoingPoint;
  TGeomTurnCandidate & m_candidates; /*!< All road feature angles crosses |m_junctionPoint| point. */

public:
  Point2Geometry(m2::PointD const & junctionPoint, m2::PointD const & ingoingPoint,
                 TGeomTurnCandidate & candidates)
      : m_junctionPoint(junctionPoint), m_ingoingPoint(ingoingPoint), m_candidates(candidates)
  {
  }

  void operator()(FeatureType const & ft)
  {
    if (!CarModel::Instance().IsRoad(ft))
      return;
    ft.ParseGeometry(FeatureType::BEST_GEOMETRY);
    size_t const count = ft.GetPointsCount();
    ASSERT_GREATER(count, 1, ());

    for (size_t i = 0; i < count; ++i)
    {
      if (MercatorBounds::DistanceOnEarth(m_junctionPoint, ft.GetPoint(i)) <
          kFeaturesNearTurnMeters)
      {
        if (i > 0)
          m_candidates.push_back(my::RadToDeg(
              PiMinusTwoVectorsAngle(m_junctionPoint, m_ingoingPoint, ft.GetPoint(i - 1))));
        if (i < count - 1)
          m_candidates.push_back(my::RadToDeg(
              PiMinusTwoVectorsAngle(m_junctionPoint, m_ingoingPoint, ft.GetPoint(i + 1))));
        return;
      }
    }
  }

  DISALLOW_COPY_AND_MOVE(Point2Geometry);
};

/*!
 * \brief GetTurnGeometry looks for all the road network edges near ingoingPoint.
 * GetTurnGeometry fills candidates with angles of all the incoming and outgoint segments.
 * \warning GetTurnGeometry should be used carefully because it's a time-consuming function.
 * \warning In multilevel crossroads there is an insignificant possibility that candidates
 * is filled with redundant segments of roads of different levels.
 */
void GetTurnGeometry(m2::PointD const & junctionPoint, m2::PointD const & ingoingPoint,
                     TGeomTurnCandidate & candidates, Index::MwmId const & mwmId,
                     Index const & index)
{
  Point2Geometry getter(junctionPoint, ingoingPoint, candidates);
  index.ForEachInRectForMWM(
      getter, MercatorBounds::RectByCenterXYAndSizeInMeters(junctionPoint, kFeaturesNearTurnMeters),
      scales::GetUpperScale(), mwmId);
}

class AStarRoutingResultGraph : public IRoutingResultGraph
{
public:
  virtual vector<LoadedPathSegment> const & GetSegments() const override
  {
    return vector<LoadedPathSegment>();
  }

  virtual void GetPossibleTurns(TNodeId node, m2::PointD const & ingoingPoint,
                                m2::PointD const & junctionPoint,
                                size_t & ingoingCount,
                                TTurnCandidates & outgoingTurns) const override
  {
    if (node >= m_routeEdges.size())
    {
      ASSERT(false, ());
      return;
    }
    // TODO(bykoianko) Calculate correctly ingoingCount. For the time being
    // any juction with one outgoing way would not be considered as a turn.
    ingoingCount = 0;
    // Find all roads near junctionPoint using GetTurnGeometry()
    //
  }

  virtual double GetPathLength() const override { return m_routeLength; }

  virtual m2::PointD const & GetStartPoint() const override
  {
    CHECK(!m_routeEdges.empty(), ());
    return m_routeEdges.front().GetStartJunction().GetPoint();
  }

  virtual m2::PointD const & GetEndPoint() const override
  {
    CHECK(!m_routeEdges.empty(), ());
    return m_routeEdges.back().GetEndJunction().GetPoint();
  }

  AStarRoutingResultGraph(/*Index const & index,*/ vector<Edge> const & routeEdges)
    : // m_index(index),
      m_routeEdges(routeEdges), m_routeLength(0)
  {
    // Calculate length of routeEdges |m_routeLength|
  }

  ~AStarRoutingResultGraph() {}

private:
//   Index const & m_index;
   vector<Edge> const & m_routeEdges;
   double m_routeLength;
};
}  // namespace

namespace routing
{
BicycleDirectionsEngine::BicycleDirectionsEngine()
{
}

void BicycleDirectionsEngine::Generate(IRoadGraph const & graph, vector<Junction> const & path,
                                       Route::TTimes & times,
                                       Route::TTurns & turnsDir,
                                       my::Cancellable const & cancellable)
{
  CHECK_GREATER(path.size(), 1, ());

  CalculateTimes(graph, path, times);

  vector<Edge> routeEdges;
  if (!ReconstructPath(graph, path, routeEdges, cancellable))
  {
    LOG(LDEBUG, ("Couldn't reconstruct path"));
    // use only "arrival" direction
    turnsDir.emplace_back(path.size() - 1, turns::TurnDirection::ReachedYourDestination);
    return;
  }
  if (routeEdges.empty())
  {
    ASSERT(false, ());
    turnsDir.emplace_back(path.size() - 1, turns::TurnDirection::ReachedYourDestination);
    return;
  }

  AStarRoutingResultGraph resultGraph(routeEdges);
  RouterDelegate delegate;
  vector<m2::PointD> routePoints;
  Route::TTimes turnAnnotationTimes;
  Route::TStreets streetNames;
  MakeTurnAnnotation(resultGraph, delegate, routePoints, turnsDir, turnAnnotationTimes, streetNames);
}
}  // namespace routing
