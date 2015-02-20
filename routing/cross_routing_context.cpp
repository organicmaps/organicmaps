#include "cross_routing_context.hpp"

#include "../indexer/point_to_int64.hpp"

namespace routing
{

uint32_t const g_coordBits = POINT_COORD_BITS;

void OutgoingCrossNode::Save(Writer &w)
{
  w.Write(&m_nodeId, sizeof(m_nodeId));
  uint64_t const point = PointToInt64(m_point, g_coordBits);
  w.Write(&point, sizeof(point));
  w.Write(&m_outgoingIndex, sizeof(m_outgoingIndex));
}

size_t OutgoingCrossNode::Load(const Reader &r, size_t pos)
{
  r.Read(pos, &m_nodeId, sizeof(m_nodeId));
  pos += sizeof(m_nodeId);
  uint64_t point;
  r.Read(pos, &point, sizeof(point));
  m_point = Int64ToPoint(point, g_coordBits);
  pos += sizeof(point);
  r.Read(pos, &m_outgoingIndex, sizeof(m_outgoingIndex));
  return pos + sizeof(m_outgoingIndex);
}

void IngoingCrossNode::Save(Writer &w)
{
  w.Write(&m_nodeId, sizeof(m_nodeId));
  uint64_t const point = PointToInt64(m_point, g_coordBits);
  w.Write(&point, sizeof(point));
}

size_t IngoingCrossNode::Load(const Reader &r, size_t pos)
{
  r.Read(pos, &m_nodeId, sizeof(m_nodeId));
  pos += sizeof(m_nodeId);
  uint64_t point;
  r.Read(pos, &point, sizeof(point));
  m_point = Int64ToPoint(point, g_coordBits);
  return pos + sizeof(point);
}

size_t CrossRoutingContextReader::GetIndexInAdjMatrix(IngoingEdgeIteratorT ingoing, OutgoingEdgeIteratorT outgoing) const
{
  size_t ingoing_index = distance(m_ingoingNodes.cbegin(), ingoing);
  size_t outgoing_index = distance(m_outgoingNodes.cbegin(), outgoing);
  ASSERT_LESS(ingoing_index, m_ingoingNodes.size(), ("ingoing index out of range"));
  ASSERT_LESS(outgoing_index, m_outgoingNodes.size(), ("outgoing index out of range"));
  return m_outgoingNodes.size() * ingoing_index + outgoing_index;
}

void CrossRoutingContextReader::Load(Reader const & r)
{
  uint32_t size, pos = 0;
  r.Read(pos, &size, sizeof(size));
  pos += sizeof(size);
  m_ingoingNodes.resize(size);
  for (int i = 0; i < size; ++i)
  {
    pos = m_ingoingNodes[i].Load(r, pos);
  }

  r.Read(pos, &size, sizeof(size));
  pos += sizeof(size);
  m_outgoingNodes.resize(size);
  for (int i = 0; i < size; ++i)
  {
    pos = m_outgoingNodes[i].Load(r, pos);
  }
  size_t const adjMatrixSize = sizeof(WritedEdgeWeightT) * m_ingoingNodes.size() * m_outgoingNodes.size();
  mp_reader = unique_ptr<Reader>(r.CreateSubReader(pos, adjMatrixSize));
  pos += adjMatrixSize;

  uint32_t strsize;
  r.Read(pos, &strsize, sizeof(strsize));
  pos += sizeof(strsize);
  for (uint32_t i = 0; i < strsize; ++i)
  {
    vector<char> tmpString;
    r.Read(pos, &size, sizeof(size));
    pos += sizeof(size);
    vector<char> buffer(size);
    r.Read(pos, &buffer[0], size);
    m_neighborMwmList.push_back(string(&buffer[0], size));
    pos += size;
  }
}

const string & CrossRoutingContextReader::getOutgoingMwmName(size_t mwmIndex) const
{
  ASSERT(mwmIndex < m_neighborMwmList.size(), ("Routing context out of size mwm name index:", mwmIndex, m_neighborMwmList.size()));
  return m_neighborMwmList[mwmIndex];
}

pair<IngoingEdgeIteratorT, IngoingEdgeIteratorT> CrossRoutingContextReader::GetIngoingIterators() const
{
  return make_pair(m_ingoingNodes.cbegin(), m_ingoingNodes.cend());
}

pair<OutgoingEdgeIteratorT, OutgoingEdgeIteratorT> CrossRoutingContextReader::GetOutgoingIterators() const
{
  return make_pair(m_outgoingNodes.cbegin(), m_outgoingNodes.cend());
}

WritedEdgeWeightT CrossRoutingContextReader::getAdjacencyCost(IngoingEdgeIteratorT ingoing, OutgoingEdgeIteratorT outgoing) const
{
  if (!mp_reader)
    return INVALID_CONTEXT_EDGE_WEIGHT;
  WritedEdgeWeightT result;
  mp_reader->Read(GetIndexInAdjMatrix(ingoing, outgoing) * sizeof(WritedEdgeWeightT), &result, sizeof(WritedEdgeWeightT));
  return result;
}

size_t CrossRoutingContextWriter::GetIndexInAdjMatrix(IngoingEdgeIteratorT ingoing, OutgoingEdgeIteratorT outgoing) const
{
  size_t ingoing_index = distance(m_ingoingNodes.cbegin(), ingoing);
  size_t outgoing_index = distance(m_outgoingNodes.cbegin(), outgoing);
  ASSERT_LESS(ingoing_index, m_ingoingNodes.size(), ("ingoing index out of range"));
  ASSERT_LESS(outgoing_index, m_outgoingNodes.size(), ("outgoing index out of range"));
  return m_outgoingNodes.size() * ingoing_index + outgoing_index;
}

void CrossRoutingContextWriter::Save(Writer & w)
{
  uint32_t size = static_cast<uint32_t>(m_ingoingNodes.size());
  w.Write(&size, sizeof(size));
  for (int i = 0; i < size; ++i)
  {
    m_ingoingNodes[i].Save(w);
  }

  size = static_cast<uint32_t>(m_outgoingNodes.size());
  w.Write(&size, sizeof(size));
  for (int i = 0; i < size; ++i)
  {
    m_outgoingNodes[i].Save(w);
  }

  CHECK(m_adjacencyMatrix.size() == m_outgoingNodes.size()*m_ingoingNodes.size(), ());
  w.Write(&m_adjacencyMatrix[0], sizeof(m_adjacencyMatrix[0]) * m_adjacencyMatrix.size());

  size = static_cast<uint32_t>(m_neighborMwmList.size());
  w.Write(&size, sizeof(size));
  for (string const & neighbor: m_neighborMwmList)
  {
    size = static_cast<uint32_t>(neighbor.size());
    w.Write(&size, sizeof(size));
    w.Write(neighbor.c_str(), neighbor.size());
  }
}

void CrossRoutingContextWriter::addIngoingNode(size_t const nodeId, m2::PointD const & point)
{
  m_ingoingNodes.push_back(IngoingCrossNode(nodeId, point));
}

void CrossRoutingContextWriter::addOutgoingNode(size_t const nodeId, string const & targetMwm, m2::PointD const & point)
{
  auto it = find(m_neighborMwmList.begin(), m_neighborMwmList.end(), targetMwm);
  if (it == m_neighborMwmList.end())
    it = m_neighborMwmList.insert(m_neighborMwmList.end(), targetMwm);
  m_outgoingNodes.push_back(OutgoingCrossNode(nodeId, distance(m_neighborMwmList.begin(), it), point));
}

void CrossRoutingContextWriter::reserveAdjacencyMatrix()
{
  m_adjacencyMatrix.resize(m_ingoingNodes.size() * m_outgoingNodes.size(), INVALID_CONTEXT_EDGE_WEIGHT);
}

void CrossRoutingContextWriter::setAdjacencyCost(IngoingEdgeIteratorT ingoing, OutgoingEdgeIteratorT outgoin, WritedEdgeWeightT value)
{
  m_adjacencyMatrix[GetIndexInAdjMatrix(ingoing, outgoin)] = value;
}

pair<IngoingEdgeIteratorT, IngoingEdgeIteratorT> CrossRoutingContextWriter::GetIngoingIterators() const
{
  return make_pair(m_ingoingNodes.cbegin(), m_ingoingNodes.cend());
}

pair<OutgoingEdgeIteratorT, OutgoingEdgeIteratorT> CrossRoutingContextWriter::GetOutgoingIterators() const
{
  return make_pair(m_outgoingNodes.cbegin(), m_outgoingNodes.cend());
}
}
