#include "routing/cross_routing_context.hpp"

#include "indexer/point_to_int64.hpp"

namespace
{
uint32_t constexpr kCoordBits = POINT_COORD_BITS;
}  // namespace

namespace routing
{
void OutgoingCrossNode::Save(Writer & w) const
{
  uint64_t point = PointToInt64(m2::PointD(m_point.lon, m_point.lat), kCoordBits);
  char buff[sizeof(m_nodeId) + sizeof(point) + sizeof(m_outgoingIndex)];
  *reinterpret_cast<decltype(m_nodeId) *>(&buff[0]) = m_nodeId;
  *reinterpret_cast<decltype(point) *>(&(buff[sizeof(m_nodeId)])) = point;
  *reinterpret_cast<decltype(m_outgoingIndex) *>(&(buff[sizeof(m_nodeId) + sizeof(point)])) = m_outgoingIndex;
  w.Write(buff, sizeof(buff));
}

size_t OutgoingCrossNode::Load(const Reader & r, size_t pos, size_t adjacencyIndex)
{
  char buff[sizeof(m_nodeId) + sizeof(uint64_t) + sizeof(m_outgoingIndex)];
  r.Read(pos, buff, sizeof(buff));
  m_nodeId = *reinterpret_cast<decltype(m_nodeId) *>(&buff[0]);
  m2::PointD bufferPoint = Int64ToPoint(*reinterpret_cast<uint64_t *>(&(buff[sizeof(m_nodeId)])), kCoordBits);
  m_point = ms::LatLon(bufferPoint.y, bufferPoint.x);
  m_outgoingIndex = *reinterpret_cast<decltype(m_outgoingIndex) *>(&(buff[sizeof(m_nodeId) + sizeof(uint64_t)]));
  m_adjacencyIndex = adjacencyIndex;
  return pos + sizeof(buff);
}

void IngoingCrossNode::Save(Writer & w) const
{
  uint64_t point = PointToInt64(m2::PointD(m_point.lon, m_point.lat), kCoordBits);
  char buff[sizeof(m_nodeId) + sizeof(point)];
  *reinterpret_cast<decltype(m_nodeId) *>(&buff[0]) = m_nodeId;
  *reinterpret_cast<decltype(point) *>(&(buff[sizeof(m_nodeId)])) = point;
  w.Write(buff, sizeof(buff));
}

size_t IngoingCrossNode::Load(const Reader & r, size_t pos, size_t adjacencyIndex)
{
  char buff[sizeof(m_nodeId) + sizeof(uint64_t)];
  r.Read(pos, buff, sizeof(buff));
  m_nodeId = *reinterpret_cast<decltype(m_nodeId) *>(&buff[0]);
  m2::PointD bufferPoint = Int64ToPoint(*reinterpret_cast<uint64_t *>(&(buff[sizeof(m_nodeId)])), kCoordBits);
  m_point = ms::LatLon(bufferPoint.y, bufferPoint.x);
  m_adjacencyIndex = adjacencyIndex;
  return pos + sizeof(buff);
}

void CrossRoutingContextReader::Load(Reader const & r)
{
  size_t pos = 0;

  uint32_t size, ingoingSize;
  r.Read(pos, &ingoingSize, sizeof(ingoingSize));
  pos += sizeof(ingoingSize);

  for (size_t i = 0; i < ingoingSize; ++i)
  {
    IngoingCrossNode node;
    pos = node.Load(r, pos, i);
    m_ingoingIndex.Add(node);
  }

  r.Read(pos, &size, sizeof(size));
  pos += sizeof(size);
  m_outgoingNodes.resize(size);

  for (size_t i = 0; i < size; ++i)
  {
    pos = m_outgoingNodes[i].Load(r, pos, i);
    m_outgoingIndex.Add(m_outgoingNodes[i]);
  }

  size_t adjacencySize = ingoingSize * m_outgoingNodes.size();
  size_t const adjMatrixSize = sizeof(TWrittenEdgeWeight) * adjacencySize;
  m_adjacencyMatrix.resize(adjacencySize);
  r.Read(pos, &m_adjacencyMatrix[0], adjMatrixSize);
  pos += adjMatrixSize;

  uint32_t strsize;
  r.Read(pos, &strsize, sizeof(strsize));
  pos += sizeof(strsize);
  for (uint32_t i = 0; i < strsize; ++i)
  {
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

TWrittenEdgeWeight CrossRoutingContextReader::GetAdjacencyCost(
    IngoingCrossNode const & ingoing, OutgoingCrossNode const & outgoing) const
{
  if (ingoing.m_adjacencyIndex == kInvalidAdjacencyIndex ||
      outgoing.m_adjacencyIndex == kInvalidAdjacencyIndex)
    return kInvalidContextEdgeWeight;

  size_t cost_index = m_outgoingNodes.size() * ingoing.m_adjacencyIndex + outgoing.m_adjacencyIndex;
  return cost_index < m_adjacencyMatrix.size() ? m_adjacencyMatrix[cost_index] : kInvalidContextEdgeWeight;
}

m2::RectD CrossRoutingContextReader::GetMwmCrossingNodeEqualityRect(ms::LatLon const & point) const
{
  double constexpr kMwmCrossingNodeEqualityDegrees = kMwmCrossingNodeEqualityMeters * MercatorBounds::degreeInMetres;
  return m2::RectD(point.lat - kMwmCrossingNodeEqualityDegrees,
                   point.lon - kMwmCrossingNodeEqualityDegrees,
                   point.lat + kMwmCrossingNodeEqualityDegrees,
                   point.lon + kMwmCrossingNodeEqualityDegrees);
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

  CHECK(m_adjacencyMatrix.size() == m_outgoingNodes.size() * m_ingoingNodes.size(), ());
  w.Write(&m_adjacencyMatrix[0], sizeof(m_adjacencyMatrix[0]) * m_adjacencyMatrix.size());

  size = static_cast<uint32_t>(m_neighborMwmList.size());
  w.Write(&size, sizeof(size));
  for (string const & neighbor : m_neighborMwmList)
  {
    size = static_cast<uint32_t>(neighbor.size());
    w.Write(&size, sizeof(size));
    w.Write(neighbor.c_str(), neighbor.size());
  }
}

void CrossRoutingContextWriter::AddIngoingNode(TWrittenNodeId const nodeId, ms::LatLon const & point)
{
  size_t const adjIndex = m_ingoingNodes.size();
  m_ingoingNodes.emplace_back(nodeId, point, adjIndex);
}

void CrossRoutingContextWriter::AddOutgoingNode(TWrittenNodeId const nodeId, string const & targetMwm,
                                                ms::LatLon const & point)
{
  size_t const adjIndex = m_outgoingNodes.size();
  auto it = find(m_neighborMwmList.begin(), m_neighborMwmList.end(), targetMwm);
  if (it == m_neighborMwmList.end())
    it = m_neighborMwmList.insert(m_neighborMwmList.end(), targetMwm);
  m_outgoingNodes.emplace_back(nodeId, distance(m_neighborMwmList.begin(), it), point, adjIndex);
}

void CrossRoutingContextWriter::ReserveAdjacencyMatrix()
{
  m_adjacencyMatrix.resize(m_ingoingNodes.size() * m_outgoingNodes.size(),
                           kInvalidContextEdgeWeight);
}

void CrossRoutingContextWriter::SetAdjacencyCost(IngoingEdgeIteratorT ingoing,
                                                 OutgoingEdgeIteratorT outgoing,
                                                 TWrittenEdgeWeight value)
{
  size_t const index = m_outgoingNodes.size() * ingoing->m_adjacencyIndex + outgoing->m_adjacencyIndex;
  ASSERT_LESS(index, m_adjacencyMatrix.size(), ());
  m_adjacencyMatrix[index] = value;
}

pair<IngoingEdgeIteratorT, IngoingEdgeIteratorT> CrossRoutingContextWriter::GetIngoingIterators()
    const
{
  return make_pair(m_ingoingNodes.cbegin(), m_ingoingNodes.cend());
}

pair<OutgoingEdgeIteratorT, OutgoingEdgeIteratorT> CrossRoutingContextWriter::GetOutgoingIterators()
    const
{
  return make_pair(m_outgoingNodes.cbegin(), m_outgoingNodes.cend());
}
}
