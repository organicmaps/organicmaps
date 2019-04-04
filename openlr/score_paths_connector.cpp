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

  lenScore = static_cast<Score>(static_cast<double>(kMaxScoreForRouteLen) *
                                (pathDiffRatio - kMinValidPathDiffRation) /
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
    LOG(LINFO, ("Best score:", it->m_score, "resultPathPart.size():", resultPathPart.size()));
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
      return make_tuple(m_score, m_edge) > make_tuple(o.m_score, o.m_edge);
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
        path.push_back(e);
      path.push_back(from);
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
    copy(begin(from), end(from), back_inserter(resultPath));
    copy(begin(to) + skip, end(to), back_inserter(resultPath));
    return true;
  }

  CHECK(from.back() != to.front(), ());

  Graph::EdgeVector shortestPath;
  auto const found =
      FindShortestPath(from.back(), to.front(), lowestFrcToNextPoint,
                       static_cast<uint32_t>(ceil(distanceToNextPoint)), shortestPath);
  if (!found)
    return false;

  // Skip the last edge from |from| because it already took its place at begin(shortestPath).
  copy(begin(from), prev(end(from)), back_inserter(resultPath));
  copy(begin(shortestPath), end(shortestPath), back_inserter(resultPath));
  // Skip the first edge from |to| because it already took its place at prev(end(shortestPath)).
  copy(next(begin(to)), end(to), back_inserter(resultPath));

  return found && !resultPath.empty();
}

Score ScorePathsConnector::GetScoreForUniformity(Graph::EdgeVector const & path)
{
  ftypes::HighwayClass minHwClass = ftypes::HighwayClass::Undefined;
  ftypes::HighwayClass maxHwClass = ftypes::HighwayClass::Undefined;
  bool oneWay = false;
  bool oneWayIsTheSame = true;
  bool roundabout = false;
  bool roundaboutIsTheSame = true;
  bool link = false;
  bool linkIsTheSame = true;
  string name;
  bool nameIsTheSame = true;

  for (auto const & p : path)
  {
    CHECK(!p.IsFake(), ());

    feature::TypesHolder types;
    m_graph.GetFeatureTypes(p.GetFeatureId(), types);

    string name;
    if (minHwClass == ftypes::HighwayClass::Undefined)
    {
      minHwClass = ftypes::GetHighwayClass(types);
      maxHwClass = minHwClass;
      oneWay = ftypes::IsOneWayChecker::Instance()(types);
      roundabout = ftypes::IsRoundAboutChecker::Instance()(types);
      link = ftypes::IsLinkChecker::Instance()(types);
      name = m_graph.GetName(p.GetFeatureId());
    }
    else
    {
      ftypes::HighwayClass const hwClass = ftypes::GetHighwayClass(types);
      minHwClass = static_cast<ftypes::HighwayClass>(
          min(static_cast<uint8_t>(minHwClass), static_cast<uint8_t>(hwClass)));
      maxHwClass = static_cast<ftypes::HighwayClass>(
          max(static_cast<uint8_t>(maxHwClass), static_cast<uint8_t>(hwClass)));

      if (oneWayIsTheSame && oneWay != ftypes::IsOneWayChecker::Instance()(types))
        oneWayIsTheSame = false;
      if (roundaboutIsTheSame && roundabout != ftypes::IsRoundAboutChecker::Instance()(types))
        roundaboutIsTheSame = false;
      if (linkIsTheSame && link != ftypes::IsLinkChecker::Instance()(types))
        linkIsTheSame = false;
      if (nameIsTheSame && name != m_graph.GetName(p.GetFeatureId()))
        nameIsTheSame = false;
    }
  }
  CHECK_NOT_EQUAL(minHwClass, ftypes::HighwayClass::Undefined, ());

  uint8_t const hwClassDiff = static_cast<uint8_t>(maxHwClass) - static_cast<uint8_t>(minHwClass);
  CHECK_GREATER_OR_EQUAL(hwClassDiff, 0, ());

  Score constexpr kScoreForTheSameHwClass = 40;
  Score constexpr kScoreForNeighboringHwClasses = 15;
  Score const hwClassScore = hwClassDiff == 0
                             ? kScoreForTheSameHwClass
                             : hwClassDiff == 1 ? kScoreForNeighboringHwClasses : 0;

  Score constexpr kScoreForOneWayOnly = 17;
  Score constexpr kScoreForRoundaboutOnly = 18;
  Score constexpr kScoreForLinkOnly = 10;
  Score constexpr kScoreForTheSameName = 10;
  Score const theSameTypeScore = (oneWayIsTheSame ? kScoreForOneWayOnly : 0) +
                                 (roundaboutIsTheSame ? kScoreForRoundaboutOnly : 0) +
                                 (linkIsTheSame ? kScoreForLinkOnly : 0) +
                                 (nameIsTheSame && !name.empty() ? kScoreForTheSameName : 0);

  return hwClassScore + theSameTypeScore;
}
}  // namespace openlr
