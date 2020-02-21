#pragma once

#include "routing/base/astar_graph.hpp"
#include "routing/base/astar_vertex_data.hpp"
#include "routing/fake_feature_ids.hpp"
#include "routing/index_graph_starter.hpp"
#include "routing/joint_segment.hpp"
#include "routing/segment.hpp"

#include "routing/base/astar_algorithm.hpp"

#include "geometry/distance_on_sphere.hpp"
#include "geometry/latlon.hpp"

#include "base/assert.hpp"

#include <algorithm>
#include <limits>
#include <map>
#include <optional>
#include <queue>
#include <set>
#include <vector>

#include "3party/skarupke/bytell_hash_map.hpp"

namespace routing
{
template <typename Graph>
class IndexGraphStarterJoints : public AStarGraph<JointSegment, JointEdge, RouteWeight>
{
public:
  explicit IndexGraphStarterJoints(Graph & graph) : m_graph(graph) {}
  IndexGraphStarterJoints(Graph & graph,
                          Segment const & startSegment,
                          Segment const & endSegment);

  IndexGraphStarterJoints(Graph & graph,
                          Segment const & startSegment);

  void Init(Segment const & startSegment, Segment const & endSegment);

  ms::LatLon const & GetPoint(JointSegment const & jointSegment, bool start);
  JointSegment const & GetStartJoint() const { return m_startJoint; }
  JointSegment const & GetFinishJoint() const { return m_endJoint; }

  // AStarGraph overridings
  // @{
  RouteWeight HeuristicCostEstimate(Vertex const & from, Vertex const & to) override;

  void GetOutgoingEdgesList(astar::VertexData<Vertex, Weight> const & vertexData,
                            std::vector<Edge> & edges) override
  {
    GetEdgeList(vertexData, true /* isOutgoing */, edges);
  }

  void GetIngoingEdgesList(astar::VertexData<Vertex, Weight> const & vertexData,
                           std::vector<Edge> & edges) override
  {
    GetEdgeList(vertexData, false /* isOutgoing */, edges);
  }

  void SetAStarParents(bool forward, Parents & parents) override
  {
    m_graph.SetAStarParents(forward, parents);
  }

  void DropAStarParents() override
  {
    m_graph.DropAStarParents();
  }

  bool AreWavesConnectible(Parents & forwardParents, Vertex const & commonVertex,
                           Parents & backwardParents) override
  {
    auto converter = [&](JointSegment const & vertex)
    {
      ASSERT(!vertex.IsRealSegment(), ());

      auto const it = m_fakeJointSegments.find(vertex);
      ASSERT(it != m_fakeJointSegments.cend(), ());

      auto const & first = it->second.GetSegment(true /* start */);
      if (first.IsRealSegment())
        return first.GetFeatureId();

      auto const & second = it->second.GetSegment(false /* start */);
      return second.GetFeatureId();
    };

    return m_graph.AreWavesConnectible(forwardParents, commonVertex, backwardParents,
                                       std::move(converter));
  }

  RouteWeight GetAStarWeightEpsilon() override { return m_graph.GetAStarWeightEpsilon(); }
  // @}

  WorldGraphMode GetMode() const { return m_graph.GetMode(); }
  
  /// \brief Reconstructs JointSegment by segment after building the route.
  std::vector<Segment> ReconstructJoint(JointSegment const & joint);

  void Reset();

  // Can not check segment for fake or not with segment.IsRealSegment(), because all segments
  // have got fake m_numMwmId during mwm generation.
  bool IsRealSegment(Segment const & segment) const
  {
    return segment.GetFeatureId() != FakeFeatureIds::kIndexGraphStarterId;
  }

  Segment const & GetSegmentOfFakeJoint(JointSegment const & joint, bool start);

  ~IndexGraphStarterJoints() override = default;

private:
  static auto constexpr kInvalidId = JointSegment::kInvalidSegmentId;
  static auto constexpr kInvisibleEndId = kInvalidId - 1;
  static auto constexpr kInvisibleStartId = kInvalidId - 2;

  struct FakeJointSegment
  {
    FakeJointSegment() = default;
    FakeJointSegment(Segment const & start, Segment const & end)
      : m_start(start), m_end(end) {}

    Segment const & GetSegment(bool start) const
    {
      return start ? m_start : m_end;
    }

    Segment m_start;
    Segment m_end;
  };

  void InitEnding(Segment const & ending, bool start);

  void AddFakeJoints(Segment const & segment, bool isOutgoing, std::vector<JointEdge> & edges);

  void GetEdgeList(astar::VertexData<Vertex, Weight> const & vertexData, bool isOutgoing,
                   std::vector<JointEdge> & edges);

