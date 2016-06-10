#include "routing/directions_engine.hpp"

#include "base/assert.hpp"

namespace
{
double constexpr KMPH2MPS = 1000.0 / (60 * 60);
}  // namespace

namespace routing
{
void IDirectionsEngine::CalculateTimes(IRoadGraph const & graph, vector<Junction> const & path,
                                       Route::TTimes & times) const
{
  times.clear();
  if (path.size() <= 1)
    return;

  // graph.GetMaxSpeedKMPH() below is used on purpose.
  // The idea is while pedestian (bicycle) routing ways for pedestrians (cyclists) are prefered.
  // At the same time routing along big roads is still possible but if there's
  // a pedestrian (bicycle) alternative it's prefered. To implement it a small speed
  // is set in pedestrian_model (bicycle_model) for big roads. On the other hand
  // the most likely a pedestrian (a cyclist) will go along big roads with average
  // speed (graph.GetMaxSpeedKMPH()).
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

bool IDirectionsEngine::ReconstructPath(IRoadGraph const & graph, vector<Junction> const & path,
                                        vector<Edge> & routeEdges,
                                        my::Cancellable const & cancellable) const
{
  routeEdges.clear();
  if (path.size() <= 1)
    return false;

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
      return false;

    curr = next;
  }

  ASSERT_EQUAL(routeEdges.size() + 1, path.size(), ());

  return true;
}
}  // namespace routing
