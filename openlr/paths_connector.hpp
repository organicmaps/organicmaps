#pragma once

#include "openlr/graph.hpp"
#include "openlr/openlr_model.hpp"
#include "openlr/stats.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace openlr
{
class PathsConnector
{
public:
  PathsConnector(double const pathLengthTolerance, Graph const & graph, v2::Stats & stat);

  bool ConnectCandidates(std::vector<LocationReferencePoint> const & points,
                         std::vector<std::vector<Graph::EdgeVector>> const & lineCandidates,
                         std::vector<Graph::EdgeVector> & resultPath);

private:
  bool FindShortestPath(Graph::Edge const & from, Graph::Edge const & to,
                        FunctionalRoadClass const frc, uint32_t const maxPathLength,
                        Graph::EdgeVector & path);

  bool ConnectAdjacentCandidateLines(Graph::EdgeVector const & from, Graph::EdgeVector const & to,
                                     FunctionalRoadClass const frc,
                                     uint32_t const distanceToNextPoint,
                                     Graph::EdgeVector & resultPath);

  double const m_pathLengthTolerance;
  Graph const & m_graph;
  v2::Stats & m_stat;
};
}  // namespace openlr
