#pragma once

#include "road_graph_router.hpp"
#include "../std/queue.hpp"

namespace routing
{

class AStarRouter : public RoadGraphRouter
{
  typedef RoadGraphRouter BaseT;
public:
  AStarRouter(Index const * pIndex = 0) : BaseT(pIndex) {}

  virtual string GetName() const { return "routeme"; }
  virtual void SetFinalRoadPos(vector<RoadPos> const & finalPos);
  virtual void CalculateRoute(vector<RoadPos> const & startPos, vector<RoadPos> & route);

protected:

  class ShortestPath
  {
  public:
    explicit ShortestPath(RoadPos pos, ShortestPath const * pParentEntry = NULL, double gScore = 0.0)
      : m_pos(pos), m_pParentEntry(pParentEntry), m_gScore(gScore), m_isInOpenSet(false), m_isVisited(false) {}

    bool operator < (ShortestPath const & e) const
    {
      return m_pos < e.m_pos;
    }

    RoadPos GetPos() const { return m_pos; }
    ShortestPath const * GetParentEntry() const { return m_pParentEntry; }
    bool IsVisited() const { return m_isVisited; }
    void SetParent(ShortestPath const * pParentEntry) const
    {
      ASSERT_NOT_EQUAL(this, pParentEntry, ());
      m_pParentEntry = pParentEntry;
    }

    void SetVisited() const { m_isVisited = true; }

    void AppendedIntoOpenSet() const { m_isInOpenSet = true; }
    void RemovedFromOpenSet() const { m_isInOpenSet = false; }
    bool IsInOpenSet() const { return m_isInOpenSet; }

    inline void SetScoreG(double g) const { m_gScore = g; }
    inline double GetScoreG() const { return m_gScore; }

  private:
    RoadPos m_pos;
    mutable ShortestPath const * m_pParentEntry;
    mutable double m_gScore;
    mutable bool m_isInOpenSet;
    mutable bool m_isVisited;
  };

  struct PossiblePath
  {
    ShortestPath const * m_path;
    double m_fScore;

    explicit PossiblePath(ShortestPath const * path, double fScore = 0.0) : m_path(path), m_fScore(fScore) {}

    bool operator < (PossiblePath const & p) const
    {
      if (m_fScore != p.m_fScore)
        return m_fScore > p.m_fScore;

      if (m_path->GetScoreG() != p.m_path->GetScoreG())
        return m_path->GetScoreG() > p.m_path->GetScoreG();

      return !(m_path < p.m_path);
    }
  };

  double HeuristicCostEstimate(ShortestPath const * s1, set<RoadPos> const & goals);
  double DistanceBetween(ShortestPath const * p1, ShortestPath const * p2);
  void ReconstructRoute(ShortestPath const * path, vector<RoadPos> & route) const;

  typedef priority_queue<PossiblePath> PossiblePathQueueT;
  PossiblePathQueueT m_queue;

  typedef set<ShortestPath> ShortPathSetT;
  ShortPathSetT m_entries;
};


}
