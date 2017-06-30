#include "routing/directions_engine.hpp"

#include "geometry/mercator.hpp"

#include "base/assert.hpp"

namespace routing
{
bool IDirectionsEngine::ReconstructPath(RoadGraphBase const & graph, vector<Junction> const & path,
                                        vector<Edge> & routeEdges,
                                        my::Cancellable const & cancellable) const
{
  routeEdges.clear();
  if (path.size() <= 1)
    return false;

  if (graph.IsRouteEdgesImplemented())
  {
    graph.GetRouteEdges(routeEdges);
    ASSERT_EQUAL(routeEdges.size() + 1, path.size(), ());
    return true;
  }

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
    for (auto const & e : currEdges)
    {
      if (AlmostEqualAbs(e.GetEndJunction(), next))
      {
        routeEdges.emplace_back(e);
        found = true;
        break;
      }
    }

    if (!found)
    {
      LOG(LERROR, ("Can't find next edge, curr:", MercatorBounds::ToLatLon(curr.GetPoint()),
                   ", next:", MercatorBounds::ToLatLon(next.GetPoint()), ", edges size:",
                   currEdges.size(), ", i:", i));
      return false;
    }

    curr = next;
  }

  ASSERT_EQUAL(routeEdges.size() + 1, path.size(), ());

  return true;
}
}  // namespace routing
