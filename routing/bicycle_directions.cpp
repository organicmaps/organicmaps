#include "routing/bicycle_directions.hpp"
#include "routing/router_delegate.hpp"
#include "routing/routing_result_graph.hpp"
#include "routing/turns_generator.hpp"

#include "geometry/point2d.hpp"

#include "indexer/index.hpp"

namespace
{
using namespace routing;
using namespace routing::turns;

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
    // Find all roads near junctionPoint.
    //
  }

  virtual double GetShortestPathLength() const override { return m_routeLength; }

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
