#include "openlr/score_paths_connector.hpp"

#include "openlr/helpers.hpp"

#include "indexer/feature_data.hpp"
#include "indexer/ftypes_matcher.hpp"

#include "base/assert.hpp"
#include "base/checked_cast.hpp"
#include "base/stl_iterator.hpp"

#include <algorithm>
#include <cmath>
#include <map>
#include <string>
#include <queue>
#include <tuple>

using namespace std;

namespace openlr
{
namespace
{
class Uniformity
{
public:
  struct Field
  {
    bool m_field = false;
    bool m_isTheSame = true;
  };

  explicit Uniformity(Graph const & graph) : m_graph(graph) {}

  void NextEdge(Graph::Edge const & edge)
  {
    CHECK(!edge.IsFake(), ());

    feature::TypesHolder types;
    m_graph.GetFeatureTypes(edge.GetFeatureId(), types);

    if (m_minHwClass == ftypes::HighwayClass::Undefined)
    {
      m_minHwClass = ftypes::GetHighwayClass(types);
      m_maxHwClass = m_minHwClass;
      m_oneWay.m_field = ftypes::IsOneWayChecker::Instance()(types);
      m_roundabout.m_field = ftypes::IsRoundAboutChecker::Instance()(types);
      m_link.m_field = ftypes::IsLinkChecker::Instance()(types);
    }
    else
    {
      ftypes::HighwayClass const hwClass = ftypes::GetHighwayClass(types);
      m_minHwClass = static_cast<ftypes::HighwayClass>(
          min(static_cast<uint8_t>(m_minHwClass), static_cast<uint8_t>(hwClass)));
      m_maxHwClass = static_cast<ftypes::HighwayClass>(
          max(static_cast<uint8_t>(m_maxHwClass), static_cast<uint8_t>(hwClass)));

      if (m_oneWay.m_isTheSame && m_oneWay.m_field != ftypes::IsOneWayChecker::Instance()(types))
        m_oneWay.m_isTheSame = false;
      if (m_roundabout.m_isTheSame && m_roundabout.m_field != ftypes::IsRoundAboutChecker::Instance()(types))
        m_roundabout.m_isTheSame = false;
      if (m_link.m_isTheSame && m_link.m_field != ftypes::IsLinkChecker::Instance()(types))
        m_link.m_isTheSame = false;
    }
  }

  uint8_t GetHighwayClassDiff() const
  {
    CHECK_NOT_EQUAL(m_minHwClass, ftypes::HighwayClass::Undefined, ());
    CHECK_GREATER_OR_EQUAL(m_maxHwClass, m_minHwClass, ());

    uint8_t const hwClassDiff = static_cast<uint8_t>(m_maxHwClass) - static_cast<uint8_t>(m_minHwClass);
    return hwClassDiff;
  }

  bool IsOneWayTheSame() const { return m_oneWay.m_isTheSame; }
  bool IsRoundaboutTheSame() const { return m_roundabout.m_isTheSame; }
  bool IsLinkTheSame() const { return m_link.m_isTheSame; }

private:
  Graph const & m_graph;

