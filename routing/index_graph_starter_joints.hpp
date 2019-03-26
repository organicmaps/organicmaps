#pragma once

#include "routing/base/astar_graph.hpp"

#include "routing/fake_feature_ids.hpp"
#include "routing/index_graph_starter.hpp"
#include "routing/joint_segment.hpp"
#include "routing/segment.hpp"

#include "geometry/point2d.hpp"

#include "base/assert.hpp"

#include <algorithm>
#include <limits>
#include <map>
#include <queue>
#include <set>
#include <vector>

#include "boost/optional.hpp"

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

  JointSegment const & GetStartJoint() const { return m_startJoint; }
  JointSegment const & GetFinishJoint() const { return m_endJoint; }
  m2::PointD const & GetPoint(JointSegment const & jointSegment, bool start);

  // AStarGraph overridings
  // @{
  RouteWeight HeuristicCostEstimate(JointSegment const & from, JointSegment const & to) override;

  void GetOutgoingEdgesList(JointSegment const & vertex, std::vector<JointEdge> & edges) override
  {
    GetEdgeList(vertex, true /* isOutgoing */, edges);
  }

  void GetIngoingEdgesList(JointSegment const & vertex, std::vector<JointEdge> & edges) override
  {
    GetEdgeList(vertex, false /* isOutgoing */, edges);
  }
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
  static auto constexpr kInvalidId = JointSegment::kInvalidId;
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

  void AddFakeJoints(Segment const & segment, bool isOutgoing, std::vector<JointEdge> & edges);

  void GetEdgeList(JointSegment const & vertex, bool isOutgoing, std::vector<JointEdge> & edges);

  JointSegment CreateFakeJoint(Segment const & from, Segment const & to);

  bool IsJoint(Segment const & segment, bool fromStart) const
  {
    return m_graph.IsJoint(segment, fromStart);
  }

  bool FillEdgesAndParentsWeights(JointSegment const & vertex,
                                  bool isOutgoing,
                                  size_t & firstFakeId,
                                  std::vector<JointEdge> & edges,
                                  std::vector<Weight> & parentWeights);

  boost::optional<Segment> GetParentSegment(JointSegment const & vertex,
                                            bool isOutgoing,
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

  // For GetEdgeList from segments.
  Graph & m_graph;

  // Fake start and end joints.
  JointSegment m_startJoint;
  JointSegment m_endJoint;

  Segment m_startSegment;
  Segment m_endSegment;

  m2::PointD m_startPoint;
  m2::PointD m_endPoint;

  // See comments in |GetEdgeList()| about |m_savedWeight|.
  std::map<JointSegment, Weight> m_savedWeight;

  // JointSegment consists of two segments of one feature.
  // FakeJointSegment consists of two segments of different features.
  // So we create an invalid JointSegment (see |ToFake()| method), that
  // converts to FakeJointSegments. This std::map is converter.
  std::map<JointSegment, FakeJointSegment> m_fakeJointSegments;
  std::map<JointSegment, std::vector<Segment>> m_reconstructedFakeJoints;

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
  : m_graph(graph), m_startSegment(startSegment), m_endSegment(Segment())
{
  Init(m_startSegment, m_endSegment);
}

template <typename Graph>
void IndexGraphStarterJoints<Graph>::Init(Segment const & startSegment, Segment const & endSegment)
{
  m_startSegment = startSegment;
  m_endSegment = endSegment;

  m_startPoint = m_graph.GetPoint(m_startSegment, true /* front */);
  m_endPoint = m_graph.GetPoint(m_endSegment, true /* front */);

  if (IsRealSegment(startSegment))
    m_startJoint = CreateInvisibleJoint(startSegment, true /* start */);
  else
    m_startJoint = CreateFakeJoint(m_graph.GetStartSegment(), m_graph.GetStartSegment());

  if (IsRealSegment(endSegment))
    m_endJoint = CreateInvisibleJoint(endSegment, false /* start */);
  else
    m_endJoint = CreateFakeJoint(m_graph.GetFinishSegment(), m_graph.GetFinishSegment());

  m_reconstructedFakeJoints[m_startJoint] = {m_startSegment};
  m_reconstructedFakeJoints[m_endJoint] = {m_endSegment};

  m_startOutEdges = FindFirstJoints(startSegment, true /* fromStart */);
  m_endOutEdges = FindFirstJoints(endSegment, false /* fromStart */);

  m_savedWeight[m_endJoint] = Weight(0.0);
  for (auto const & edge : m_endOutEdges)
    m_savedWeight[edge.GetTarget()] = edge.GetWeight();

  m_init = true;
}

template <typename Graph>
RouteWeight IndexGraphStarterJoints<Graph>::HeuristicCostEstimate(JointSegment const & from, JointSegment const & to)
{
  Segment const & fromSegment =
    from.IsFake() || IsInvisible(from) ? m_fakeJointSegments[from].GetSegment(false /* start */)
                                       : from.GetSegment(false /* start */);

  Segment const & toSegment =
    to.IsFake() || IsInvisible(to) ? m_fakeJointSegments[to].GetSegment(false /* start */)
                                   : to.GetSegment(false /* start */);

  ASSERT(toSegment == m_startSegment || toSegment == m_endSegment, ("Invariant violated."));

  return toSegment == m_startSegment ? m_graph.HeuristicCostEstimate(fromSegment, m_startPoint)
                                     : m_graph.HeuristicCostEstimate(fromSegment, m_endPoint);
}

template <typename Graph>
m2::PointD const &
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

    return it->second;
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
    Segment const & firstSegment = m_fakeJointSegments[edge.GetTarget()].GetSegment(!opposite /* start */);
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
boost::optional<Segment> IndexGraphStarterJoints<Graph>::GetParentSegment(JointSegment const & vertex,
                                                                          bool isOutgoing,
                                                                          std::vector<JointEdge> & edges)
{
  boost::optional<Segment> parentSegment;
  bool const opposite = !isOutgoing;
  if (vertex.IsFake())
  {
    CHECK(m_fakeJointSegments.find(vertex) != m_fakeJointSegments.end(),
          ("No such fake joint:", vertex, "in JointStarter."));

    FakeJointSegment fakeJointSegment = m_fakeJointSegments[vertex];

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
bool IndexGraphStarterJoints<Graph>::FillEdgesAndParentsWeights(JointSegment const & vertex,
                                                                bool isOutgoing,
                                                                size_t & firstFakeId,
                                                                std::vector<JointEdge> & edges,
                                                                std::vector<Weight> & parentWeights)
{
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

    Segment parentSegment = optional.value();

    std::vector<JointEdge> jointEdges;
    m_graph.GetEdgeList(parentSegment, isOutgoing, jointEdges, parentWeights);
    edges.insert(edges.end(), jointEdges.begin(), jointEdges.end());

    firstFakeId = edges.size();
    bool const opposite = !isOutgoing;
    for (size_t i = 0; i < jointEdges.size(); ++i)
    {
      size_t prevSize = edges.size();
      AddFakeJoints(jointEdges[i].GetTarget().GetSegment(!opposite /* start */), isOutgoing, edges);
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
void IndexGraphStarterJoints<Graph>::GetEdgeList(JointSegment const & vertex, bool isOutgoing,
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
  if (!FillEdgesAndParentsWeights(vertex, isOutgoing, firstFakeId, edges, parentWeights))
    return;

  if (!isOutgoing)
  {
    // |parentSegment| is parent-vertex from which we search children.
    // For correct weight calculation we should get weight of JointSegment, that
    // ends in |parentSegment| and add |parentWeight[i]| to the saved value.
    auto const it = m_savedWeight.find(vertex);
    CHECK(it != m_savedWeight.cend(), ("Can not find weight for:", vertex));

    Weight const & weight = it->second;
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
    m_savedWeight.erase(it);
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

  auto const reconstructPath = [&parent, &startSegment, &endSegment](Segment current, bool forward)
  {
    std::vector<Segment> path;
    // In case we can go from start to finish without joint (e.g. start and finish at
    // the same long feature), we don't add the last segment to path for correctness
    // reconstruction of the route. Otherwise last segment will repeat twice.
    if (current != endSegment)
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
    m_reconstructedFakeJoints[fakeJoint] = path;
  };

  auto const isEndOfSegment = [&](Segment const & fake, Segment const & segment)
  {
    CHECK(!IsRealSegment(fake), ());

    auto const fakeEnd = m_graph.GetPoint(fake, fake.IsForward());
    auto const realEnd = m_graph.GetPoint(segment, segment.IsForward());

    static auto constexpr kEps = 1e-5;
    return base::AlmostEqualAbs(fakeEnd, realEnd, kEps);
  };

  while (!queue.empty())
  {
    Segment segment = queue.front();
    queue.pop();
    Segment beforeConvert = segment;
    // Either the segment is fake and it can be converted to real one with |Joint| end (RoadPoint),
    // or it's the real one and its end (RoadPoint) is |Joint|.
    if (((!IsRealSegment(segment) && m_graph.ConvertToReal(segment) &&
          isEndOfSegment(beforeConvert, segment)) || IsRealSegment(beforeConvert)) &&
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
