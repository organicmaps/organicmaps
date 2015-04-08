#pragma once

#include "road_graph_router.hpp"
#include "../std/map.hpp"
#include "../std/queue.hpp"


namespace routing
{

class AStarRouter : public RoadGraphRouter
{
  typedef RoadGraphRouter BaseT;
public:
  AStarRouter(Index const * pIndex = 0) : BaseT(pIndex) {}

  virtual string GetName() const { return "pedestrian"; }
  // TODO (Dragunov) ResultCode returning
  void CalculateM2MRoute(vector<RoadPos> const & startPos, vector<RoadPos> const & finalPos,
                         vector<RoadPos> & route) override;

protected:

  // Vertex is what is going to be put in the priority queue
  class Vertex
  {
  public:
    explicit Vertex(RoadPos pos, double dist = 0.0)
      : m_pos(pos), m_reducedDist(dist) {}

    bool operator < (Vertex const & v) const
    {
      return m_reducedDist > v.m_reducedDist;
    }

    RoadPos GetPos() const { return m_pos; }

    inline void SetReducedDist(double dist) { m_reducedDist = dist; }
    inline double GetReducedDist() const { return m_reducedDist; }

  private:
    RoadPos m_pos;
    double m_reducedDist;
  };

  double HeuristicCostEstimate(Vertex const & v, vector<RoadPos> const & goals);
  double DistanceBetween(Vertex const & v1, Vertex const & v2);
  void ReconstructRoute(RoadPos const & destination, vector<RoadPos> & route) const;

  typedef map<RoadPos, double> RoadPosToDoubleMapT;
  RoadPosToDoubleMapT m_bestDistance;

  typedef map<RoadPos, RoadPos> RoadPosParentMapT;
  RoadPosParentMapT m_parent;
};


}
