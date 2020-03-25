#pragma once

#include "routing/fake_feature_ids.hpp"
#include "routing/fake_vertex.hpp"
#include "routing/latlon_with_altitude.hpp"

#include "base/assert.hpp"
#include "base/stl_helpers.hpp"
#include "base/stl_iterator.hpp"

#include <algorithm>
#include <cstdint>
#include <map>
#include <set>
#include <utility>

namespace routing
{
template <typename SegmentType, typename VertexType>
class FakeGraph final
{
public:
  // Adds vertex with no connections.
  void AddStandaloneVertex(SegmentType const & newSegment, VertexType const & newVertex)
  {
    m_segmentToVertex[newSegment] = newVertex;
    m_vertexToSegment[newVertex] = newSegment;
  }

  // Adds vertex. Connects newSegment to existentSegment. Adds ingoing and
  // outgoing edges, fills segment to vertex mapping. Fills real to fake and fake to real
  // mapping if isPartOfReal is true.
  void AddVertex(SegmentType const & existentSegment, SegmentType const & newSegment,
                 VertexType const & newVertex, bool isOutgoing, bool isPartOfReal,
                 SegmentType const & real)
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

  // Adds connection from existent fake segment |from| to existent fake segment |to|
  void AddConnection(SegmentType const & from, SegmentType const & to)
  {
    ASSERT(m_segmentToVertex.find(from) != m_segmentToVertex.end(),
           ("Segment", from, "does not exist in fake graph."));
    ASSERT(m_segmentToVertex.find(to) != m_segmentToVertex.end(),
           ("Segment", to, "does not exist in fake graph."));
    m_outgoing[from].insert(to);
    m_ingoing[to].insert(from);
  }

  // Merges |rhs| into this.
  void Append(FakeGraph const & rhs)
  {
    std::map<SegmentType, VertexType> intersection;
    typename std::map<SegmentType, VertexType>::iterator intersectionIt(intersection.begin());

    std::set_intersection(m_segmentToVertex.begin(), m_segmentToVertex.end(),
                          rhs.m_segmentToVertex.begin(), rhs.m_segmentToVertex.end(),
                          std::inserter(intersection, intersectionIt),
                          base::LessBy(&std::pair<SegmentType, VertexType>::first));

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

  // Returns Vertex which corresponds to |segment|. Segment must be a part of the fake graph.
  VertexType const & GetVertex(SegmentType const & segment) const
  {
    auto const it = m_segmentToVertex.find(segment);
    CHECK(it != m_segmentToVertex.end(), ("Vertex for invalid fake segment requested."));
    return it->second;
  }

  // Returns outgoing/ingoing edges set for specified segment.
  std::set<SegmentType> const & GetEdges(SegmentType const & segment, bool isOutgoing) const
  {
    auto const & adjacentEdges = isOutgoing ? m_outgoing : m_ingoing;

    auto const it = adjacentEdges.find(segment);
    if (it != adjacentEdges.end())
      return it->second;

    return kEmptySet;
  }

  size_t GetSize() const { return m_segmentToVertex.size(); }

  std::set<SegmentType> const & GetFake(SegmentType const & real) const
  {
    auto const it = m_realToFake.find(real);
    if (it != m_realToFake.end())
      return it->second;

    return kEmptySet;
  }

  bool FindReal(SegmentType const & fake, SegmentType & real) const
  {
    auto const it = m_fakeToReal.find(fake);
    if (it == m_fakeToReal.end())
      return false;

    real = it->second;
    return true;
  }

  bool FindSegment(VertexType const & vertex, SegmentType & segment) const
  {
    auto const it = m_vertexToSegment.find(vertex);
    if (it == m_vertexToSegment.end())
      return false;

    segment = it->second;
    return true;
  }

  // Connects loop to the VertexType::Type::PartOfReal segments on the Guides track.
  void ConnectLoopToGuideSegments(
      FakeVertex const & loop, SegmentType const & guidesSegment,
      LatLonWithAltitude const & guidesSegmentFrom, LatLonWithAltitude const & guidesSegmentTo,
      std::vector<std::pair<FakeVertex, SegmentType>> const & partsOfReal)
  {
    auto itLoop = m_vertexToSegment.find(loop);
    CHECK(itLoop != m_vertexToSegment.end(), (loop));

    auto const & loopSegment = itLoop->second;
    auto const & loopPoint = loop.GetPointTo();

    auto const backwardReal =
        SegmentType(guidesSegment.GetMwmId(), guidesSegment.GetFeatureId(),
                    guidesSegment.GetSegmentIdx(), !guidesSegment.IsForward());

    ConnectLoopToExistentPartsOfReal(loop, guidesSegment, backwardReal);

    for (auto & [newVertex, newSegment] : partsOfReal)
    {
      auto const [it, inserted] = m_vertexToSegment.emplace(newVertex, newSegment);
      SegmentType const & segment = it->second;

      SegmentType const & directedReal = (newVertex.GetPointFrom() == guidesSegmentTo.GetLatLon() ||
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

      CHECK((newVertex.GetPointFrom() == loopPoint || newVertex.GetPointTo() == loopPoint),
            (newVertex, loopPoint));

      auto const & to = (newVertex.GetPointTo() == loopPoint) ? loopSegment : segment;
      auto const & from = (newVertex.GetPointFrom() == loopPoint) ? loopSegment : segment;

      m_ingoing[to].insert(from);
      m_outgoing[from].insert(to);
    }
  }

private:
  void ConnectLoopToExistentPartsOfReal(FakeVertex const & loop, SegmentType const & guidesSegment,
                                        SegmentType const & directedGuidesSegment)
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

  // Key is fake segment, value is set of outgoing fake segments.
  std::map<SegmentType, std::set<SegmentType>> m_outgoing;
  // Key is fake segment, value is set of ingoing fake segments.
  std::map<SegmentType, std::set<SegmentType>> m_ingoing;
  // Key is fake segment, value is fake vertex which corresponds fake segment.
  std::map<SegmentType, VertexType> m_segmentToVertex;
  // Key is fake vertex, value is fake segment which corresponds fake vertex.
  std::map<VertexType, SegmentType> m_vertexToSegment;
  // Key is fake segment of type VertexType::Type::PartOfReal, value is corresponding real segment.
  std::map<SegmentType, SegmentType> m_fakeToReal;
  // Key is real segment, value is set of fake segments with type VertexType::Type::PartOfReal
  // which are parts of this real segment.
  std::map<SegmentType, std::set<SegmentType>> m_realToFake;
  // To return empty set by const reference.
  std::set<SegmentType> const kEmptySet = std::set<SegmentType>();
};
}  // namespace routing
