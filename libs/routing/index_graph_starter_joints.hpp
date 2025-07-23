#pragma once

#include "routing/base/astar_graph.hpp"
#include "routing/base/astar_vertex_data.hpp"

#include "routing/joint_segment.hpp"
#include "routing/segment.hpp"

#include "geometry/latlon.hpp"

#include "base/assert.hpp"

#include "3party/skarupke/bytell_hash_map.hpp"  // needed despite of IDE warning

#include <algorithm>
#include <map>
#include <optional>
#include <queue>
#include <vector>

namespace routing
{
enum class WorldGraphMode;

template <typename Graph>
class IndexGraphStarterJoints : public AStarGraph<JointSegment, JointEdge, RouteWeight>
{
public:
  explicit IndexGraphStarterJoints(Graph & graph) : m_graph(graph) {}
  IndexGraphStarterJoints(Graph & graph, Segment const & startSegment, Segment const & endSegment);

  IndexGraphStarterJoints(Graph & graph, Segment const & startSegment);

  void Init(Segment const & startSegment, Segment const & endSegment);

  ms::LatLon const & GetPoint(JointSegment const & jointSegment, bool start) const
  {
    return m_graph.GetPoint(GetSegmentFromJoint(jointSegment, start), jointSegment.IsForward());
  }
  JointSegment const & GetStartJoint() const { return m_startJoint; }
  JointSegment const & GetFinishJoint() const { return m_endJoint; }

  // AStarGraph overridings
  // @{
  RouteWeight HeuristicCostEstimate(Vertex const & from, Vertex const & to) override;

  void GetOutgoingEdgesList(astar::VertexData<Vertex, Weight> const & vertexData, EdgeListT & edges) override
  {
    GetEdgeList(vertexData, true /* isOutgoing */, edges);
  }

  void GetIngoingEdgesList(astar::VertexData<Vertex, Weight> const & vertexData, EdgeListT & edges) override
  {
    GetEdgeList(vertexData, false /* isOutgoing */, edges);
  }

  void SetAStarParents(bool forward, Parents & parents) override { m_graph.SetAStarParents(forward, parents); }

  void DropAStarParents() override { m_graph.DropAStarParents(); }

  bool AreWavesConnectible(Parents & forwardParents, Vertex const & commonVertex, Parents & backwardParents) override
  {
    auto const converter = [&](JointSegment & vertex)
    {
      if (!vertex.IsFake())
        return;

      auto const & fake = GetFakeSegmentSure(vertex);
      auto const & first = fake.GetSegment(true /* start */);
      if (first.IsRealSegment())
        vertex.AssignID(first);
      else
        vertex.AssignID(fake.GetSegment(false /* start */));
    };

    return m_graph.AreWavesConnectible(forwardParents, commonVertex, backwardParents, converter);
  }

  RouteWeight GetAStarWeightEpsilon() override { return m_graph.GetAStarWeightEpsilon(); }
  // @}

  WorldGraphMode GetMode() const { return m_graph.GetMode(); }

  /// \brief Reconstructs JointSegment by segment after building the route.
  std::vector<Segment> ReconstructJoint(JointSegment const & joint);

  void Reset();

  static bool IsRealSegment(Segment const & segment) { return !segment.IsFakeCreated(); }

  Segment const & GetSegmentOfFakeJoint(JointSegment const & joint, bool start) const
  {
    return GetFakeSegmentSure(joint).GetSegment(start);
  }
  Segment GetSegmentFromJoint(JointSegment const & joint, bool start) const
  {
    return joint.IsFake() ? GetSegmentOfFakeJoint(joint, start) : joint.GetSegment(start);
  }

private:
  static auto constexpr kInvalidId = JointSegment::kInvalidSegmentId;
  static auto constexpr kInvisibleEndId = kInvalidId - 1;
  static auto constexpr kInvisibleStartId = kInvalidId - 2;

  struct FakeJointSegment
  {
    FakeJointSegment() = default;
    FakeJointSegment(Segment const & start, Segment const & end) : m_start(start), m_end(end) {}

    Segment const & GetSegment(bool start) const { return start ? m_start : m_end; }

