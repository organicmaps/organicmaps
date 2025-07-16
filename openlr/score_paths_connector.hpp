#pragma once

#include "openlr/graph.hpp"
#include "openlr/openlr_model.hpp"
#include "openlr/road_info_getter.hpp"
#include "openlr/score_types.hpp"
#include "openlr/stats.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace openlr
{
class ScorePathsConnector
{
public:
  ScorePathsConnector(Graph & graph, RoadInfoGetter & infoGetter, v2::Stats & stat);

  /// \brief Connects |lineCandidates| and fills |resultPath| with the path with maximum score
  /// if there's a good enough.
  /// \returns true if the best path is found and false otherwise.
  bool FindBestPath(std::vector<LocationReferencePoint> const & points,
                    std::vector<std::vector<ScorePath>> const & lineCandidates, LinearSegmentSource source,
                    std::vector<Graph::EdgeVector> & resultPath);

private:
  bool FindShortestPath(Graph::Edge const & from, Graph::Edge const & to, LinearSegmentSource source,
                        FunctionalRoadClass lowestFrcToNextPoint, uint32_t maxPathLength, Graph::EdgeVector & path);

  bool ConnectAdjacentCandidateLines(Graph::EdgeVector const & from, Graph::EdgeVector const & to,
                                     LinearSegmentSource source, FunctionalRoadClass lowestFrcToNextPoint,
                                     double distanceToNextPoint, Graph::EdgeVector & resultPath);

  Score GetScoreForUniformity(Graph::EdgeVector const & path);

  Graph & m_graph;
  RoadInfoGetter & m_infoGetter;
  v2::Stats & m_stat;
};
}  // namespace openlr
