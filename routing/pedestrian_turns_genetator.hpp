#pragma once

#include "routing/road_graph.hpp"
#include "routing/route.hpp"

#include "std/vector.hpp"

class Index;

namespace routing
{
namespace turns
{

class PedestrianTurnsGenerator
{
public:
  PedestrianTurnsGenerator();

  void Generate(IRoadGraph const & graph, vector<m2::PointD> const & path,
                Route::TTurns & turnsDir,
                TTurnsGeom & turnsGeom) const;

private:
  bool ReconstructPath(IRoadGraph const & graph, vector<m2::PointD> const & path,
                       vector<Edge> & routeEdges) const;

  uint32_t const m_typeSteps;
};

}  // namespace turns
}  // namespace routing
