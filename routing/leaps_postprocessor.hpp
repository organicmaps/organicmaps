#pragma once

#include "routing/base/bfs.hpp"

#include "routing/index_graph_starter.hpp"
#include "routing/segment.hpp"

#include "base/math.hpp"

#include <cstdint>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace routing
{
/// \brief After LeapsOnly mode we can have Loops in the path, like this:
/// Path through features: {f0, f1, ..., f9}.
/// *-f0->*-f1->*-f2->*-f3->  *  -f6->*-f7->*-f8->*-f9->
///                         |   ↑
///                        f4  f5
///                         ↓   |
///                           *   <------- mwm exit.
///
/// As you can see, we can go from f3 to f6 but because of heuristic searching of intermediate
/// points (mwms' exists) in LeapsOnly routing algorithm, we build route through this point
/// and received next path:
/// {..., f3, f4, f5, f6, ... } - this is Loop.
/// So next class provides local optimization and relax such paths:
/// (... f3 -> f4 -> f5 -> f6 ...) to (... f3 -> f6 ...).
///
/// Works for: |kMaxStep| * N. Where N it is path's length.
/// Thus it is O(N), but we should not increase |kMaxStep| much.
class LeapsPostProcessor
{
public:
  LeapsPostProcessor(std::vector<Segment> const & path, IndexGraphStarter & starter);

  struct PathInterval
  {
    static double constexpr kWeightEps = 1.0;

    PathInterval(double weight, size_t left, size_t right, std::vector<Segment> && path);

    struct GreaterByWeight
    {
      bool operator()(PathInterval const & lhs, PathInterval const & rhs) const;
    };

    bool operator<(PathInterval const & rhs) const;

    double m_winWeight = 0.0;
    size_t m_left = 0;
    size_t m_right = 0;
    std::vector<Segment> m_path;
  };

  std::vector<Segment> GetProcessedPath();

private:
  static size_t const kMaxStep;

  struct SegmentData
  {
    SegmentData() = default;
    SegmentData(size_t steps, double eta);
    size_t m_steps = 0;
    double m_summaryETA = 0.0;
  };

  void Init();

  std::set<PathInterval, PathInterval::GreaterByWeight> CalculateIntervalsToRelax();
  void FillIngoingPaths(Segment const & start, std::map<Segment, SegmentData> & segmentsData);

  std::vector<Segment> m_path;
  IndexGraphStarter & m_starter;

  BFS<IndexGraphStarter> m_bfs;
  std::map<Segment, size_t> m_segmentToIndex;
  std::vector<double> m_prefixSumETA;
};

std::string DebugPrint(LeapsPostProcessor::PathInterval const & interval);
}  // namespace routing
