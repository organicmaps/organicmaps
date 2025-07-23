#include "routing/leaps_postprocessor.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"
#include "base/non_intersecting_intervals.hpp"
#include "base/scope_guard.hpp"

#include <algorithm>
#include <iterator>
#include <sstream>
#include <utility>

namespace
{
using namespace routing;

class NonIntersectingSegmentPaths
{
public:
  void AddInterval(LeapsPostProcessor::PathInterval const & pathInterval)
  {
    if (!m_nonIntersectingIntervals.AddInterval(pathInterval.m_left, pathInterval.m_right))
      return;

    m_intervals.emplace_back(pathInterval);
  }

  std::vector<LeapsPostProcessor::PathInterval> && StealIntervals() { return std::move(m_intervals); }

private:
  base::NonIntersectingIntervals<size_t> m_nonIntersectingIntervals;
  std::vector<LeapsPostProcessor::PathInterval> m_intervals;
};
}  // namespace

namespace routing
{
// static
size_t const LeapsPostProcessor::kMaxStep = 5;

/// \brief It is assumed that there in no cycles in path.
/// But sometimes it's wrong (because of heuristic searching of intermediate
/// points (mwms' exists) in LeapsOnly routing algorithm).
/// Look here for screenshots of such problem: https://github.com/mapsme/omim/pull/11091
/// This code removes cycles from path before |Init()|.
/// The algorithm saves jumps between the same segments, so if we have such loop:
/// s1, s2, ... , s10, s1, s11, s12, ... , s100, where "s3" means segment number 3.
/// ^------------------^ <--- loop (from s1 to s1).
/// Thus if we look at the first s1, we will find out that a jump to second s1 exists
/// and we will skip all segments between first and second s1 and the result path will be:
/// s1, s11, s12, ... , s100
LeapsPostProcessor::LeapsPostProcessor(std::vector<Segment> const & path, IndexGraphStarter & starter)
  : m_starter(starter)
  , m_bfs(starter)
{
  std::map<size_t, size_t> jumps;
  std::map<Segment, size_t> segmentToIndex;

  for (size_t i = 0; i < path.size(); ++i)
  {
    auto const it = segmentToIndex.find(path[i]);
    if (it == segmentToIndex.cend())
    {
      segmentToIndex[path[i]] = i;
      continue;
    }

    auto const prevIndex = it->second;
    jumps[prevIndex] = i;
    it->second = i;
  }

  for (size_t i = 0; i < path.size(); ++i)
  {
    auto it = jumps.find(i);
    if (it == jumps.end())
    {
      m_path.emplace_back(path[i]);
      continue;
    }

    while (it != jumps.end())
    {
      i = it->second;
      it = jumps.find(i);
    }

    CHECK_LESS(i, path.size(), ());
    m_path.emplace_back(path[i]);
  }

  Init();
}

void LeapsPostProcessor::Init()
{
  m_prefixSumETA = std::vector<double>(m_path.size(), 0.0);

  for (size_t i = 1; i < m_path.size(); ++i)
  {
    auto const & segment = m_path[i];
    m_prefixSumETA[i] = m_prefixSumETA[i - 1] + m_starter.CalculateETAWithoutPenalty(segment);

    CHECK_EQUAL(m_segmentToIndex.count(segment), 0, ());
    m_segmentToIndex[segment] = i;
  }
}

std::vector<Segment> LeapsPostProcessor::GetProcessedPath()
{
  auto const intervalsToRelax = CalculateIntervalsToRelax();

  NonIntersectingSegmentPaths toReplace;
  for (auto const & interval : intervalsToRelax)
    toReplace.AddInterval(interval);

  std::vector<PathInterval> intervals = toReplace.StealIntervals();
  std::sort(intervals.begin(), intervals.end());

  std::vector<Segment> output;
  size_t prevIndex = 0;
  for (auto & interval : intervals)
  {
    CHECK(m_path.begin() + prevIndex <= m_path.begin() + interval.m_left, ());
    output.insert(output.end(), m_path.begin() + prevIndex, m_path.begin() + interval.m_left);
    output.insert(output.end(), interval.m_path.begin(), interval.m_path.end());

    prevIndex = interval.m_right + 1;
  }

  output.insert(output.end(), m_path.begin() + prevIndex, m_path.end());
  return output;
}

auto LeapsPostProcessor::CalculateIntervalsToRelax() -> std::set<PathInterval, PathInterval::GreaterByWeight>
{
  std::set<PathInterval, PathInterval::GreaterByWeight> result;
  for (size_t right = kMaxStep; right < m_path.size(); ++right)
  {
    std::map<Segment, SegmentData> segmentsData;
    auto const & segment = m_path[right];
    segmentsData.emplace(segment, SegmentData(0, m_starter.CalculateETAWithoutPenalty(segment)));

    FillIngoingPaths(segment, segmentsData);

    for (auto const & item : segmentsData)
    {
      Segment const & visited = item.first;
      SegmentData const & data = item.second;

      auto const it = m_segmentToIndex.find(visited);
      if (it == m_segmentToIndex.cend())
        continue;

      size_t const left = it->second;
      if (left >= right || left == 0)
        continue;

      auto const prevWeight = m_prefixSumETA[right] - m_prefixSumETA[left - 1];
      auto const curWeight = data.m_summaryETA;
      if (prevWeight - PathInterval::kWeightEps <= curWeight)
        continue;

      double const winWeight = prevWeight - curWeight;
      result.emplace(winWeight, left, right, m_bfs.ReconstructPath(visited, true /* reverse */));
    }
  }

  return result;
}

void LeapsPostProcessor::FillIngoingPaths(Segment const & start,
                                          std::map<Segment, LeapsPostProcessor::SegmentData> & segmentsData)
{
  m_bfs.Run(start, false /* isOutgoing */, [&segmentsData, this](BFS<IndexGraphStarter>::State const & state)
  {
    if (segmentsData.count(state.m_vertex) != 0)
      return false;

    auto const & parent = segmentsData[state.m_parent];
    ASSERT_LESS_OR_EQUAL(parent.m_steps, kMaxStep, ());
    if (parent.m_steps == kMaxStep)
      return false;

    auto & current = segmentsData[state.m_vertex];
    current.m_summaryETA = parent.m_summaryETA + m_starter.CalculateETAWithoutPenalty(state.m_vertex);

    current.m_steps = parent.m_steps + 1;

    return true;
  });
}

LeapsPostProcessor::SegmentData::SegmentData(size_t steps, double eta) : m_steps(steps), m_summaryETA(eta) {}

LeapsPostProcessor::PathInterval::PathInterval(double weight, size_t left, size_t right, std::vector<Segment> && path)
  : m_winWeight(weight)
  , m_left(left)
  , m_right(right)
  , m_path(std::move(path))
{}

bool LeapsPostProcessor::PathInterval::GreaterByWeight::operator()(PathInterval const & lhs,
                                                                   PathInterval const & rhs) const
{
  return lhs.m_winWeight > rhs.m_winWeight;
}

bool LeapsPostProcessor::PathInterval::operator<(PathInterval const & rhs) const
{
  CHECK(m_left > rhs.m_right || m_right < rhs.m_left || (m_left == rhs.m_left && m_right == rhs.m_right),
        ("Intervals shouldn't intersect.", *this, rhs));

  return m_right < rhs.m_left;
}

std::string DebugPrint(LeapsPostProcessor::PathInterval const & interval)
{
  std::stringstream ss;
  ss << "[" << interval.m_left << ", " << interval.m_right << "], weight = " << interval.m_winWeight;

  return ss.str();
}
}  // namespace routing
