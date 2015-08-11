#include "routing/pedestrian_directions.hpp"
#include "routing/turns_generator.hpp"

#include "indexer/classificator.hpp"
#include "indexer/feature.hpp"
#include "indexer/ftypes_matcher.hpp"
#include "indexer/index.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"

namespace
{
double constexpr KMPH2MPS = 1000.0 / (60 * 60);

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

PedestrianDirectionsEngine::PedestrianDirectionsEngine()
  : m_typeSteps(classif().GetTypeByPath({"highway", "steps"}))
  , m_typeLiftGate(classif().GetTypeByPath({"barrier", "lift_gate"}))
  , m_typeGate(classif().GetTypeByPath({"barrier", "gate"}))
{
}

void PedestrianDirectionsEngine::Generate(IRoadGraph const & graph, vector<Junction> const & path,
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
    turnsDir.emplace_back(path.size() - 1, turns::PedestrianDirection::ReachedYourDestination);
    return;
  }

  CalculateTurns(graph, routeEdges, turnsDir, cancellable);
}

bool PedestrianDirectionsEngine::ReconstructPath(IRoadGraph const & graph, vector<Junction> const & path,
                                                 vector<Edge> & routeEdges,
                                                 my::Cancellable const & cancellable) const
{
  routeEdges.reserve(path.size() - 1);

  Junction curr = path[0];
  vector<Edge> currEdges;
  for (size_t i = 1; i < path.size(); ++i)
  {
    if (cancellable.IsCancelled())
      return false;

    Junction const & next = path[i];

    currEdges.clear();
    graph.GetOutgoingEdges(curr, currEdges);

    bool found = false;
    for (Edge const & e : currEdges)
    {
      if (e.GetEndJunction() == next)
      {
        routeEdges.emplace_back(e);
        found = true;
        break;
      }
    }

    if (!found)
      return false;

    curr = next;
  }

  ASSERT_EQUAL(routeEdges.size()+1, path.size(), ());

  return true;
}

void PedestrianDirectionsEngine::CalculateTimes(IRoadGraph const & graph, vector<Junction> const & path,
                                                Route::TTimes & times) const
{
  double const speedMPS = graph.GetMaxSpeedKMPH() * KMPH2MPS;

  times.reserve(path.size());

  double trackTimeSec = 0.0;
  times.emplace_back(0, trackTimeSec);

  m2::PointD prev = path[0].GetPoint();
  for (size_t i = 1; i < path.size(); ++i)
  {
    m2::PointD const & curr = path[i].GetPoint();

    double const lengthM = MercatorBounds::DistanceOnEarth(prev, curr);
    trackTimeSec += lengthM / speedMPS;

    times.emplace_back(i, trackTimeSec);

    prev = curr;
  }
}

void PedestrianDirectionsEngine::CalculateTurns(IRoadGraph const & graph, vector<Edge> const & routeEdges,
                                                Route::TTurns & turnsDir,
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
        turnsDir.emplace_back(i, turns::PedestrianDirection::Upstairs);
      else
        turnsDir.emplace_back(i, turns::PedestrianDirection::Downstairs);
    }
    else
    {
      graph.GetJunctionTypes(edge.GetStartJunction(), types);

      if (HasType(m_typeLiftGate, types))
        turnsDir.emplace_back(i, turns::PedestrianDirection::LiftGate);
      else if (HasType(m_typeGate, types))
        turnsDir.emplace_back(i, turns::PedestrianDirection::Gate);
    }
  }

  // direction "arrival"
  // (index of last junction is the same as number of edges)
  turnsDir.emplace_back(routeEdges.size(), turns::PedestrianDirection::ReachedYourDestination);
}

}  // namespace routing
