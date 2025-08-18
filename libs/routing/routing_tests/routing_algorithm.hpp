#pragma once

#include "routing/base/astar_graph.hpp"
#include "routing/base/routing_result.hpp"

#include "routing/road_graph.hpp"

#include "routing_common/vehicle_model.hpp"

#include "geometry/point_with_altitude.hpp"

#include <map>
#include <string>

namespace routing_test
{
using namespace routing;

struct SimpleEdge
{
  SimpleEdge() = default;  // needed for buffer_vector only
  SimpleEdge(uint32_t to, double weight) : m_to(to), m_weight(weight) {}

  uint32_t GetTarget() const { return m_to; }
  double GetWeight() const { return m_weight; }

  uint32_t m_to;
  double m_weight;
};

class RoadGraphIFace : public IRoadGraph
{
public:
  virtual RoadInfo GetRoadInfo(FeatureID const & f, SpeedParams const & speedParams) const = 0;
  virtual double GetSpeedKMpH(FeatureID const & featureId, SpeedParams const & speedParams) const = 0;
  virtual double GetMaxSpeedKMpH() const = 0;

  double GetSpeedKMpH(Edge const & edge, SpeedParams const & speedParams) const
  {
    double const speedKMpH = (edge.IsFake() ? GetMaxSpeedKMpH() : GetSpeedKMpH(edge.GetFeatureId(), speedParams));
    ASSERT_LESS_OR_EQUAL(speedKMpH, GetMaxSpeedKMpH(), ());
    return speedKMpH;
  }
};

class UndirectedGraph : public AStarGraph<uint32_t, SimpleEdge, double>
{
public:
  void AddEdge(Vertex u, Vertex v, Weight w);
  size_t GetNodesNumber() const;

  // AStarGraph overrides
  // @{
  void GetIngoingEdgesList(astar::VertexData<Vertex, Weight> const & vertexData, EdgeListT & adj) override;
  void GetOutgoingEdgesList(astar::VertexData<Vertex, Weight> const & vertexData, EdgeListT & adj) override;
  double HeuristicCostEstimate(Vertex const & v, Vertex const & w) override;
  // @}

  void GetEdgesList(Vertex const & vertex, bool /* isOutgoing */, EdgeListT & adj);

private:
  void GetAdjacencyList(Vertex v, EdgeListT & adj) const;

  std::map<uint32_t, EdgeListT> m_adjs;
};

class DirectedGraph
{
public:
  using Vertex = uint32_t;
  using Edge = SimpleEdge;
  using Weight = double;

  using EdgeListT = SmallList<SimpleEdge>;

  void AddEdge(Vertex from, Vertex to, Weight w);

  void GetEdgesList(Vertex const & v, bool isOutgoing, EdgeListT & adj);

private:
  std::map<uint32_t, EdgeListT> m_outgoing;
  std::map<uint32_t, EdgeListT> m_ingoing;
};

class TestAStarBidirectionalAlgo
{
public:
  enum class Result
  {
    OK,
    NoPath,
    Cancelled
  };

  Result CalculateRoute(RoadGraphIFace const & graph, geometry::PointWithAltitude const & startPos,
                        geometry::PointWithAltitude const & finalPos,
                        RoutingResult<IRoadGraph::Vertex, IRoadGraph::Weight> & path);
};

std::string DebugPrint(TestAStarBidirectionalAlgo::Result const & result);
}  // namespace routing_test
