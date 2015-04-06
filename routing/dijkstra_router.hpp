#pragma once

#include "road_graph_router.hpp"

#include "../geometry/point2d.hpp"

#include "../std/queue.hpp"
#include "../std/set.hpp"
#include "../std/vector.hpp"


namespace routing
{

class DijkstraRouter : public RoadGraphRouter
{
  typedef RoadGraphRouter BaseT;

public:
  DijkstraRouter(Index const * pIndex = 0) : BaseT(pIndex) {}

  virtual string GetName() const { return "routeme"; }
  virtual void SetFinalRoadPos(vector<RoadPos> const & finalPos);
  virtual void CalculateRoute(vector<RoadPos> const & startPos, vector<RoadPos> & route);

private:
  class ShortestPath
  {
  public:
    explicit ShortestPath(RoadPos pos, ShortestPath const * pParentEntry = NULL)
      : m_pos(pos), m_pParentEntry(pParentEntry) {}

    bool operator < (ShortestPath const & e) const
    {
      return m_pos < e.m_pos;
    }

    RoadPos GetPos() const { return m_pos; }
    ShortestPath const * GetParentEntry() const { return m_pParentEntry; }
    bool IsVisited() const { return m_pParentEntry != NULL; }
    void SetParentAndMarkVisited(ShortestPath const * pParentEntry) const { m_pParentEntry = pParentEntry; }

    static ShortestPath const * const FINAL_POS;

  private:
    RoadPos m_pos;
    mutable ShortestPath const * m_pParentEntry;
  };

  struct PossiblePath
  {
    double m_cost;
    ShortestPath const * m_pEntry;
    ShortestPath const * m_pParentEntry;

    PossiblePath(double cost, ShortestPath const * pEntry, ShortestPath const * pParentEntry)
      : m_cost(cost), m_pEntry(pEntry), m_pParentEntry(pParentEntry) {}

    bool operator < (PossiblePath const & p) const
    {
      if (m_cost != p.m_cost)
        return m_cost > p.m_cost;

      // Comparisons below are used to make the algorithm stable.
      if (m_pEntry->GetPos() < p.m_pEntry->GetPos())
        return true;
      if (p.m_pEntry->GetPos() < m_pEntry->GetPos())
        return false;
      if (!m_pParentEntry && p.m_pParentEntry)
        return true;
      if (!p.m_pParentEntry && m_pParentEntry)
        return false;
      if (!m_pParentEntry && !p.m_pParentEntry)
      {
        if (m_pParentEntry->GetPos() < p.m_pParentEntry->GetPos())
          return true;
        if (p.m_pParentEntry->GetPos() < m_pParentEntry->GetPos())
          return false;
      }

      return false;
    }
  };

  typedef set<ShortestPath> PathSet;
  PathSet m_entries;

  typedef priority_queue<PossiblePath> PossiblePathQueue;
  PossiblePathQueue m_queue;
};

} // namespace routing
