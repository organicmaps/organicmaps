#pragma once

#include "routing/fake_vertex.hpp"
#include "routing/segment.hpp"

#include <map>
#include <set>
#include <utility>
#include <vector>

namespace routing
{
class FakeGraph
{
public:
  // Adds vertex with no connections.
  void AddStandaloneVertex(Segment const & newSegment, FakeVertex const & newVertex);
  // Adds vertex. Connects newSegment to existentSegment. Adds ingoing and
  // outgoing edges, fills segment to vertex mapping. Fills real to fake and fake to real
  // mapping if isPartOfReal is true.
  void AddVertex(Segment const & existentSegment, Segment const & newSegment, FakeVertex const & newVertex,
                 bool isOutgoing, bool isPartOfReal, Segment const & real);
  // Adds connection from existent fake segment |from| to existent fake segment |to|
  void AddConnection(Segment const & from, Segment const & to);
  // Merges |rhs| into this.
  void Append(FakeGraph const & rhs);

  // Returns Vertex which corresponds to |segment|. Segment must be a part of the fake graph.
  FakeVertex const & GetVertex(Segment const & segment) const;
  // Returns outgoing/ingoing edges set for specified segment.
  std::set<Segment> const & GetEdges(Segment const & segment, bool isOutgoing) const;
  size_t GetSize() const;
  std::set<Segment> const & GetFake(Segment const & real) const;

  bool FindReal(Segment const & fake, Segment & real) const;
  bool FindSegment(FakeVertex const & vertex, Segment & segment) const;

  // Connects loop to the PartOfReal segments on the Guides track.
  void ConnectLoopToGuideSegments(FakeVertex const & loop, Segment const & guidesSegment,
                                  LatLonWithAltitude const & guidesSegmentFrom,
                                  LatLonWithAltitude const & guidesSegmentTo,
                                  std::vector<std::pair<FakeVertex, Segment>> const & partsOfReal);

private:
  void ConnectLoopToExistentPartsOfReal(FakeVertex const & loop, Segment const & guidesSegment,
                                        Segment const & directedGuidesSegment);

  // Key is fake segment, value is set of outgoing fake segments.
  std::map<Segment, std::set<Segment>> m_outgoing;
  // Key is fake segment, value is set of ingoing fake segments.
  std::map<Segment, std::set<Segment>> m_ingoing;
  // Key is fake segment, value is fake vertex which corresponds fake segment.
  std::map<Segment, FakeVertex> m_segmentToVertex;
  // Key is fake vertex, value is fake segment which corresponds fake vertex.
  std::map<FakeVertex, Segment> m_vertexToSegment;
  // Key is fake segment of type PartOfReal, value is corresponding real segment.
  std::map<Segment, Segment> m_fakeToReal;
  // Key is real segment, value is set of fake segments with type PartOfReal
  // which are parts of this real segment.
  std::map<Segment, std::set<Segment>> m_realToFake;
  // To return empty set by const reference.
  std::set<Segment> const kEmptySet = std::set<Segment>();
};
}  // namespace routing
