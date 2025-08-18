#pragma once

#include "openlr/graph.hpp"
#include "openlr/openlr_model.hpp"
#include "openlr/road_info_getter.hpp"
#include "openlr/stats.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace openlr
{
class PathsConnector
{
public:
  PathsConnector(double pathLengthTolerance, Graph & graph, RoadInfoGetter & infoGetter, v2::Stats & stat);

  bool ConnectCandidates(std::vector<LocationReferencePoint> const & points,
                         std::vector<std::vector<Graph::EdgeVector>> const & lineCandidates,
                         std::vector<Graph::EdgeVector> & resultPath);

private:
  bool FindShortestPath(Graph::Edge const & from, Graph::Edge const & to, FunctionalRoadClass lowestFrcToNextPoint,
                        uint32_t maxPathLength, Graph::EdgeVector & path);

  bool ConnectAdjacentCandidateLines(Graph::EdgeVector const & from, Graph::EdgeVector const & to,
                                     FunctionalRoadClass lowestFrcToNextPoint, double distanceToNextPoint,
                                     Graph::EdgeVector & resultPath);

  double m_pathLengthTolerance;
  Graph & m_graph;
  RoadInfoGetter & m_infoGetter;
  v2::Stats & m_stat;
};
}  // namespace openlr