  JointSegment CreateFakeJoint(Segment const & from, Segment const & to);

  bool IsJoint(Segment const & segment, bool fromStart) const
  {
    return m_graph.IsJoint(segment, fromStart);
  }

  bool IsJointOrEnd(Segment const & segment, bool fromStart) const
  {
    return m_graph.IsJointOrEnd(segment, fromStart);
  }

  bool FillEdgesAndParentsWeights(astar::VertexData<Vertex, Weight> const & vertexData,
                                  bool isOutgoing,
                                  size_t & firstFakeId,
                                  std::vector<JointEdge> & edges,
                                  std::vector<Weight> & parentWeights);

  std::optional<Segment> GetParentSegment(JointSegment const & vertex, bool isOutgoing,
                                          std::vector<JointEdge> & edges);

  /// \brief Makes BFS from |startSegment| in direction |fromStart| and find the closest segments
  /// which end RoadPoints are joints. Thus we build fake joint segments graph.
  std::vector<JointEdge> FindFirstJoints(Segment const & startSegment, bool fromStart);

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
    ReconstructedPath(std::vector<Segment> && path, bool fromStart)
      : m_fromStart(fromStart), m_path(std::move(path)) {}

    bool m_fromStart = true;
    std::vector<Segment> m_path;
  };

  std::map<JointSegment, ReconstructedPath> m_reconstructedFakeJoints;

  // List of JointEdges that are outgoing from start.
  std::vector<JointEdge> m_startOutEdges;
  // List of incoming to finish.
  std::vector<JointEdge> m_endOutEdges;

  uint32_t m_fakeId = 0;
  bool m_init = false;
};

template <typename Graph>
IndexGraphStarterJoints<Graph>::IndexGraphStarterJoints(Graph & graph,
                                                        Segment const & startSegment,
                                                        Segment const & endSegment)
  : m_graph(graph), m_startSegment(startSegment), m_endSegment(endSegment)
{
  Init(m_startSegment, m_endSegment);
}

template <typename Graph>
IndexGraphStarterJoints<Graph>::IndexGraphStarterJoints(Graph & graph,
                                                        Segment const & startSegment)
  : m_graph(graph), m_startSegment(startSegment)
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
RouteWeight IndexGraphStarterJoints<Graph>::HeuristicCostEstimate(JointSegment const & from,
                                                                  JointSegment const & to)
{
  ASSERT(to == m_startJoint || to == m_endJoint, ("Invariant violated."));
  bool toEnd = to == m_endJoint;

  Segment fromSegment;
  if (from.IsFake() || IsInvisible(from))
  {
    ASSERT_NOT_EQUAL(m_reconstructedFakeJoints.count(from), 0, ());
    fromSegment = m_reconstructedFakeJoints[from].m_path.back();
  }
  else
  {
    fromSegment = from.GetSegment(false /* start */);
  }

  return toEnd ? m_graph.HeuristicCostEstimate(fromSegment, m_endPoint)
               : m_graph.HeuristicCostEstimate(fromSegment, m_startPoint);
}

