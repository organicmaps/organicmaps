#pragma once

#include "../coding/file_container.hpp"

#include "../std/string.hpp"
#include "../std/vector.hpp"

namespace routing
{
using WritedEdgeWeightT = uint32_t;
using IngoingEdgeIteratorT = vector<uint32_t>::const_iterator;
using OutgoingEdgeIteratorT = vector<pair<uint32_t,uint32_t>>::const_iterator;
static const WritedEdgeWeightT INVALID_CONTEXT_EDGE_WEIGHT = std::numeric_limits<WritedEdgeWeightT>::max();

/// Reader class from cross context section in mwm.routing file
class CrossRoutingContextReader
{
  vector<uint32_t> m_ingoingNodes;
  vector<pair<uint32_t,uint32_t> > m_outgoingNodes;
  vector<string> m_neighborMwmList;
  unique_ptr<Reader> mp_reader = nullptr;

  size_t GetIndexInAdjMatrix(IngoingEdgeIteratorT ingoing, OutgoingEdgeIteratorT outgoing) const;

public:  
  void Load(Reader const & r);

  const string & getOutgoingMwmName(size_t mwmIndex) const;

  pair<IngoingEdgeIteratorT, IngoingEdgeIteratorT> GetIngoingIterators() const;

  pair<OutgoingEdgeIteratorT, OutgoingEdgeIteratorT> GetOutgoingIterators() const;

  WritedEdgeWeightT getAdjacencyCost(IngoingEdgeIteratorT ingoing, OutgoingEdgeIteratorT outgoing) const;
};

/// Helper class to generate cross context section in mwm.routing file
class CrossRoutingContextWriter
{
  vector<uint32_t> m_ingoingNodes;
  vector<pair<uint32_t,uint32_t> > m_outgoingNodes;
  vector<WritedEdgeWeightT> m_adjacencyMatrix;
  vector<string> m_neighborMwmList;

  size_t GetIndexInAdjMatrix(IngoingEdgeIteratorT ingoing, OutgoingEdgeIteratorT outgoing) const;

public:
  void Save(Writer & w);

  void addIngoingNode(size_t const nodeId);

  void addOutgoingNode(size_t const nodeId, string const & targetMwm);

  void reserveAdjacencyMatrix();

  void setAdjacencyCost(IngoingEdgeIteratorT ingoing, OutgoingEdgeIteratorT outgoin, WritedEdgeWeightT value);

  pair<IngoingEdgeIteratorT, IngoingEdgeIteratorT> GetIngoingIterators() const;

  pair<OutgoingEdgeIteratorT, OutgoingEdgeIteratorT> GetOutgoingIterators() const;
};
}
