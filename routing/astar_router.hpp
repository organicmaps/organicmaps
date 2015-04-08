#pragma once

#include "road_graph_router.hpp"
#include "../std/queue.hpp"
#include "../std/map.hpp"

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

  // Vertex is what is going to be put in the priority queue
  class Vertex
  {
  public:
    explicit Vertex(RoadPos pos, Vertex const * parent = NULL, double dist = 0.0)
      : m_pos(pos), m_parent(parent), m_reducedDist(dist) {}

    bool operator < (Vertex const & v) const
    {
      return m_reducedDist > v.m_reducedDist;
    }

    RoadPos GetPos() const { return m_pos; }
    inline void SetParent(Vertex const * parent) const
    {
      ASSERT_NOT_EQUAL(this, parent, ());
      m_parent = parent;
    }
    inline Vertex const * GetParent() const { return m_parent; }

    inline void SetReducedDist(double dist) const { m_reducedDist = dist; }
    inline double GetReducedDist() const { return m_reducedDist; }

  private:
    RoadPos m_pos;
    mutable Vertex const * m_parent;
    mutable double m_reducedDist;
  };

  double HeuristicCostEstimate(Vertex const * v, vector<RoadPos> const & goals);
  double DistanceBetween(Vertex const * v1, Vertex const * v2);
  void ReconstructRoute(RoadPos const & destination, vector<RoadPos> & route) const;

  typedef priority_queue<Vertex> VertexQueueT;
  VertexQueueT m_queue;

  typedef map<RoadPos, double> RoadPosToDoubleMapT;
  RoadPosToDoubleMapT m_bestDistance;

  typedef map<RoadPos, RoadPos> RoadPosParentMapT;
  RoadPosParentMapT m_parent;
};


}