template <typename Graph>
ms::LatLon const &
IndexGraphStarterJoints<Graph>::GetPoint(JointSegment const & jointSegment, bool start)
{
  Segment segment = jointSegment.IsFake() ? m_fakeJointSegments[jointSegment].GetSegment(start)
                                          : jointSegment.GetSegment(start);

  return m_graph.GetPoint(segment, jointSegment.IsForward());
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
    CHECK(it != m_reconstructedFakeJoints.cend(), ("Can not find such fake joint"));

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
void IndexGraphStarterJoints<Graph>::AddFakeJoints(Segment const & segment, bool isOutgoing,
                                                   std::vector<JointEdge> & edges)
{
  // If |isOutgoing| is true, we need real segments, that are real parts
  // of fake joints, entered to finish and vice versa.
  std::vector<JointEdge> const & endings = isOutgoing ? m_endOutEdges : m_startOutEdges;

  bool const opposite = !isOutgoing;
  for (auto const & edge : endings)
  {
    // The one end of FakeJointSegment is start/finish and the opposite end is real segment.
    // So we check, whether |segment| is equal to the real segment of FakeJointSegment.
    // If yes, that means, that we can go from |segment| to start/finish.
    auto const it = m_fakeJointSegments.find(edge.GetTarget());
    if (it == m_fakeJointSegments.cend())
      continue;

    auto const & fakeJointSegment = it->second;
    Segment const & firstSegment = fakeJointSegment.GetSegment(!opposite /* start */);
    if (firstSegment == segment)
    {
      edges.emplace_back(edge);
      return;
    }
  }
}

template <typename Graph>
Segment const & IndexGraphStarterJoints<Graph>::GetSegmentOfFakeJoint(JointSegment const & joint, bool start)
{
  auto const it = m_fakeJointSegments.find(joint);
  CHECK(it != m_fakeJointSegments.cend(), ("No such fake joint:", joint, "in JointStarter."));

  return (it->second).GetSegment(start);
}

template <typename Graph>
std::optional<Segment> IndexGraphStarterJoints<Graph>::GetParentSegment(
    JointSegment const & vertex, bool isOutgoing, std::vector<JointEdge> & edges)
{
  std::optional<Segment> parentSegment;
  bool const opposite = !isOutgoing;
  if (vertex.IsFake())
  {
    CHECK(m_fakeJointSegments.find(vertex) != m_fakeJointSegments.end(),
          ("No such fake joint:", vertex, "in JointStarter."));

    FakeJointSegment const & fakeJointSegment = m_fakeJointSegments[vertex];

    auto const & startSegment = isOutgoing ? m_startSegment : m_endSegment;
    auto const & endSegment = isOutgoing ? m_endSegment : m_startSegment;
    auto const & endJoint = isOutgoing ? GetFinishJoint() : GetStartJoint();

    // This is case when we can build route from start to finish without real segment, only fake.
    // It happens when start and finish are close to each other.
    // If we want A* stop, we should add |endJoint| to its queue, then A* will see the vertex: |endJoint|
    // where it has already been and stop working.
    if (fakeJointSegment.GetSegment(opposite /* start */) == endSegment)
    {
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

    CHECK(fakeJointSegment.GetSegment(!opposite /* start */) == startSegment, ());
    parentSegment = fakeJointSegment.GetSegment(opposite /* start */);
  }
  else
  {
    parentSegment = vertex.GetSegment(opposite /* start */);
  }

  return parentSegment;
}

template <typename Graph>
bool IndexGraphStarterJoints<Graph>::FillEdgesAndParentsWeights(
    astar::VertexData<Vertex, Weight> const & vertexData,
    bool isOutgoing, size_t & firstFakeId,
    std::vector<JointEdge> & edges, std::vector<Weight> & parentWeights)
{
  auto const & vertex = vertexData.m_vertex;
  // Case of fake start or finish joints.
  // Note: startJoint and finishJoint are just loops
  //       from start to start or end to end vertex.
  if (vertex == GetStartJoint())
  {
    edges.insert(edges.end(), m_startOutEdges.begin(), m_startOutEdges.end());
    parentWeights.insert(parentWeights.end(), edges.size(), Weight(0.0));
    firstFakeId = edges.size();
  }
  else if (vertex == GetFinishJoint())
  {
    edges.insert(edges.end(), m_endOutEdges.begin(), m_endOutEdges.end());
    // If vertex is FinishJoint, parentWeight is equal to zero, because the first vertex is zero-weight loop.
    parentWeights.insert(parentWeights.end(), edges.size(), Weight(0.0));
  }
  else
  {
    auto const optional = GetParentSegment(vertex, isOutgoing, edges);
    if (!optional)
      return false;

    Segment const & parentSegment = *optional;
    m_graph.GetEdgeList(vertexData, parentSegment, isOutgoing, edges, parentWeights);

    firstFakeId = edges.size();
    bool const opposite = !isOutgoing;
    for (size_t i = 0; i < firstFakeId; ++i)
    {
      size_t prevSize = edges.size();
      auto const & target = edges[i].GetTarget();

      auto const & segment = target.IsFake() ? m_fakeJointSegments[target].GetSegment(!opposite)
                                             : target.GetSegment(!opposite);

      AddFakeJoints(segment, isOutgoing, edges);

      // If we add fake edge, we should add new parentWeight as "child[i] -> parent".
      // Because fake edge and current edge (jointEdges[i]) have the same first
      // segments (differ only the ends), so we add to |parentWeights| the same
      // value: parentWeights[i].
      if (edges.size() != prevSize)
      {
        CHECK_LESS(i, parentWeights.size(), ());
        parentWeights.emplace_back(parentWeights[i]);
      }
    }
  }

  return true;
}

template <typename Graph>
void IndexGraphStarterJoints<Graph>::GetEdgeList(
    astar::VertexData<Vertex, Weight> const & vertexData, bool isOutgoing,
    std::vector<JointEdge> & edges)
{
  CHECK(m_init, ("IndexGraphStarterJoints was not initialized."));

  edges.clear();

  // This vector needs for backward A* search. Assume, we have parent and child_1, child_2.
  // In this case we will save next weights:
  // 1) from child_1 to parent.
  // 2) from child_2 to parent.
  // That needs for correct edges weights calculation.
  std::vector<Weight> parentWeights;

  size_t firstFakeId = 0;
  if (!FillEdgesAndParentsWeights(vertexData, isOutgoing, firstFakeId, edges, parentWeights))
    return;

  if (!isOutgoing)
  {
    auto const & vertex = vertexData.m_vertex;
    // |parentSegment| is parent-vertex from which we search children.
    // For correct weight calculation we should get weight of JointSegment, that
    // ends in |parentSegment| and add |parentWeight[i]| to the saved value.
    auto const it = m_savedWeight.find(vertex);
    CHECK(it != m_savedWeight.cend(), ("Can not find weight for:", vertex));

    Weight const weight = it->second;
    for (size_t i = 0; i < edges.size(); ++i)
    {
      // Saving weight of current edges for returning in the next iterations.
      m_savedWeight[edges[i].GetTarget()] = edges[i].GetWeight();
      // For parent JointSegment we know its weight without last segment, because for each child
      // it will differ (child_1 => parent != child_2 => parent), but (!) we save this weight in
      // |parentWeights[]|. So the weight of an ith edge is a cached "weight of parent JointSegment" +
      // "parentWeight[i]".
      CHECK_LESS(i, parentWeights.size(), ());
      edges[i].GetWeight() = weight + parentWeights[i];
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
}

template <typename Graph>
JointSegment IndexGraphStarterJoints<Graph>::CreateFakeJoint(Segment const & from, Segment const & to)
{
  JointSegment jointSegment;
  jointSegment.ToFake(m_fakeId++);

  FakeJointSegment fakeJointSegment(from, to);
  m_fakeJointSegments[jointSegment] = fakeJointSegment;

  return jointSegment;
}

template <typename Graph>
std::vector<JointEdge> IndexGraphStarterJoints<Graph>::FindFirstJoints(Segment const & startSegment,
                                                                       bool fromStart)
{
  Segment const & endSegment = fromStart ? m_endSegment : m_startSegment;

  std::queue<Segment> queue;
  queue.emplace(startSegment);

  std::map<Segment, Segment> parent;
  std::map<Segment, RouteWeight> weight;
  std::vector<JointEdge> result;

  auto const reconstructPath = [&parent, &startSegment](Segment current, bool forward)
  {
    std::vector<Segment> path;
    path.emplace_back(current);

    if (current == startSegment)
      return path;

    Segment parentSegment;
    do
    {
      parentSegment = parent[current];
      path.emplace_back(parentSegment);
      current = parentSegment;
    } while (parentSegment != startSegment);

    if (forward)
      std::reverse(path.begin(), path.end());

    return path;
  };

  auto const addFake = [&](Segment const & segment, Segment const & beforeConvert)
  {
    JointSegment fakeJoint;
    fakeJoint = fromStart ? CreateFakeJoint(startSegment, segment) :
                            CreateFakeJoint(segment, startSegment);
    result.emplace_back(fakeJoint, weight[beforeConvert]);

    std::vector<Segment> path = reconstructPath(beforeConvert, fromStart);
    m_reconstructedFakeJoints.emplace(fakeJoint,
                                      ReconstructedPath(std::move(path), fromStart));
  };

  auto const isEndOfSegment = [&](Segment const & fake, Segment const & segment, bool fromStart)
  {
    CHECK(!IsRealSegment(fake), ());

    bool const hasSameFront =
        m_graph.GetPoint(fake, true /* front */) == m_graph.GetPoint(segment, true);

    bool const hasSameBack =
        m_graph.GetPoint(fake, false /* front */) == m_graph.GetPoint(segment, false);

    return (fromStart && hasSameFront) || (!fromStart && hasSameBack);
  };

  while (!queue.empty())
  {
    Segment segment = queue.front();
    queue.pop();
    Segment beforeConvert = segment;
    // Either the segment is fake and it can be converted to real one with |Joint| end (RoadPoint),
    // or it's the real one and its end (RoadPoint) is |Joint|.
    if (((!IsRealSegment(segment) && m_graph.ConvertToReal(segment) &&
          isEndOfSegment(beforeConvert, segment, fromStart)) || IsRealSegment(beforeConvert)) &&
        IsJoint(segment, fromStart))
    {
      addFake(segment, beforeConvert);
      continue;
    }

    if (beforeConvert == endSegment)
    {
      addFake(segment, beforeConvert);
      continue;
    }

    std::vector<SegmentEdge> edges;
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
  JointSegment result;
  result.ToFake(start ? kInvisibleStartId : kInvisibleEndId);
  m_fakeJointSegments[result] = FakeJointSegment(segment, segment);

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
