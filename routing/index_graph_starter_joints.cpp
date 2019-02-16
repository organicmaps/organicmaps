#include "routing/index_graph_starter_joints.hpp"

#include "base/assert.hpp"

#include <algorithm>
#include <utility>

namespace routing
{
IndexGraphStarterJoints::IndexGraphStarterJoints(IndexGraphStarter & starter,
                                                 Segment const & startSegment,
                                                 Segment const & endSegment)
  : m_starter(starter), m_startSegment(startSegment), m_endSegment(endSegment)
{
  Init(startSegment, endSegment);
}

void IndexGraphStarterJoints::Init(Segment const & startSegment, Segment const & endSegment)
{
  m_startSegment = startSegment;
  m_endSegment = endSegment;

  m_startPoint = m_starter.GetPoint(m_startSegment, true /* front */);
  m_endPoint = m_starter.GetPoint(m_endSegment, true /* front */);

  CHECK(m_starter.GetGraph().GetMode() == WorldGraph::Mode::Joints ||
        m_starter.GetGraph().GetMode() == WorldGraph::Mode::JointSingleMwm, ());

  if (startSegment.IsRealSegment())
    m_startJoint = CreateInvisibleJoint(startSegment, true /* start */);
  else
    m_startJoint = CreateFakeJoint(m_starter.GetStartSegment(), m_starter.GetStartSegment());

  if (endSegment.IsRealSegment())
    m_endJoint = CreateInvisibleJoint(endSegment, false /* start */);
  else
    m_endJoint = CreateFakeJoint(m_starter.GetFinishSegment(), m_starter.GetFinishSegment());

  m_reconstructedFakeJoints[m_startJoint] = {m_startSegment};
  m_reconstructedFakeJoints[m_endJoint] = {m_endSegment};

  m_startOutEdges = FindFirstJoints(startSegment, true /* fromStart */);
  m_endOutEdges = FindFirstJoints(endSegment, false /* fromStart */);

  m_savedWeight[m_endJoint] = Weight(0.0);
  for (auto const & edge : m_endOutEdges)
    m_savedWeight[edge.GetTarget()] = edge.GetWeight();

  m_init = true;
}

RouteWeight IndexGraphStarterJoints::HeuristicCostEstimate(JointSegment const & from, JointSegment const & to)
{
  Segment fromSegment;
  Segment toSegment;

  if (to.IsFake() || IsInvisible(to))
    toSegment = m_fakeJointSegments[to].GetSegment(false /* start */);
  else
    toSegment = to.GetSegment(false /* start */);

  if (from.IsFake() || IsInvisible(from))
    fromSegment = m_fakeJointSegments[from].GetSegment(false /* start */);
  else
    fromSegment = from.GetSegment(false /* start */);

  ASSERT(toSegment == m_startSegment || toSegment == m_endSegment, ("Invariant violated."));

  return toSegment == m_startSegment ? m_starter.HeuristicCostEstimate(fromSegment, m_startPoint)
                                     : m_starter.HeuristicCostEstimate(fromSegment, m_endPoint);
}

m2::PointD const & IndexGraphStarterJoints::GetPoint(JointSegment const & jointSegment, bool start)
{
  Segment segment;
  if (jointSegment.IsFake())
    segment = m_fakeJointSegments[jointSegment].GetSegment(start);
  else
    segment = jointSegment.GetSegment(start);

  return m_starter.GetPoint(segment, jointSegment.IsForward());
}

std::vector<Segment> IndexGraphStarterJoints::ReconstructJoint(JointSegment const & joint)
{
  // We have invisible JointSegments, which are come from start to start or end to end.
  // They need just for generic algorithm working. So we skip such objects.
  if (IsInvisible(joint))
    return {};

  // In case of a fake vertex we return its prebuild path.
  if (joint.IsFake())
  {
    auto it = m_reconstructedFakeJoints.find(joint);
    CHECK(it != m_reconstructedFakeJoints.end(), ("Can not find such fake joint"));

    return it->second;
  }

  // Otherwise just reconstruct segment consequently.
  std::vector<Segment> subpath;

  Segment currentSegment = joint.GetSegment(true /* start */);
  Segment lastSegment = joint.GetSegment(false /* start */);

  bool forward = currentSegment.GetSegmentIdx() < lastSegment.GetSegmentIdx();
  while (currentSegment != lastSegment)
  {
    subpath.emplace_back(currentSegment);
    currentSegment.Next(forward);
  }

  subpath.emplace_back(lastSegment);

  return subpath;
}

void IndexGraphStarterJoints::AddFakeJoints(Segment const & segment, bool isOutgoing,
                                            std::vector<JointEdge> & edges)
{
  // If |isOutgoing| is true, we need real segments, that are real parts
  // of fake joints, entered to finish and vice versa.
  std::vector<JointEdge> const & endings = isOutgoing ? m_endOutEdges : m_startOutEdges;

  bool opposite = !isOutgoing;
  for (auto const & edge : endings)
  {
    // The one end of FakeJointSegment is start/finish and the opposite end is real segment.
    // So we check, whether |segment| is equal to the real segment of FakeJointSegment.
    // If yes, that means, that we can go from |segment| to start/finish.
    Segment firstSegment = m_fakeJointSegments[edge.GetTarget()].GetSegment(!opposite /* start */);
    if (firstSegment == segment)
    {
      edges.emplace_back(edge);
      return;
    }
  }
}

void IndexGraphStarterJoints::GetEdgeList(JointSegment const & vertex, bool isOutgoing,
                                          std::vector<JointEdge> & edges)
{
  CHECK(m_init, ("IndexGraphStarterJoints was not initialized."));

  edges.clear();

  Segment parentSegment;

  // This vector needs for backward A* search. Assume, we have parent and child_1, child_2.
  // In this case we will save next weights:
  // 1) from child_1 to parent.
  // 2) from child_2 to parent.
  // That needs for correct edges weights calculation.
  std::vector<Weight> parentWeights;
  size_t firstFakeId = 0;

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
    bool opposite = !isOutgoing;
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
          auto it = m_savedWeight.find(vertex);
          CHECK(it != m_savedWeight.end(), ("Can not find weight for:", vertex));

          Weight const & weight = it->second;
          edges.emplace_back(endJoint, weight);
        }
        return;
      }

      CHECK(fakeJointSegment.GetSegment(!opposite /* start */) == startSegment, ());
      parentSegment = fakeJointSegment.GetSegment(opposite /* start */);
    }
    else
    {
      parentSegment = vertex.GetSegment(opposite /* start */);
    }

    std::vector<JointEdge> jointEdges;
    m_starter.GetGraph().GetEdgeList(parentSegment, isOutgoing, jointEdges, parentWeights);
    edges.insert(edges.end(), jointEdges.begin(), jointEdges.end());

    firstFakeId = edges.size();
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

  if (!isOutgoing)
  {
    // |parentSegment| is parent-vertex from which we search children.
    // For correct weight calculation we should get weight of JointSegment, that
    // ends in |parentSegment| and add |parentWeight[i]| to the saved value.
    auto it = m_savedWeight.find(vertex);
    CHECK(it != m_savedWeight.end(), ("Can not find weight for:", vertex));

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

JointSegment IndexGraphStarterJoints::CreateFakeJoint(Segment const & from, Segment const & to)
{
  JointSegment jointSegment;
  jointSegment.ToFake(m_fakeId++);

  FakeJointSegment fakeJointSegment(from, to);
  m_fakeJointSegments[jointSegment] = fakeJointSegment;

  return jointSegment;
}

std::vector<JointEdge> IndexGraphStarterJoints::FindFirstJoints(Segment const & startSegment, bool fromStart)
{
  Segment endSegment = fromStart ? m_endSegment : m_startSegment;

  std::queue<Segment> queue;
  queue.emplace(startSegment);

  std::map<Segment, Segment> parent;
  std::map<Segment, RouteWeight> weight;
  std::vector<JointEdge> result;

  auto const reconstructPath = [&parent, &startSegment, &endSegment](Segment current, bool forward) {
    std::vector<Segment> path;
    // In case we can go from start to finish without joint (e.g. start and finish at
    // the same long feature), we don't add the last segment to path for correctness
    // reconstruction of the route. Otherwise last segment will repeat twice.
    if (current != endSegment)
      path.emplace_back(current);

    if (current == startSegment)
      return path;

    for (;;)
    {
      Segment parentSegment = parent[current];

      path.emplace_back(parentSegment);
      if (parentSegment == startSegment)
        break;
      current = parentSegment;
    }

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
    m_reconstructedFakeJoints.emplace(fakeJoint, std::move(path));
  };

  auto const isEndOfSegment = [&](Segment const & fake, Segment const & segment)
  {
    CHECK(!fake.IsRealSegment(), ());

    auto fakeEnd = m_starter.GetPoint(fake, fake.IsForward());
    auto realEnd = m_starter.GetPoint(segment, segment.IsForward());

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
    if (((!segment.IsRealSegment() && m_starter.ConvertToReal(segment) &&
          isEndOfSegment(beforeConvert, segment)) || beforeConvert.IsRealSegment()) &&
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
    m_starter.GetEdgesList(beforeConvert, fromStart, edges);
    for (auto const & edge : edges)
    {
      Segment const & child = edge.GetTarget();
      auto const & newWeight = weight[beforeConvert] + edge.GetWeight();
      if (weight.find(child) == weight.end() || weight[child] > newWeight)
      {
        parent[child] = beforeConvert;
        weight[child] = newWeight;
        queue.emplace(child);
      }
    }
  }

  return result;
}

JointSegment IndexGraphStarterJoints::CreateInvisibleJoint(Segment const & segment, bool start)
{
  JointSegment result;
  result.ToFake(start ? kInvisibleId : kInvisibleId + 1);
  m_fakeJointSegments[result] = FakeJointSegment(segment, segment);

  return result;
}

bool IndexGraphStarterJoints::IsInvisible(JointSegment const & jointSegment) const
{
  return jointSegment.GetStartSegmentId() == jointSegment.GetEndSegmentId() &&
         jointSegment.GetStartSegmentId() >= kInvisibleId &&
         jointSegment.GetStartSegmentId() != std::numeric_limits<uint32_t>::max();
}

bool IndexGraphStarterJoints::IsJoint(Segment const & segment, bool fromStart)
{
  return m_starter.GetGraph().GetIndexGraph(segment.GetMwmId())
                  .IsJoint(segment.GetRoadPoint(fromStart));
}

void IndexGraphStarterJoints::Reset()
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
