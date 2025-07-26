#include "openlr/score_paths_connector.hpp"

#include "openlr/helpers.hpp"

#include "indexer/feature_data.hpp"
#include "indexer/ftypes_matcher.hpp"

#include "base/assert.hpp"
#include "base/checked_cast.hpp"
#include "base/stl_helpers.hpp"
#include "base/stl_iterator.hpp"

#include <algorithm>
#include <cmath>
#include <map>
#include <queue>
#include <string>
#include <tuple>

using namespace std;

namespace openlr
{
namespace
{
class EdgeContainer
{
public:
  explicit EdgeContainer(Graph const & graph) : m_graph(graph) {}

  void ProcessEdge(Graph::Edge const & edge)
  {
    CHECK(!edge.IsFake(), ());

    feature::TypesHolder types;
    m_graph.GetFeatureTypes(edge.GetFeatureId(), types);

    ftypes::HighwayClass const hwClass = ftypes::GetHighwayClass(types);
    if (m_minHwClass == ftypes::HighwayClass::Undefined)
    {
      m_minHwClass = hwClass;
      m_maxHwClass = m_minHwClass;
      m_oneWay.m_field = ftypes::IsOneWayChecker::Instance()(types);
      m_roundabout.m_field = ftypes::IsRoundAboutChecker::Instance()(types);
      m_link.m_field = ftypes::IsLinkChecker::Instance()(types);
    }
    else
    {
      CHECK(hwClass != ftypes::HighwayClass::Undefined, (edge));

      m_minHwClass = static_cast<ftypes::HighwayClass>(min(base::Underlying(m_minHwClass), base::Underlying(hwClass)));
      m_maxHwClass = static_cast<ftypes::HighwayClass>(max(base::Underlying(m_maxHwClass), base::Underlying(hwClass)));

      if (m_oneWay.m_allEqual && m_oneWay.m_field != ftypes::IsOneWayChecker::Instance()(types))
        m_oneWay.m_allEqual = false;

      if (m_roundabout.m_allEqual && m_roundabout.m_field != ftypes::IsRoundAboutChecker::Instance()(types))
        m_roundabout.m_allEqual = false;

      if (m_link.m_allEqual && m_link.m_field != ftypes::IsLinkChecker::Instance()(types))
        m_link.m_allEqual = false;
    }
  }

  uint8_t GetHighwayClassDiff() const
  {
    CHECK_NOT_EQUAL(m_minHwClass, ftypes::HighwayClass::Undefined, ());
    CHECK_GREATER_OR_EQUAL(m_maxHwClass, m_minHwClass, ());

    uint8_t const hwClassDiff = static_cast<uint8_t>(m_maxHwClass) - static_cast<uint8_t>(m_minHwClass);
    return hwClassDiff;
  }

  bool AreAllOneWaysEqual() const { return m_oneWay.m_allEqual; }
  bool AreAllRoundaboutEqual() const { return m_roundabout.m_allEqual; }
  bool AreAllLinksEqual() const { return m_link.m_allEqual; }

private:
  struct Field
  {
    bool m_field = false;
    bool m_allEqual = true;
  };

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
bool ValidatePathByLength(Graph::EdgeVector const & path, double distanceToNextPoint, LinearSegmentSource source,
                          Score & lenScore)
{
  CHECK(!path.empty(), ());
  CHECK_NOT_EQUAL(source, LinearSegmentSource::NotValid, ());

  Score const kMaxScoreForRouteLen = 110;

  double pathLen = 0.0;
  for (auto const & e : path)
    pathLen += EdgeLength(e);

  // 0 <= |pathDiffRatio| <= 1. The more pathDiffRatio the closer |distanceToNextPoint| and |pathLen|.
  double const pathDiffRatio = 1.0 - abs(distanceToNextPoint - pathLen) / max(distanceToNextPoint, pathLen);

  bool const shortPath = path.size() <= 2;
  double const kMinValidPathDiffRation = source == LinearSegmentSource::FromLocationReferenceTag ? 0.6 : 0.25;
  if (pathDiffRatio <= kMinValidPathDiffRation && !shortPath)
    return false;

  lenScore = static_cast<Score>(kMaxScoreForRouteLen * (pathDiffRatio - kMinValidPathDiffRation) /
                                (1.0 - kMinValidPathDiffRation));

  return true;
}

bool AreEdgesEqualWithoutAltitude(Graph::Edge const & e1, Graph::Edge const & e2)
{
  return make_tuple(e1.GetType(), e1.GetFeatureId(), e1.IsForward(), e1.GetSegId(), e1.GetStartPoint(),
                    e1.GetEndPoint()) == make_tuple(e2.GetType(), e2.GetFeatureId(), e2.IsForward(), e2.GetSegId(),
                                                    e2.GetStartPoint(), e2.GetEndPoint());
}
}  // namespace

ScorePathsConnector::ScorePathsConnector(Graph & graph, RoadInfoGetter & infoGetter, v2::Stats & stat)
  : m_graph(graph)
  , m_infoGetter(infoGetter)
  , m_stat(stat)
{}

bool ScorePathsConnector::FindBestPath(vector<LocationReferencePoint> const & points,
                                       vector<vector<ScorePath>> const & lineCandidates, LinearSegmentSource source,
                                       vector<Graph::EdgeVector> & resultPath)
{
  CHECK_NOT_EQUAL(source, LinearSegmentSource::NotValid, ());
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
        if (!ConnectAdjacentCandidateLines(fromCandidates[fromInd].m_path, toCandidates[toInd].m_path, source,
                                           point.m_lfrcnp, distanceToNextPoint, path))
        {
          continue;
        }

        Score pathLenScore = 0;
        if (!ValidatePathByLength(path, distanceToNextPoint, source, pathLenScore))
          continue;

        auto const score =
            pathLenScore + GetScoreForUniformity(path) + fromCandidates[fromInd].m_score + toCandidates[toInd].m_score;
        result.emplace_back(score, std::move(path));
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

    auto const it = max_element(result.cbegin(), result.cend(),
                                [](ScorePath const & o1, ScorePath const & o2) { return o1.m_score < o2.m_score; });

    // Note. In case of source == LinearSegmentSource::FromCoordinatesTag there is less
    // information about a openlr segment so less score is collected.
    Score const kMinValidScore = source == LinearSegmentSource::FromLocationReferenceTag ? 240 : 165;
    if (it->m_score < kMinValidScore)
    {
      LOG(LINFO, ("The shortest path found but it is not good. The best score:", it->m_score));
      ++m_stat.m_notEnoughScore;
      return false;
    }

    resultPathPart = it->m_path;
    LOG(LDEBUG, ("Best score:", it->m_score, "resultPathPart.size():", resultPathPart.size()));
  }