    Segment m_start;
    Segment m_end;
  };

  void InitEnding(Segment const & ending, bool start);

  void AddFakeJoints(Segment const & segment, bool isOutgoing, EdgeListT & edges);

  void GetEdgeList(astar::VertexData<Vertex, Weight> const & vertexData, bool isOutgoing, EdgeListT & edges);

  JointSegment CreateFakeJoint(Segment const & from, Segment const & to, uint32_t featureId = kInvalidFeatureId);

  bool IsJoint(Segment const & segment, bool fromStart) const { return m_graph.IsJoint(segment, fromStart); }

  bool IsJointOrEnd(Segment const & segment, bool fromStart) const { return m_graph.IsJointOrEnd(segment, fromStart); }

  using WeightListT = typename Graph::WeightListT;
  bool FillEdgesAndParentsWeights(astar::VertexData<Vertex, Weight> const & vertexData, bool isOutgoing,
                                  size_t & firstFakeId, EdgeListT & edges, WeightListT & parentWeights);

  std::optional<Segment> GetParentSegment(JointSegment const & vertex, bool isOutgoing, EdgeListT & edges);

  /// \brief Makes BFS from |startSegment| in direction |fromStart| and find the closest segments
  /// which end RoadPoints are joints. Thus we build fake joint segments graph.
  EdgeListT FindFirstJoints(Segment const & startSegment, bool fromStart);

  JointSegment CreateInvisibleJoint(Segment const & segment, bool start);

  bool IsInvisible(JointSegment const & jointSegment) const
  {
    static_assert(kInvisibleStartId < kInvisibleEndId && kInvisibleEndId != kInvalidId,
                  "FakeID's are crossing in JointSegment.");

    return jointSegment.GetStartSegmentId() == jointSegment.GetEndSegmentId() &&
           (jointSegment.GetStartSegmentId() == kInvisibleStartId ||
            jointSegment.GetStartSegmentId() == kInvisibleEndId) &&
           jointSegment.GetStartSegmentId() != kInvalidId;
  }

  FakeJointSegment const & GetFakeSegmentSure(JointSegment const & key) const
  {
    auto const it = m_fakeJointSegments.find(key);
    ASSERT(it != m_fakeJointSegments.end(), (key));
    return it->second;
  }

  Graph & m_graph;

  // Fake start and end joints.
  JointSegment m_startJoint;
  JointSegment m_endJoint;

  Segment m_startSegment;
  Segment m_endSegment;

  ms::LatLon m_startPoint;
  ms::LatLon m_endPoint;

  // See comments in |GetEdgeList()| about |m_savedWeight|.
  ska::bytell_hash_map<Vertex, Weight> m_savedWeight;

  // JointSegment consists of two segments of one feature.
  // FakeJointSegment consists of two segments of different features.
  // So we create an invalid JointSegment (see |ToFake()| method), that
  // converts to FakeJointSegments. This std::map is converter.
  ska::flat_hash_map<JointSegment, FakeJointSegment> m_fakeJointSegments;

  struct ReconstructedPath
  {
    ReconstructedPath() = default;
    ReconstructedPath(std::vector<Segment> && path, bool fromStart) : m_fromStart(fromStart), m_path(std::move(path)) {}

    bool m_fromStart = true;
    std::vector<Segment> m_path;
  };

  std::map<JointSegment, ReconstructedPath> m_reconstructedFakeJoints;

  // List of JointEdges that are outgoing from start.
  EdgeListT m_startOutEdges;
  // List of incoming to finish.
  EdgeListT m_endOutEdges;

