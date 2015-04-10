#include "dijkstra_router.hpp"

#include "../indexer/mercator.hpp"

#include "../base/logging.hpp"

#include "../std/algorithm.hpp"
#include "../std/map.hpp"
#include "../std/queue.hpp"

namespace routing
{
int const kCancelledPollPeriod = 100;

namespace
{
template <typename E>
void SortUnique(vector<E> & v)
{
  sort(v.begin(), v.end());
  v.resize(unique(v.begin(), v.end()) - v.begin());
}

template <typename E>
bool Contains(vector<E> const & v, E const & e)
{
  return binary_search(v.begin(), v.end(), e);
}

void ReconstructPath(RoadPos const & v, map<RoadPos, RoadPos> const & parent,
                     vector<RoadPos> & route)
{
  route.clear();
  RoadPos cur = v;
  while (true)
  {
    route.push_back(cur);
    auto it = parent.find(cur);
    if (it == parent.end())
      break;
    cur = it->second;
  }
}

struct Vertex
{
  Vertex(RoadPos const & pos, double dist) : pos(pos), dist(dist) {}

  bool operator<(Vertex const & v) const { return dist > v.dist; }

  RoadPos pos;
  double dist;
};
}  // namespace

IRouter::ResultCode DijkstraRouter::CalculateRouteM2M(vector<RoadPos> const & startPos,
                                                      vector<RoadPos> const & finalPos,
                                                      vector<RoadPos> & route)
{
  priority_queue<Vertex> queue;

  // Upper bound on the distance from start positions to current vertex.
  map<RoadPos, double> dist;

  // Parent in the search tree.
  map<RoadPos, RoadPos> parent;

  vector<uint32_t> sortedStartFeatures(startPos.size());
  for (size_t i = 0; i < startPos.size(); ++i)
    sortedStartFeatures[i] = startPos[i].GetFeatureId();
  SortUnique(sortedStartFeatures);

  vector<RoadPos> sortedStartPos(startPos.begin(), startPos.end());
  SortUnique(sortedStartPos);

  for (RoadPos const & fp : finalPos)
    dist[fp] = 0.0;
  for (auto const & p : dist)
    queue.push(Vertex(p.first, 0.0 /* distance */));

  int steps = 0;

  while (!queue.empty())
  {
    ++steps;
    if (steps % kCancelledPollPeriod == 0)
    {
      if (IsCancelled())
        return IRouter::Cancelled;
    }

    Vertex const v = queue.top();
    queue.pop();

    if (v.dist > dist[v.pos])
      continue;

    if (Contains(sortedStartFeatures, v.pos.GetFeatureId()) && Contains(sortedStartPos, v.pos))
    {
      ReconstructPath(v.pos, parent, route);
      return IRouter::NoError;
    }
    IRoadGraph::TurnsVectorT turns;
    m_roadGraph->GetNearestTurns(v.pos, turns);
    for (PossibleTurn const & turn : turns)
    {
      RoadPos const & pos = turn.m_pos;
      if (v.pos == pos)
        continue;
      // TODO (@gorshenin): distance must be measured in time units,
      // so it possibly worth to divide distance by the pedestrian's
      // speed.
      double const d =
          v.dist + MercatorBounds::DistanceOnEarth(v.pos.GetSegEndpoint(), pos.GetSegEndpoint());
      auto it = dist.find(pos);
      if (it == dist.end() || d < it->second)
      {
        dist[pos] = d;
        parent[pos] = v.pos;
        queue.push(Vertex(pos, d));
      }
    }
  }
  return IRouter::RouteNotFound;
}
}  // namespace routing