  CHECK_EQUAL(resultPath.size(), points.size() - 1, ());

  return true;
}

bool ScorePathsConnector::FindShortestPath(Graph::Edge const & from, Graph::Edge const & to, LinearSegmentSource source,
                                           FunctionalRoadClass lowestFrcToNextPoint, uint32_t maxPathLength,
                                           Graph::EdgeVector & path)
{
  double constexpr kLengthToleranceFactor = 1.1;
  uint32_t constexpr kMinLengthTolerance = 20;
  uint32_t const lengthToleranceM =
      max(static_cast<uint32_t>(kLengthToleranceFactor * maxPathLength), kMinLengthTolerance);

  struct State
  {
    State(Graph::Edge const & e, uint32_t const s) : m_edge(e), m_score(s) {}

    bool operator>(State const & o) const { return tie(m_score, m_edge) > tie(o.m_score, o.m_edge); }

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

    auto const & stateEdge = state.m_edge;
    // TODO(mgsergio): Unify names: use either score or distance.
    auto const stateScore = state.m_score;

    if (stateScore > maxPathLength + lengthToleranceM)
      continue;

    if (stateScore > scores[stateEdge])
      continue;

    if (AreEdgesEqualWithoutAltitude(stateEdge, to))
    {
      for (auto edge = stateEdge; edge != from; edge = links[edge])
        path.emplace_back(edge);
      path.emplace_back(from);
      reverse(begin(path), end(path));
      return true;
    }

    Graph::EdgeListT edges;
    m_graph.GetOutgoingEdges(stateEdge.GetEndJunction(), edges);
    for (auto const & edge : edges)
    {
      if (source == LinearSegmentSource::FromLocationReferenceTag &&
          !ConformLfrcnpV3(edge, lowestFrcToNextPoint, m_infoGetter))
      {
        continue;
      }

      CHECK(!stateEdge.IsFake(), ());
      CHECK(!edge.IsFake(), ());

      auto const it = scores.find(edge);
      auto const eScore = stateScore + EdgeLength(edge);
      if (it == end(scores) || it->second > eScore)
      {
        scores[edge] = eScore;
        links[edge] = stateEdge;
        q.emplace(edge, eScore);
      }
    }
  }

  return false;
}

bool ScorePathsConnector::ConnectAdjacentCandidateLines(Graph::EdgeVector const & from, Graph::EdgeVector const & to,
                                                        LinearSegmentSource source,
                                                        FunctionalRoadClass lowestFrcToNextPoint,
                                                        double distanceToNextPoint, Graph::EdgeVector & resultPath)
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
  auto const found = FindShortestPath(from.back(), to.front(), source, lowestFrcToNextPoint,
                                      static_cast<uint32_t>(ceil(distanceToNextPoint)), shortestPath);
  if (!found)
    return false;

  CHECK_EQUAL(from.back(), shortestPath.front(), ());
  resultPath.insert(resultPath.end(), from.cbegin(), prev(from.cend()));
  resultPath.insert(resultPath.end(), shortestPath.cbegin(), shortestPath.cend());

  CHECK(AreEdgesEqualWithoutAltitude(to.front(), shortestPath.back()), (to.front(), shortestPath.back()));
  resultPath.insert(resultPath.end(), next(to.begin()), to.end());

  return !resultPath.empty();
}

Score ScorePathsConnector::GetScoreForUniformity(Graph::EdgeVector const & path)
{
  EdgeContainer edgeContainer(m_graph);
  for (auto const & edge : path)
    edgeContainer.ProcessEdge(edge);

  auto const hwClassDiff = edgeContainer.GetHighwayClassDiff();
  Score constexpr kScoreForTheSameHwClass = 40;
  Score constexpr kScoreForNeighboringHwClasses = 15;
  Score const hwClassScore = hwClassDiff == 0 ? kScoreForTheSameHwClass
                           : hwClassDiff == 1 ? kScoreForNeighboringHwClasses
                                              : 0;

  Score constexpr kScoreForOneWayOnly = 17;
  Score constexpr kScoreForRoundaboutOnly = 18;
  Score constexpr kScoreForLinkOnly = 10;
  Score const allEqualScore = (edgeContainer.AreAllOneWaysEqual() ? kScoreForOneWayOnly : 0) +
                              (edgeContainer.AreAllRoundaboutEqual() ? kScoreForRoundaboutOnly : 0) +
                              (edgeContainer.AreAllLinksEqual() ? kScoreForLinkOnly : 0);

  return hwClassScore + allEqualScore;
}
}  // namespace openlr