  uint32_t m_fakeId = 0;
  bool m_init = false;
};

template <typename Graph>
IndexGraphStarterJoints<Graph>::IndexGraphStarterJoints(Graph & graph, Segment const & startSegment,
                                                        Segment const & endSegment)
  : m_graph(graph)
  , m_startSegment(startSegment)
  , m_endSegment(endSegment)
{
  Init(m_startSegment, m_endSegment);
}

template <typename Graph>
IndexGraphStarterJoints<Graph>::IndexGraphStarterJoints(Graph & graph, Segment const & startSegment)
  : m_graph(graph)
  , m_startSegment(startSegment)
{
  InitEnding(startSegment, true /* start */);

  m_endSegment = Segment();
  m_endJoint = JointSegment();
  m_endPoint = ms::LatLon();

  m_init = true;
}

template <typename Graph>
void IndexGraphStarterJoints<Graph>::Init(Segment const & startSegment, Segment const & endSegment)
{
  InitEnding(startSegment, true /* start */);
  InitEnding(endSegment, false /* start */);

  m_init = true;
}

template <typename Graph>
void IndexGraphStarterJoints<Graph>::InitEnding(Segment const & ending, bool start)
{
  auto & segment = start ? m_startSegment : m_endSegment;
  segment = ending;

  auto & point = start ? m_startPoint : m_endPoint;
  point = m_graph.GetPoint(ending, true /* front */);

  auto & endingJoint = start ? m_startJoint : m_endJoint;
  if (IsRealSegment(ending))
  {
    endingJoint = CreateInvisibleJoint(ending, start);
  }
  else
  {
    auto const & loopSegment = start ? m_graph.GetStartSegment() : m_graph.GetFinishSegment();
    endingJoint = CreateFakeJoint(loopSegment, loopSegment);
  }

  m_reconstructedFakeJoints[endingJoint] = ReconstructedPath({ending}, start);

  auto & outEdges = start ? m_startOutEdges : m_endOutEdges;
  outEdges = FindFirstJoints(ending, start);

  if (!start)
  {
    m_savedWeight[m_endJoint] = Weight(0.0);
    for (auto const & edge : m_endOutEdges)
      m_savedWeight[edge.GetTarget()] = edge.GetWeight();
  }
}

template <typename Graph>
RouteWeight IndexGraphStarterJoints<Graph>::HeuristicCostEstimate(JointSegment const & from, JointSegment const & to)
{
  ASSERT(to == m_startJoint || to == m_endJoint, ("Invariant violated."));

  Segment fromSegment;
  if (from.IsFake() || IsInvisible(from))
  {
    auto const it = m_reconstructedFakeJoints.find(from);
    CHECK(it != m_reconstructedFakeJoints.end(), ("No such fake joint:", from));
    fromSegment = it->second.m_path.back();
  }
  else
  {
    fromSegment = from.GetSegment(false /* start */);
  }

  return (to == m_endJoint) ? m_graph.HeuristicCostEstimate(fromSegment, m_endPoint)
                            : m_graph.HeuristicCostEstimate(fromSegment, m_startPoint);
}

template <typename Graph>
std::vector<Segment> IndexGraphStarterJoints<Graph>::ReconstructJoint(JointSegment const & joint)
{
  // We have invisible JointSegments, which are come from start to start or end to end.
  // They need just for generic algorithm working. So we skip such objects.
  if (IsInvisible(joint))
    return {};

  // In case of a fake vertex we return its prebuilt path.
  if (joint.IsFake())
  {
    auto const it = m_reconstructedFakeJoints.find(joint);
    CHECK(it != m_reconstructedFakeJoints.cend(), ("No such fake joint:", joint));

    auto path = it->second.m_path;
    ASSERT(!path.empty(), ());
    if (path.front() == m_startSegment && path.back() == m_endSegment)
      path.pop_back();

    return path;
  }

  // Otherwise just reconstruct segment consequently.
  std::vector<Segment> subpath;

  Segment currentSegment = joint.GetSegment(true /* start */);
  Segment lastSegment = joint.GetSegment(false /* start */);

  bool const forward = currentSegment.GetSegmentIdx() < lastSegment.GetSegmentIdx();
  while (currentSegment != lastSegment)
  {
    subpath.emplace_back(currentSegment);
    currentSegment.Next(forward);
  }

  subpath.emplace_back(lastSegment);
  return subpath;
}

template <typename Graph>
void IndexGraphStarterJoints<Graph>::AddFakeJoints(Segment const & segment, bool isOutgoing, EdgeListT & edges)
{
  // If |isOutgoing| is true, we need real segments, that are real parts
  // of fake joints, entered to finish and vice versa.
  EdgeListT const & endings = isOutgoing ? m_endOutEdges : m_startOutEdges;

  for (auto const & edge : endings)
  {
    // The one end of FakeJointSegment is start/finish and the opposite end is real segment.
    // So we check, whether |segment| is equal to the real segment of FakeJointSegment.
    // If yes, that means, that we can go from |segment| to start/finish.
    auto const it = m_fakeJointSegments.find(edge.GetTarget());
    if (it == m_fakeJointSegments.end())
      continue;

    Segment const & firstSegment = it->second.GetSegment(isOutgoing /* start */);
    if (firstSegment == segment)
    {
      edges.emplace_back(edge);
      return;
    }
  }
}

template <typename Graph>
std::optional<Segment> IndexGraphStarterJoints<Graph>::GetParentSegment(JointSegment const & vertex, bool isOutgoing,
                                                                        EdgeListT & edges)
{
  std::optional<Segment> parentSegment;
  bool const opposite = !isOutgoing;
  if (vertex.IsFake())
  {
    FakeJointSegment const & fakeJointSegment = GetFakeSegmentSure(vertex);

    auto const & endSegment = isOutgoing ? m_endSegment : m_startSegment;
    parentSegment = fakeJointSegment.GetSegment(opposite /* start */);

    // This is case when we can build route from start to finish without real segment, only fake.
    // It happens when start and finish are close to each other.
    // If we want A* stop, we should add |endJoint| to its queue, then A* will see the vertex: |endJoint|
    // where it has already been and stop working.
    if (parentSegment == endSegment)
    {
      auto const & endJoint = isOutgoing ? m_endJoint : m_startJoint;
      if (isOutgoing)
      {
        static auto constexpr kZeroWeight = RouteWeight(0.0);
        edges.emplace_back(endJoint, kZeroWeight);
      }
      else
      {
        auto const it = m_savedWeight.find(vertex);
        CHECK(it != m_savedWeight.cend(), ("Can not find weight for:", vertex));

        Weight const & weight = it->second;
        edges.emplace_back(endJoint, weight);
      }
      return {};
    }

#ifdef DEBUG
    auto const & startSegment = isOutgoing ? m_startSegment : m_endSegment;
    ASSERT_EQUAL(fakeJointSegment.GetSegment(!opposite /* start */), startSegment, ());
#endif  // DEBUG
  }
  else
  {
    parentSegment = vertex.GetSegment(opposite /* start */);
  }

  return parentSegment;
}

template <typename Graph>
bool IndexGraphStarterJoints<Graph>::FillEdgesAndParentsWeights(astar::VertexData<Vertex, Weight> const & vertexData,
                                                                bool isOutgoing, size_t & firstFakeId,
                                                                EdgeListT & edges, WeightListT & parentWeights)
{
  auto const & vertex = vertexData.m_vertex;
  // Case of fake start or finish joints.
  // Note: startJoint and finishJoint are just loops
  //       from start to start or end to end vertex.
  if (vertex == GetStartJoint())
  {
    edges.append(m_startOutEdges.begin(), m_startOutEdges.end());
    parentWeights.append(edges.size(), Weight(0.0));
    firstFakeId = edges.size();
  }
  else if (vertex == GetFinishJoint())
  {
    edges.append(m_endOutEdges.begin(), m_endOutEdges.end());
    // If vertex is FinishJoint, parentWeight is equal to zero, because the first vertex is zero-weight loop.
    parentWeights.append(edges.size(), Weight(0.0));
  }
  else
  {
    auto const optional = GetParentSegment(vertex, isOutgoing, edges);
    if (!optional)
      return false;

    Segment const & parentSegment = *optional;
    m_graph.GetEdgeList(vertexData, parentSegment, isOutgoing, edges, parentWeights);

    firstFakeId = edges.size();
    for (size_t i = 0; i < firstFakeId; ++i)
    {
      size_t const prevSize = edges.size();

      AddFakeJoints(GetSegmentFromJoint(edges[i].GetTarget(), isOutgoing), isOutgoing, edges);

      // If we add fake edge, we should add new parentWeight as "child[i] -> parent".
      // Because fake edge and current edge (jointEdges[i]) have the same first
      // segments (differ only the ends), so we add to |parentWeights| the same
      // value: parentWeights[i].
      if (edges.size() != prevSize)
        parentWeights.push_back(parentWeights[i]);
    }
  }

  return true;
}

template <typename Graph>
void IndexGraphStarterJoints<Graph>::GetEdgeList(astar::VertexData<Vertex, Weight> const & vertexData, bool isOutgoing,
                                                 EdgeListT & edges)
{
  CHECK(m_init, ("IndexGraphStarterJoints was not initialized."));

  edges.clear();

  // This vector needs for backward A* search. Assume, we have parent and child_1, child_2.
  // In this case we will save next weights:
  // 1) from child_1 to parent.
  // 2) from child_2 to parent.
  // That needs for correct edges weights calculation.
  WeightListT parentWeights;

  size_t firstFakeId = 0;
  if (!FillEdgesAndParentsWeights(vertexData, isOutgoing, firstFakeId, edges, parentWeights))
    return;

  auto const & vertex = vertexData.m_vertex;
  if (!isOutgoing)
  {
    // |parentSegment| is parent-vertex from which we search children.
    // For correct weight calculation we should get weight of JointSegment, that
    // ends in |parentSegment| and add |parentWeight[i]| to the saved value.
    auto const it = m_savedWeight.find(vertex);
    CHECK(it != m_savedWeight.cend(), ("Can not find weight for:", vertex));

    Weight const weight = it->second;
    for (size_t i = 0; i < edges.size(); ++i)
    {
      // Saving weight of current edges for returning in the next iterations.
      auto & w = edges[i].GetWeight();
      auto const & t = edges[i].GetTarget();

      // By VNG: Revert to the MM original code, since Cross-MWM borders penalty is assigned in the end of this
      // function.
      /// @todo I still have doubts on how we "fetch" weight for ingoing edges with m_savedWeight.
      m_savedWeight[t] = w;

      // For parent JointSegment we know its weight without last segment, because for each child
      // it will differ (child_1 => parent != child_2 => parent), but (!) we save this weight in
      // |parentWeights[]|. So the weight of an ith edge is a cached "weight of parent JointSegment" +
      // "parentWeight[i]".
      w = weight + parentWeights[i];
    }

    // Delete useless weight of parent JointSegment.
    m_savedWeight.erase(vertex);
  }
  else
  {
    // This needs for correct weights calculation of FakeJointSegments during forward A* search.
    for (size_t i = firstFakeId; i < edges.size(); ++i)
      edges[i].GetWeight() += parentWeights[i];
  }

  auto const vertexMwmId = vertex.GetMwmId();
  if (vertexMwmId != kFakeNumMwmId)
  {
    /// @todo By VNG: Main problem with current (borders penalty) approach and bidirectional A* is that:
    /// a weight of v1->v2 transition moving forward (v1 outgoing) should be the same as
    /// a weight of v1->v2 transition moving backward (v2 ingoing). This is impossible in current (m_savedWeight)
    /// logic, so I moved Cross-MWM penalty into separate block here after _all_ weights calculations.

    for (auto & e : edges)
    {
      auto const targetMwmId = e.GetTarget().GetMwmId();
      if (targetMwmId != kFakeNumMwmId && vertexMwmId != targetMwmId)
        e.GetWeight() += m_graph.GetCrossBorderPenalty(vertexMwmId, targetMwmId);
    }
  }
}

template <typename Graph>
JointSegment IndexGraphStarterJoints<Graph>::CreateFakeJoint(Segment const & from, Segment const & to,
                                                             uint32_t featureId /* = kInvalidFeatureId*/)
{
  JointSegment const result = JointSegment::MakeFake(m_fakeId++, featureId);
  m_fakeJointSegments.emplace(result, FakeJointSegment(from, to));
  return result;
}

template <typename Graph>
typename IndexGraphStarterJoints<Graph>::EdgeListT IndexGraphStarterJoints<Graph>::FindFirstJoints(
    Segment const & startSegment, bool fromStart)
{
  Segment const & endSegment = fromStart ? m_endSegment : m_startSegment;

  std::queue<Segment> queue;
  queue.emplace(startSegment);

  std::map<Segment, Segment> parent;
  std::map<Segment, RouteWeight> weight;
  EdgeListT result;

  auto const reconstructPath = [&parent, &startSegment](Segment current, bool forward)
  {
    std::vector<Segment> path;
    path.emplace_back(current);

    while (current != startSegment)
    {
      current = parent[current];
      path.emplace_back(current);
    }

    if (forward)
      std::reverse(path.begin(), path.end());
    return path;
  };

  uint32_t firstFeatureId = kInvalidFeatureId;
  auto const addFake = [&](Segment const & segment, Segment const & beforeConvert)
  {
    JointSegment fakeJoint;
    fakeJoint =
        fromStart ? CreateFakeJoint(startSegment, segment, firstFeatureId) : CreateFakeJoint(segment, startSegment);
    result.emplace_back(fakeJoint, weight[beforeConvert]);

    std::vector<Segment> path = reconstructPath(beforeConvert, fromStart);
    m_reconstructedFakeJoints.emplace(fakeJoint, ReconstructedPath(std::move(path), fromStart));
  };

  auto const isEndOfSegment = [&](Segment const & fake, Segment const & segment, bool fromStart)
  {
    bool const hasSameFront = m_graph.GetPoint(fake, true /* front */) == m_graph.GetPoint(segment, true);

    bool const hasSameBack = m_graph.GetPoint(fake, false /* front */) == m_graph.GetPoint(segment, false);

    return (fromStart && hasSameFront) || (!fromStart && hasSameBack);
  };

  while (!queue.empty())
  {
    Segment segment = queue.front();
    queue.pop();
    Segment beforeConvert = segment;

    bool const isRealPart =
        !IsRealSegment(segment) && m_graph.ConvertToReal(segment) && isEndOfSegment(beforeConvert, segment, fromStart);

    // Get first real feature id and assign it below into future fake joint, that will pass over this feature.
    // Its important for IndexGraph::IsRestricted. See https://github.com/organicmaps/organicmaps/issues/1565.
    if (isRealPart && firstFeatureId == kInvalidFeatureId)
      firstFeatureId = segment.GetFeatureId();

    // Either the segment is fake and it can be converted to real one with |Joint| end (RoadPoint),
    // or it's the real one and its end (RoadPoint) is |Joint|.
    if ((isRealPart || IsRealSegment(beforeConvert)) && IsJoint(segment, fromStart))
    {
      addFake(segment, beforeConvert);
      continue;
    }

    if (beforeConvert == endSegment)
    {
      addFake(segment, beforeConvert);
      continue;
    }

    typename Graph::EdgeListT edges;
    m_graph.GetEdgesList(beforeConvert, fromStart, edges);

    for (auto const & edge : edges)
    {
      Segment child = edge.GetTarget();
      auto const & newWeight = weight[beforeConvert] + edge.GetWeight();
      if (weight.find(child) == weight.end() || weight[child] > newWeight)
      {
        parent[child] = beforeConvert;
        weight[child] = newWeight;
        queue.push(child);
      }
    }
  }

  return result;
}

template <typename Graph>
JointSegment IndexGraphStarterJoints<Graph>::CreateInvisibleJoint(Segment const & segment, bool start)
{
  JointSegment const result = JointSegment::MakeFake(start ? kInvisibleStartId : kInvisibleEndId);
  m_fakeJointSegments.emplace(result, FakeJointSegment(segment, segment));
  return result;
}

template <typename Graph>
void IndexGraphStarterJoints<Graph>::Reset()
{
  m_startJoint = JointSegment();
  m_endJoint = JointSegment();
  m_startSegment = Segment();
  m_endSegment = Segment();
  m_savedWeight.clear();
  m_fakeJointSegments.clear();
  m_reconstructedFakeJoints.clear();
  m_startOutEdges.clear();
  m_endOutEdges.clear();
  m_fakeId = 0;
  m_init = false;
}
}  // namespace routing
