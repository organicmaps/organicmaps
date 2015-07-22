#include "routing/pedestrian_turns_genetator.hpp"
#include "routing/turns_generator.hpp"

#include "indexer/classificator.hpp"
#include "indexer/feature.hpp"
#include "indexer/ftypes_matcher.hpp"
#include "indexer/index.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"

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
namespace turns
{

PedestrianTurnsGenerator::PedestrianTurnsGenerator()
  : m_typeSteps(classif().GetTypeByPath({"highway", "steps"}))
{
}

void PedestrianTurnsGenerator::Generate(IRoadGraph const & graph, vector<m2::PointD> const & path,
                                        Route::TTurns & turnsDir,
                                        TTurnsGeom & turnsGeom) const
{
  ASSERT_GREATER(path.size(), 1, ());

  vector<Edge> routeEdges;
  if (!ReconstructPath(graph, path, routeEdges))
  {
    LOG(LDEBUG, ("Couldn't reconstruct path"));
    // use only "arrival" direction
    turnsDir.emplace_back(path.size() - 1, turns::PedestrianDirection::ReachedYourDestination);
    return;
  }

  for (size_t i = 0; i < routeEdges.size(); ++i)
  {
    Edge const & edge = routeEdges[i];

    feature::TypesHolder types;
    graph.GetEdgeTypes(edge, types);

    if (HasType(m_typeSteps, types))
    {
      // direction "steps"
      if (edge.IsForward())
      {
        LOG(LDEBUG, ("Edge", i, edge.GetFeatureId(), "upstairs"));
        turnsDir.emplace_back(i, turns::PedestrianDirection::Upstairs);
      }
      else
      {
        LOG(LDEBUG, ("Edge", i, edge.GetFeatureId(), "downstairs"));
        turnsDir.emplace_back(i, turns::PedestrianDirection::Downstairs);
      }
    }
  }

  // direction "arrival"
  turnsDir.emplace_back(path.size() - 1, turns::PedestrianDirection::ReachedYourDestination);

  // Do not show arrows for pedestrian routing until a good design solution
  // CalculateTurnGeometry(path, turnsDir, turnsGeom);
  UNUSED_VALUE(turnsGeom);
}

bool PedestrianTurnsGenerator::ReconstructPath(IRoadGraph const & graph, vector<m2::PointD> const & path,
                                               vector<Edge> & routeEdges) const
{
  Junction curr = path[0];
  vector<Edge> currEdges;
  for (size_t i = 1; i < path.size(); ++i)
  {
    Junction const next = path[i];

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

}  // namespace turns
}  // namespace routing
