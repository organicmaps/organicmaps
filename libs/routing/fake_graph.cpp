#include "routing/fake_graph.hpp"

#include "routing/fake_feature_ids.hpp"
#include "routing/latlon_with_altitude.hpp"

#include "base/assert.hpp"
#include "base/stl_helpers.hpp"

#include <algorithm>

namespace routing
{
void FakeGraph::AddStandaloneVertex(Segment const & newSegment, FakeVertex const & newVertex)
{
  m_segmentToVertex[newSegment] = newVertex;
  m_vertexToSegment[newVertex] = newSegment;
}

void FakeGraph::AddVertex(Segment const & existentSegment, Segment const & newSegment, FakeVertex const & newVertex,
                          bool isOutgoing, bool isPartOfReal, Segment const & real)
{
  AddStandaloneVertex(newSegment, newVertex);
  auto const & segmentFrom = isOutgoing ? existentSegment : newSegment;
  auto const & segmentTo = isOutgoing ? newSegment : existentSegment;
  m_outgoing[segmentFrom].insert(segmentTo);
  m_ingoing[segmentTo].insert(segmentFrom);
  if (isPartOfReal)
  {
    m_realToFake[real].insert(newSegment);
    m_fakeToReal[newSegment] = real;
  }
}

void FakeGraph::AddConnection(Segment const & from, Segment const & to)
{
  ASSERT(m_segmentToVertex.find(from) != m_segmentToVertex.end(), ("Segment", from, "does not exist in fake graph."));
  ASSERT(m_segmentToVertex.find(to) != m_segmentToVertex.end(), ("Segment", to, "does not exist in fake graph."));
  m_outgoing[from].insert(to);
  m_ingoing[to].insert(from);
}

void FakeGraph::Append(FakeGraph const & rhs)
{
  std::map<Segment, FakeVertex> intersection;
  typename std::map<Segment, FakeVertex>::iterator intersectionIt(intersection.begin());

  std::set_intersection(m_segmentToVertex.begin(), m_segmentToVertex.end(), rhs.m_segmentToVertex.begin(),
                        rhs.m_segmentToVertex.end(), std::inserter(intersection, intersectionIt),
                        base::LessBy(&std::pair<Segment, FakeVertex>::first));

  size_t countEqual = 0;
  for (auto const & segmentVertexPair : intersection)
  {
    auto const it = m_fakeToReal.find(segmentVertexPair.first);
    if (it == m_fakeToReal.cend() || !FakeFeatureIds::IsGuidesFeature(it->second.GetFeatureId()))
      countEqual++;
  }

  CHECK_EQUAL(countEqual, 0, ("Fake segments are not unique."));

  m_segmentToVertex.insert(rhs.m_segmentToVertex.begin(), rhs.m_segmentToVertex.end());
  m_vertexToSegment.insert(rhs.m_vertexToSegment.begin(), rhs.m_vertexToSegment.end());

  for (auto const & kv : rhs.m_outgoing)
    m_outgoing[kv.first].insert(kv.second.begin(), kv.second.end());

  for (auto const & kv : rhs.m_ingoing)
    m_ingoing[kv.first].insert(kv.second.begin(), kv.second.end());

  for (auto const & kv : rhs.m_realToFake)
    m_realToFake[kv.first].insert(kv.second.begin(), kv.second.end());

  m_fakeToReal.insert(rhs.m_fakeToReal.begin(), rhs.m_fakeToReal.end());
}

FakeVertex const & FakeGraph::GetVertex(Segment const & segment) const
{
  auto const it = m_segmentToVertex.find(segment);
  CHECK(it != m_segmentToVertex.end(), ("Vertex for invalid fake segment requested."));
  return it->second;
}

std::set<Segment> const & FakeGraph::GetEdges(Segment const & segment, bool isOutgoing) const
{
  auto const & adjacentEdges = isOutgoing ? m_outgoing : m_ingoing;

  auto const it = adjacentEdges.find(segment);
  if (it != adjacentEdges.end())
    return it->second;

  return kEmptySet;
}

size_t FakeGraph::GetSize() const
{
  return m_segmentToVertex.size();
}

std::set<Segment> const & FakeGraph::GetFake(Segment const & real) const
{
  auto const it = m_realToFake.find(real);
  if (it != m_realToFake.end())
    return it->second;

  return kEmptySet;
}

bool FakeGraph::FindReal(Segment const & fake, Segment & real) const
{
  auto const it = m_fakeToReal.find(fake);
  if (it == m_fakeToReal.end())
    return false;

  real = it->second;
  return true;
}

bool FakeGraph::FindSegment(FakeVertex const & vertex, Segment & segment) const
{
  auto const it = m_vertexToSegment.find(vertex);
  if (it == m_vertexToSegment.end())
    return false;

  segment = it->second;
  return true;
}

void FakeGraph::ConnectLoopToGuideSegments(FakeVertex const & loop, Segment const & guidesSegment,
                                           LatLonWithAltitude const & guidesSegmentFrom,
                                           LatLonWithAltitude const & guidesSegmentTo,
                                           std::vector<std::pair<FakeVertex, Segment>> const & partsOfReal)
{
  auto itLoop = m_vertexToSegment.find(loop);
  CHECK(itLoop != m_vertexToSegment.end(), (loop));

  auto const & loopSegment = itLoop->second;
  auto const & loopPoint = loop.GetPointTo();

  auto const backwardReal = Segment(guidesSegment.GetMwmId(), guidesSegment.GetFeatureId(),
                                    guidesSegment.GetSegmentIdx(), !guidesSegment.IsForward());

  ConnectLoopToExistentPartsOfReal(loop, guidesSegment, backwardReal);

  for (auto & [newVertex, newSegment] : partsOfReal)
  {
    auto const [it, inserted] = m_vertexToSegment.emplace(newVertex, newSegment);
    Segment const & segment = it->second;

    Segment const & directedReal = (newVertex.GetPointFrom() == guidesSegmentTo.GetLatLon() ||
                                    newVertex.GetPointTo() == guidesSegmentFrom.GetLatLon())
                                     ? backwardReal
                                     : guidesSegment;

    m_realToFake[directedReal].insert(segment);

    if (inserted)
    {
      m_fakeToReal[segment] = directedReal;
      m_segmentToVertex[segment] = newVertex;
      m_vertexToSegment[newVertex] = segment;
    }

    CHECK((newVertex.GetPointFrom() == loopPoint || newVertex.GetPointTo() == loopPoint), (newVertex, loopPoint));

    auto const & to = (newVertex.GetPointTo() == loopPoint) ? loopSegment : segment;
    auto const & from = (newVertex.GetPointFrom() == loopPoint) ? loopSegment : segment;

    m_ingoing[to].insert(from);
    m_outgoing[from].insert(to);
  }
}

void FakeGraph::ConnectLoopToExistentPartsOfReal(FakeVertex const & loop, Segment const & guidesSegment,
                                                 Segment const & directedGuidesSegment)
{
  auto const & loopSegment = m_vertexToSegment[loop];
  auto const & loopPoint = loop.GetPointTo();

  for (auto const & real : {guidesSegment, directedGuidesSegment})
  {
    for (auto const & partOfReal : GetFake(real))
    {
      auto const & partOfRealVertex = m_segmentToVertex[partOfReal];
      if (partOfRealVertex.GetPointTo() == loopPoint)
      {
        m_outgoing[partOfReal].insert(loopSegment);
        m_ingoing[loopSegment].insert(partOfReal);
      }

      if (partOfRealVertex.GetPointFrom() == loopPoint)
      {
        m_outgoing[loopSegment].insert(partOfReal);
        m_ingoing[partOfReal].insert(loopSegment);
      }
    }
  }
}
}  // namespace routing
