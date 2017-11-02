#include "routing/pedestrian_directions.hpp"

#include "routing/road_graph.hpp"
#include "routing/routing_helpers.hpp"

#include "indexer/classificator.hpp"
#include "indexer/feature.hpp"
#include "indexer/ftypes_matcher.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"

#include <utility>

using namespace std;

namespace
{
bool HasType(uint32_t type, feature::TypesHolder const & types)
{
  for (uint32_t t : types)
  {
    t = ftypes::BaseChecker::PrepareToMatch(t, 2);
    if (type == t)
      return true;
  }
  return false;
}
}  // namespace

namespace routing
{

PedestrianDirectionsEngine::PedestrianDirectionsEngine(shared_ptr<NumMwmIds> numMwmIds)
  : m_typeSteps(classif().GetTypeByPath({"highway", "steps"}))
  , m_typeLiftGate(classif().GetTypeByPath({"barrier", "lift_gate"}))
  , m_typeGate(classif().GetTypeByPath({"barrier", "gate"}))
  , m_numMwmIds(move(numMwmIds))
{
}

bool PedestrianDirectionsEngine::Generate(RoadGraphBase const & graph,
                                          vector<Junction> const & path,
                                          my::Cancellable const & cancellable,
                                          Route::TTurns & turns, Route::TStreets & streetNames,
                                          vector<Junction> & routeGeometry,
                                          vector<Segment> & segments)
{
  turns.clear();
  streetNames.clear();
  segments.clear();
  routeGeometry = path;

  // Note. According to Route::IsValid() method route of zero or one point is invalid.
  if (path.size() < 1)
    return false;

  vector<Edge> routeEdges;
  if (!ReconstructPath(graph, path, routeEdges, cancellable))
  {
    LOG(LWARNING, ("Can't reconstruct path."));
    return false;
  }

  CalculateTurns(graph, routeEdges, turns, cancellable);

  if (graph.IsRouteSegmentsImplemented())
  {
    segments = graph.GetRouteSegments();
  }
  else
  {
    segments.reserve(routeEdges.size());
    for (Edge const & e : routeEdges)
      segments.push_back(ConvertEdgeToSegment(*m_numMwmIds, e));
  }

  return true;
}

void PedestrianDirectionsEngine::CalculateTurns(RoadGraphBase const & graph,
                                                vector<Edge> const & routeEdges,
                                                Route::TTurns & turns,
                                                my::Cancellable const & cancellable) const
{
  for (size_t i = 0; i < routeEdges.size(); ++i)
  {
    if (cancellable.IsCancelled())
      return;

    Edge const & edge = routeEdges[i];

    feature::TypesHolder types;
    graph.GetEdgeTypes(edge, types);

    if (HasType(m_typeSteps, types))
    {
      if (edge.IsForward())
        turns.emplace_back(i, turns::PedestrianDirection::Upstairs);
      else
        turns.emplace_back(i, turns::PedestrianDirection::Downstairs);
    }
    else
    {
      graph.GetJunctionTypes(edge.GetStartJunction(), types);

      if (HasType(m_typeLiftGate, types))
        turns.emplace_back(i, turns::PedestrianDirection::LiftGate);
      else if (HasType(m_typeGate, types))
        turns.emplace_back(i, turns::PedestrianDirection::Gate);
    }
  }

  // direction "arrival"
  // (index of last junction is the same as number of edges)
  turns.emplace_back(routeEdges.size(), turns::PedestrianDirection::ReachedYourDestination);
}
}  // namespace routing
