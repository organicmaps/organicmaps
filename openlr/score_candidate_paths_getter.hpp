#pragma once

#include "openlr/graph.hpp"
#include "openlr/helpers.hpp"
#include "openlr/openlr_model.hpp"
#include "openlr/road_info_getter.hpp"
#include "openlr/score_types.hpp"
#include "openlr/stats.hpp"

#include "geometry/point2d.hpp"

#include "base/assert.hpp"

#include <cstdint>
#include <limits>
#include <memory>
#include <vector>

namespace openlr
{
class ScoreCandidatePointsGetter;

class ScoreCandidatePathsGetter
{
public:
  ScoreCandidatePathsGetter(ScoreCandidatePointsGetter & pointsGetter, Graph & graph, RoadInfoGetter & infoGetter,
                            v2::Stats & stat)
    : m_pointsGetter(pointsGetter)
    , m_graph(graph)
    , m_infoGetter(infoGetter)
    , m_stats(stat)
  {}

  bool GetLineCandidatesForPoints(std::vector<LocationReferencePoint> const & points, LinearSegmentSource source,
                                  std::vector<ScorePathVec> & lineCandidates);

private:
  struct Link
  {
    Link(std::shared_ptr<Link> const & parent, Graph::Edge const & edge, double distanceM, Score pointScore,
         Score rfcScore)
      : m_parent(parent)
      , m_edge(edge)
      , m_distanceM(distanceM)
      , m_pointScore(pointScore)
      , m_minRoadScore(rfcScore)
    {
      CHECK(!edge.IsFake(), ("Edge should not be fake:", edge));
    }

    bool IsJunctionInPath(geometry::PointWithAltitude const & j) const;

    Graph::Edge GetStartEdge() const;

    std::shared_ptr<Link> const m_parent;
    Graph::Edge const m_edge;
    double const m_distanceM;
    Score const m_pointScore;
    // Minimum score of segments of the path going along |m_parent| based on functional road class
    // and form of way.
    Score const m_minRoadScore;
  };

  struct CandidatePath
  {
    CandidatePath() = default;

    CandidatePath(std::shared_ptr<Link> const path, Score pointScore, Score rfcScore, Score bearingScore)
      : m_path(path)
      , m_pointScore(pointScore)
      , m_roadScore(rfcScore)
      , m_bearingScore(bearingScore)
    {}

    bool operator>(CandidatePath const & o) const { return GetScore() > o.GetScore(); }

    Score GetScore() const { return m_pointScore + m_roadScore + m_bearingScore; }

    std::shared_ptr<Link> m_path = nullptr;
    Score m_pointScore = 0;
    Score m_roadScore = 0;
    Score m_bearingScore = 0;
  };

  // Note: In all methods below if |isLastPoint| is true than algorithm should
  // calculate all parameters (such as bearing, distance to next point, etc.)
  // relative to the last point.
  // o ----> o ----> o <---- o.
  // 1       2       3       4
  //                         ^ isLastPoint = true.
  // To calculate bearing for points 1 to 3 one have to go beardist from
  // previous point to the next one (eg. from 1 to 2 and from 2 to 3).
  // For point 4 one have to go from 4 to 3 reversing directions. And
  // distance-to-next point is taken from point 3. You can learn more in
  // TomTom OpenLR spec.

  /// \brief Fills |allPaths| with paths near start or finish point starting from |startLines|.
  /// To extract a path from |allPaths| a item from |allPaths| should be taken,
  /// then should be taken the member |m_parent| of the item and so on till the beginning.
  void GetAllSuitablePaths(ScoreEdgeVec const & startLines, LinearSegmentSource source, bool isLastPoint,
                           double bearDistM, FunctionalRoadClass functionalRoadClass, FormOfWay formOfWay,
                           double distanceToNextPointM, std::vector<std::shared_ptr<Link>> & allPaths);

  void GetBestCandidatePaths(std::vector<std::shared_ptr<Link>> const & allPaths, LinearSegmentSource source,
                             bool isLastPoint, uint32_t requiredBearing, double bearDistM,
                             m2::PointD const & startPoint, ScorePathVec & candidates);

  void GetLineCandidates(openlr::LocationReferencePoint const & p, LinearSegmentSource source, bool isLastPoint,
                         double distanceToNextPointM, ScoreEdgeVec const & edgeCandidates, ScorePathVec & candidates);

  bool GetBearingScore(BearingPointsSelector const & pointsSelector, ScoreCandidatePathsGetter::Link const & part,
                       m2::PointD const & bearStartPoint, uint32_t requiredBearing, Score & score);

  ScoreCandidatePointsGetter & m_pointsGetter;
  Graph & m_graph;
  RoadInfoGetter & m_infoGetter;
  v2::Stats & m_stats;
};
}  // namespace openlr
