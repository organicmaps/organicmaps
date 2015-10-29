#pragma once

#include "coding/file_container.hpp"

#include "geometry/latlon.hpp"
#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"
#include "geometry/tree4d.hpp"

#include "std/string.hpp"
#include "std/vector.hpp"

namespace routing
{
using TWrittenNodeId = uint32_t;
using TWrittenEdgeWeight = uint32_t;

TWrittenEdgeWeight constexpr kInvalidContextEdgeNodeId = std::numeric_limits<uint32_t>::max();
TWrittenEdgeWeight constexpr kInvalidContextEdgeWeight = std::numeric_limits<TWrittenEdgeWeight>::max();
size_t constexpr kInvalidAdjacencyIndex = numeric_limits<size_t>::max();

struct IngoingCrossNode
{
  ms::LatLon m_point;
  TWrittenNodeId m_nodeId;
  size_t m_adjacencyIndex;

  IngoingCrossNode()
    : m_point(ms::LatLon::Zero())
    , m_nodeId(kInvalidContextEdgeNodeId)
    , m_adjacencyIndex(kInvalidAdjacencyIndex)
  {
  }
  IngoingCrossNode(TWrittenNodeId nodeId, ms::LatLon const & point, size_t const adjacencyIndex)
    : m_point(point), m_nodeId(nodeId), m_adjacencyIndex(adjacencyIndex)
  {
  }

  void Save(Writer & w) const;

  size_t Load(Reader const & r, size_t pos, size_t adjacencyIndex);

  m2::RectD const GetLimitRect() const { return m2::RectD(m_point.lat, m_point.lon, m_point.lat, m_point.lon); }
};

struct OutgoingCrossNode
{
  ms::LatLon m_point;
  TWrittenNodeId m_nodeId;
  unsigned char m_outgoingIndex;
  size_t m_adjacencyIndex;

  OutgoingCrossNode()
    : m_point(ms::LatLon::Zero())
    , m_nodeId(kInvalidContextEdgeNodeId)
    , m_outgoingIndex(0)
    , m_adjacencyIndex(kInvalidAdjacencyIndex)
  {
  }
  OutgoingCrossNode(TWrittenNodeId nodeId, size_t const index, ms::LatLon const & point,
                    size_t const adjacencyIndex)
    : m_point(point)
    , m_nodeId(nodeId)
    , m_outgoingIndex(static_cast<unsigned char>(index))
    , m_adjacencyIndex(adjacencyIndex)
  {
  }

  void Save(Writer & w) const;

  size_t Load(Reader const & r, size_t pos, size_t adjacencyIndex);
};

using IngoingEdgeIteratorT = vector<IngoingCrossNode>::const_iterator;
using OutgoingEdgeIteratorT = vector<OutgoingCrossNode>::const_iterator;

/// Reader class from cross context section in mwm.routing file
class CrossRoutingContextReader
{
  vector<OutgoingCrossNode> m_outgoingNodes;
  vector<string> m_neighborMwmList;
  vector<TWrittenEdgeWeight> m_adjacencyMatrix;
  m4::Tree<IngoingCrossNode> m_ingoingIndex;

public:
  void Load(Reader const & r);

  const string & GetOutgoingMwmName(OutgoingCrossNode const & mwmIndex) const;

  bool FindIngoingNodeByPoint(ms::LatLon const & point, IngoingCrossNode & node) const;

  TWrittenEdgeWeight GetAdjacencyCost(IngoingCrossNode const & ingoing,
                                     OutgoingCrossNode const & outgoing) const;

  template <class TFunctor>
  void ForEachIngoingNode(TFunctor f) const {m_ingoingIndex.ForEach(f);}

  template <class TFunctor>
  void ForEachOutgoingNode(TFunctor f) const {for_each(m_outgoingNodes.cbegin(), m_outgoingNodes.cend(), f);}
};

/// Helper class to generate cross context section in mwm.routing file
class CrossRoutingContextWriter
{
  vector<IngoingCrossNode> m_ingoingNodes;
  vector<OutgoingCrossNode> m_outgoingNodes;
  vector<TWrittenEdgeWeight> m_adjacencyMatrix;
  vector<string> m_neighborMwmList;

  size_t GetIndexInAdjMatrix(IngoingEdgeIteratorT ingoing, OutgoingEdgeIteratorT outgoing) const;

public:
  void Save(Writer & w) const;

  void AddIngoingNode(TWrittenNodeId const nodeId, ms::LatLon const & point);

  void AddOutgoingNode(TWrittenNodeId const nodeId, string const & targetMwm,
                       ms::LatLon const & point);

  void ReserveAdjacencyMatrix();

  void SetAdjacencyCost(IngoingEdgeIteratorT ingoing, OutgoingEdgeIteratorT outgoin,
                        TWrittenEdgeWeight value);

  pair<IngoingEdgeIteratorT, IngoingEdgeIteratorT> GetIngoingIterators() const;

  pair<OutgoingEdgeIteratorT, OutgoingEdgeIteratorT> GetOutgoingIterators() const;
};
}
