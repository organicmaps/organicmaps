#include "routing/cross_routing_context.hpp"

#include "indexer/point_to_int64.hpp"

namespace routing
{

static uint32_t const g_coordBits = POINT_COORD_BITS;

void OutgoingCrossNode::Save(Writer &w) const
{
  uint64_t point = PointToInt64(m_point, g_coordBits);
  char buff[sizeof(m_nodeId) + sizeof(point) + sizeof(m_outgoingIndex)];
  *reinterpret_cast<decltype(m_nodeId) *>(&buff[0]) = m_nodeId;
  *reinterpret_cast<decltype(point) *>(&(buff[sizeof(m_nodeId)])) = point;
  *reinterpret_cast<decltype(m_outgoingIndex) *>(&(buff[sizeof(m_nodeId) + sizeof(point)])) = m_outgoingIndex;
  w.Write(buff, sizeof(buff));

}

size_t OutgoingCrossNode::Load(const Reader &r, size_t pos)
{
  char buff[sizeof(m_nodeId) + sizeof(uint64_t) + sizeof(m_outgoingIndex)];
  r.Read(pos, buff, sizeof(buff));
  m_nodeId = *reinterpret_cast<decltype(m_nodeId) *>(&buff[0]);
  m_point = Int64ToPoint(*reinterpret_cast<uint64_t *>(&(buff[sizeof(m_nodeId)])), g_coordBits);
  m_outgoingIndex = *reinterpret_cast<decltype(m_outgoingIndex) *>(&(buff[sizeof(m_nodeId) + sizeof(uint64_t)]));
  return pos + sizeof(buff);
}

void IngoingCrossNode::Save(Writer &w) const
{
  uint64_t point = PointToInt64(m_point, g_coordBits);
  char buff[sizeof(m_nodeId) + sizeof(point)];
  *reinterpret_cast<decltype(m_nodeId) *>(&buff[0]) = m_nodeId;
  *reinterpret_cast<decltype(point) *>(&(buff[sizeof(m_nodeId)])) = point;
  w.Write(buff, sizeof(buff));
}

size_t IngoingCrossNode::Load(const Reader &r, size_t pos)
{
  char buff[sizeof(m_nodeId) + sizeof(uint64_t)];
  r.Read(pos, buff, sizeof(buff));
  m_nodeId = *reinterpret_cast<decltype(m_nodeId) *>(&buff[0]);
  m_point = Int64ToPoint(*reinterpret_cast<uint64_t *>(&(buff[sizeof(m_nodeId)])), g_coordBits);
  return pos + sizeof(buff);
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

  for (auto & node : m_ingoingNodes)
    pos = node.Load(r, pos);

  r.Read(pos, &size, sizeof(size));
  pos += sizeof(size);
  m_outgoingNodes.resize(size);

  for (auto & node : m_outgoingNodes)
    pos = node.Load(r, pos);

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

const string & CrossRoutingContextReader::GetOutgoingMwmName(
    OutgoingCrossNode const & outgoingNode) const
{
  ASSERT(outgoingNode.m_outgoingIndex < m_neighborMwmList.size(),
         ("Routing context out of size mwm name index:", outgoingNode.m_outgoingIndex,
          m_neighborMwmList.size()));
  return m_neighborMwmList[outgoingNode.m_outgoingIndex];
}

pair<IngoingEdgeIteratorT, IngoingEdgeIteratorT> CrossRoutingContextReader::GetIngoingIterators() const
{
  return make_pair(m_ingoingNodes.cbegin(), m_ingoingNodes.cend());
}

pair<OutgoingEdgeIteratorT, OutgoingEdgeIteratorT> CrossRoutingContextReader::GetOutgoingIterators() const
{
  return make_pair(m_outgoingNodes.cbegin(), m_outgoingNodes.cend());
}

WritedEdgeWeightT CrossRoutingContextReader::GetAdjacencyCost(IngoingEdgeIteratorT ingoing, OutgoingEdgeIteratorT outgoing) const
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

void CrossRoutingContextWriter::Save(Writer & w) const
{
  uint32_t size = static_cast<uint32_t>(m_ingoingNodes.size());
  w.Write(&size, sizeof(size));
  for (auto const & node : m_ingoingNodes)
    node.Save(w);

  size = static_cast<uint32_t>(m_outgoingNodes.size());
  w.Write(&size, sizeof(size));

  for (auto const & node : m_outgoingNodes)
    node.Save(w);

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

void CrossRoutingContextWriter::AddIngoingNode(size_t const nodeId, m2::PointD const & point)
{
  m_ingoingNodes.push_back(IngoingCrossNode(nodeId, point));
}

void CrossRoutingContextWriter::AddOutgoingNode(size_t const nodeId, string const & targetMwm, m2::PointD const & point)
{
  auto it = find(m_neighborMwmList.begin(), m_neighborMwmList.end(), targetMwm);
  if (it == m_neighborMwmList.end())
    it = m_neighborMwmList.insert(m_neighborMwmList.end(), targetMwm);
  m_outgoingNodes.push_back(OutgoingCrossNode(nodeId, distance(m_neighborMwmList.begin(), it), point));
}

void CrossRoutingContextWriter::ReserveAdjacencyMatrix()
{
  m_adjacencyMatrix.resize(m_ingoingNodes.size() * m_outgoingNodes.size(), INVALID_CONTEXT_EDGE_WEIGHT);
}

void CrossRoutingContextWriter::SetAdjacencyCost(IngoingEdgeIteratorT ingoing, OutgoingEdgeIteratorT outgoin, WritedEdgeWeightT value)
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
