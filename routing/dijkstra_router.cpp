#include "dijkstra_router.hpp"
#include "../base/assert.hpp"
#include "../base/logging.hpp"
#include "../std/set.hpp"

namespace routing
{

DijkstraRouter::ShortestPath const * const DijkstraRouter::ShortestPath::FINAL_POS
  = reinterpret_cast<ShortestPath const *>(1);

void DijkstraRouter::SetFinalRoadPos(vector<RoadPos> const & finalPos)
{
  m_entries = PathSet();
  m_queue = PossiblePathQueue();

  for (size_t i = 0; i < finalPos.size(); ++i)
  {
    pair<PathSet::iterator, bool> t = m_entries.insert(ShortestPath(finalPos[i]));
    ASSERT(t.second, ());
    m_queue.push(PossiblePath(0.0, &*t.first, ShortestPath::FINAL_POS));
  }
}

void DijkstraRouter::CalculateRoute(vector<RoadPos> const & startPos, vector<RoadPos> & route)
{
  route.clear();
  set<RoadPos> startSet(startPos.begin(), startPos.end());
  set<uint32_t> startFeatureSet;
  for (vector<RoadPos>::const_iterator it = startPos.begin(); it != startPos.end(); ++it)
    startFeatureSet.insert(it->GetFeatureId());
  while (!m_queue.empty())
  {
    double const cost = m_queue.top().m_cost;
    ShortestPath const * const pEntry = m_queue.top().m_pEntry;
    ShortestPath const * const pParentEntry = m_queue.top().m_pParentEntry;
    m_queue.pop();

    LOG(LDEBUG, ("Visiting", pEntry->GetPos(), "with cost", cost));

    if (pEntry->IsVisited())
    {
      LOG(LDEBUG, ("Already visited before, skipping."));
      continue;
    }
    pEntry->SetParentAndMarkVisited(pParentEntry);

#ifdef DEBUG
    if (pParentEntry == ShortestPath::FINAL_POS)
    {
      LOG(LDEBUG, ("Setting parent to", "FINAL_POS"));
    }
    else
    {
      LOG(LDEBUG, ("Setting parent to", pParentEntry->GetPos()));
    }
#endif

    bool const bStartFeature = startFeatureSet.find(pEntry->GetPos().GetFeatureId()) != startFeatureSet.end();

    if (bStartFeature && startSet.find(pEntry->GetPos()) != startSet.end())
    {
      LOG(LDEBUG, ("Found result!"));
      // Reached one of the starting points!
      for (ShortestPath const * pE = pEntry; pE != ShortestPath::FINAL_POS; pE = pE->GetParentEntry())
        route.push_back(pE->GetPos());
      LOG(LDEBUG, (route));
      return;
    }

    IRoadGraph::TurnsVectorT turns;
    m_pRoadGraph->GetPossibleTurns(pEntry->GetPos(), turns, bStartFeature);
    LOG(LDEBUG, ("Getting all turns", turns));
    for (IRoadGraph::TurnsVectorT::const_iterator iTurn = turns.begin(); iTurn != turns.end(); ++iTurn)
    {
      PossibleTurn const & turn = *iTurn;
      pair<PathSet::iterator, bool> t = m_entries.insert(ShortestPath(turn.m_pos));
      if (t.second || !t.first->IsVisited())
      {
        // We've either never pushed or never poped this road before.
        double nextCost = cost + turn.m_secondsCovered;
        m_queue.push(PossiblePath(nextCost, &*t.first, pEntry));
        LOG(LDEBUG, ("Pushing", t.first->GetPos(), "with cost", nextCost));
      }
    }
  }

  LOG(LDEBUG, ("No route found!"));
  // Route not found.
}


} // namespace routing