  ftypes::HighwayClass m_minHwClass = ftypes::HighwayClass::Undefined;
  ftypes::HighwayClass m_maxHwClass = ftypes::HighwayClass::Undefined;
  Field m_oneWay;
  Field m_roundabout;
  Field m_link;
};

/// \returns true if |path| may be used as a candidate. In that case |lenScore| is filled
/// with score of this candidate based on length. The closer length of the |path| to
/// |distanceToNextPoint| the more score.
bool ValidatePathByLength(Graph::EdgeVector const & path, double distanceToNextPoint,
                          Score & lenScore)
{
  CHECK(!path.empty(), ());
  Score const kMaxScoreForRouteLen = 110;

  double pathLen = 0.0;
  for (auto const & e : path)
    pathLen += EdgeLength(e);

  // 0 <= |pathDiffRatio| <= 1. The more pathDiffRatio the closer |distanceToNextPoint| and |pathLen|.
  double const pathDiffRatio =
      1.0 - AbsDifference(distanceToNextPoint, pathLen) / max(distanceToNextPoint, pathLen);

  bool const shortPath = path.size() <= 2;
  double constexpr kMinValidPathDiffRation = 0.6;
  if (pathDiffRatio <= kMinValidPathDiffRation && !shortPath)
    return false;

  lenScore = static_cast<Score>(kMaxScoreForRouteLen * (pathDiffRatio - kMinValidPathDiffRation) /
                                (1.0 - kMinValidPathDiffRation));

  return true;
}
}  // namespace

ScorePathsConnector::ScorePathsConnector(Graph & graph, RoadInfoGetter & infoGetter, v2::Stats & stat)
  : m_graph(graph), m_infoGetter(infoGetter), m_stat(stat)
{
}

bool ScorePathsConnector::FindBestPath(vector<LocationReferencePoint> const & points,
                                       vector<vector<ScorePath>> const & lineCandidates,
                                       vector<Graph::EdgeVector> & resultPath)
{
  CHECK_GREATER_OR_EQUAL(points.size(), 2, ());

  resultPath.resize(points.size() - 1);

  for (size_t i = 1; i < points.size(); ++i)
  {
    // @TODO It's possible that size of |points| is more then two. In that case two or more edges
    // should be approximated with routes. If so only |toCandidates| which may be reached from
    // |fromCandidates| should be used for the next edge.

    auto const & point = points[i - 1];
    auto const distanceToNextPoint = static_cast<double>(point.m_distanceToNextPoint);
    auto const & fromCandidates = lineCandidates[i - 1];
    auto const & toCandidates = lineCandidates[i];
    auto & resultPathPart = resultPath[i - 1];

    vector<ScorePath> result;
    for (size_t fromInd = 0; fromInd < fromCandidates.size(); ++fromInd)
    {
      for (size_t toInd = 0; toInd < toCandidates.size(); ++toInd)
      {
        Graph::EdgeVector path;
        if (!ConnectAdjacentCandidateLines(fromCandidates[fromInd].m_path,
                                           toCandidates[toInd].m_path, point.m_lfrcnp,
                                           distanceToNextPoint, path))
        {
          continue;
        }

        Score pathLenScore = 0;
        if (!ValidatePathByLength(path, distanceToNextPoint, pathLenScore))
          continue;

        result.emplace_back(pathLenScore + GetScoreForUniformity(path) +
                                fromCandidates[fromInd].m_score + toCandidates[toInd].m_score,
                            move(path));
      }
    }

    for (auto const & p : result)
      CHECK(!p.m_path.empty(), ());

    if (result.empty())
    {
      LOG(LINFO, ("No shortest path found"));
      ++m_stat.m_noShortestPathFound;
      resultPathPart.clear();
      return false;
    }

    auto const it = max_element(
        result.cbegin(), result.cend(),
        [](ScorePath const & o1, ScorePath const & o2) { return o1.m_score < o2.m_score; });

    Score constexpr kMinValidScore = 240;
    if (it->m_score < kMinValidScore)
    {
      LOG(LINFO, ("The shortest path found but it is no good."));
      return false;
    }

    resultPathPart = it->m_path;
    LOG(LDEBUG, ("Best score:", it->m_score, "resultPathPart.size():", resultPathPart.size()));
  }

  CHECK_EQUAL(resultPath.size(), points.size() - 1, ());

  return true;
}

bool ScorePathsConnector::FindShortestPath(Graph::Edge const & from, Graph::Edge const & to,
                                           FunctionalRoadClass lowestFrcToNextPoint,
                                           uint32_t maxPathLength, Graph::EdgeVector & path)
{
  double constexpr kLengthToleranceFactor = 1.1;
  uint32_t constexpr kMinLengthTolerance = 20;
  uint32_t const lengthToleranceM =
      max(static_cast<uint32_t>(kLengthToleranceFactor * maxPathLength), kMinLengthTolerance);

  struct State
  {
    State(Graph::Edge const & e, uint32_t const s) : m_edge(e), m_score(s) {}

    bool operator>(State const & o) const
    {
      return tie(m_score, m_edge) > tie(o.m_score, o.m_edge);
    }

    Graph::Edge m_edge;
    uint32_t m_score;
  };

  CHECK(from.HasRealPart() && to.HasRealPart(), ());

  priority_queue<State, vector<State>, greater<>> q;
  map<Graph::Edge, double> scores;
  map<Graph::Edge, Graph::Edge> links;

  q.emplace(from, 0);
  scores[from] = 0;

  while (!q.empty())
  {
    auto const state = q.top();
    q.pop();

    auto const & u = state.m_edge;
    // TODO(mgsergio): Unify names: use either score or distance.
    auto const us = state.m_score;

    if (us > maxPathLength + lengthToleranceM)
      continue;

    if (us > scores[u])
      continue;

    if (u == to)
    {
      for (auto e = u; e != from; e = links[e])
        path.emplace_back(e);
      path.emplace_back(from);
      reverse(begin(path), end(path));
      return true;
    }

    Graph::EdgeVector edges;
    m_graph.GetOutgoingEdges(u.GetEndJunction(), edges);
    for (auto const & e : edges)
    {
      if (!ConformLfrcnpV3(e, lowestFrcToNextPoint, m_infoGetter))
        continue;

      CHECK(!u.IsFake(), ());
      CHECK(!e.IsFake(), ());

      auto const it = scores.find(e);
      auto const eScore = us + EdgeLength(e);
      if (it == end(scores) || it->second > eScore)
      {
        scores[e] = eScore;
        links[e] = u;
        q.emplace(e, eScore);
      }
    }
  }

  return false;
}

bool ScorePathsConnector::ConnectAdjacentCandidateLines(Graph::EdgeVector const & from,
                                                        Graph::EdgeVector const & to,
                                                        FunctionalRoadClass lowestFrcToNextPoint,
                                                        double distanceToNextPoint,
                                                        Graph::EdgeVector & resultPath)
{
  CHECK(!to.empty(), ());

  if (auto const skip = PathOverlappingLen(from, to))
  {
    if (skip == -1)
      return false;

    resultPath.insert(resultPath.end(), from.cbegin(), from.cend());
    resultPath.insert(resultPath.end(), to.cbegin() + skip, to.cend());
    return true;
  }

  CHECK_NOT_EQUAL(from, to, ());

  Graph::EdgeVector shortestPath;
  auto const found =
      FindShortestPath(from.back(), to.front(), lowestFrcToNextPoint,
                       static_cast<uint32_t>(ceil(distanceToNextPoint)), shortestPath);
  if (!found)
    return false;

  // Skip the last edge from |from| because it already took its place at begin(shortestPath).
  resultPath.insert(resultPath.end(), from.cbegin(), prev(from.cend()));
  resultPath.insert(resultPath.end(), shortestPath.cbegin(), shortestPath.cend());

  // Skip the first edge from |to| because it already took its place at prev(end(shortestPath)).
  resultPath.insert(resultPath.end(), next(to.begin()), to.end());

  return !resultPath.empty();
}

Score ScorePathsConnector::GetScoreForUniformity(Graph::EdgeVector const & path)
{
  Uniformity uniformity(m_graph);
  for (auto const & edge : path)
    uniformity.NextEdge(edge);

  auto const hwClassDiff = uniformity.GetHighwayClassDiff();
  Score constexpr kScoreForTheSameHwClass = 40;
  Score constexpr kScoreForNeighboringHwClasses = 15;
  Score const hwClassScore = hwClassDiff == 0
                             ? kScoreForTheSameHwClass
                             : hwClassDiff == 1 ? kScoreForNeighboringHwClasses : 0;

  Score constexpr kScoreForOneWayOnly = 17;
  Score constexpr kScoreForRoundaboutOnly = 18;
  Score constexpr kScoreForLinkOnly = 10;
  Score const theSameTypeScore = (uniformity.IsOneWayTheSame() ? kScoreForOneWayOnly : 0) +
                                 (uniformity.IsRoundaboutTheSame() ? kScoreForRoundaboutOnly : 0) +
                                 (uniformity.IsLinkTheSame() ? kScoreForLinkOnly : 0);

  return hwClassScore + theSameTypeScore;
}
}  // namespace openlr
