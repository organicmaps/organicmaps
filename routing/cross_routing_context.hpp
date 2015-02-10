#pragma once

#include "../coding/file_container.hpp"

#include "../std/string.hpp"
#include "../std/vector.hpp"
#include "../std/unordered_map.hpp"
#include "../std/utility.hpp"

namespace routing
{
using WritedEdgeWeight = uint32_t;

class CrossRoutingContext
{
  std::vector<uint32_t> m_ingoingNodes;
  std::vector<std::pair<uint32_t,uint32_t> > m_outgoingNodes;
  std::vector<WritedEdgeWeight> m_adjacencyMatrix;
  std::vector<string> m_neighborMwmList;

  size_t GetIndexInAdjMatrix(std::vector<uint32_t>::const_iterator ingoing_iter, std::vector<std::pair<uint32_t,uint32_t>>::const_iterator outgoin_iter) const
  {
    size_t ingoing_index = std::distance(m_ingoingNodes.cbegin(), ingoing_iter);
    size_t outgoing_index = std::distance(m_outgoingNodes.cbegin(), outgoin_iter);
    ASSERT_LESS(ingoing_index, m_ingoingNodes.size(), ("ingoing index out of range"));
    ASSERT_LESS(outgoing_index, m_outgoingNodes.size(), ("outgoing index out of range"));
    return m_ingoingNodes.size() * ingoing_index + outgoing_index;
  }

public:

  const string & getOutgoingMwmName(size_t mwmIndex) const
  {
    ASSERT(mwmIndex < m_neighborMwmList.size(), ("Routing context out of size mwm name index:", mwmIndex, m_neighborMwmList.size()));
    return m_neighborMwmList[mwmIndex];
  }

  std::pair<std::vector<uint32_t>::const_iterator, std::vector<uint32_t>::const_iterator> GetIngoingIterators() const
  {
    return make_pair(m_ingoingNodes.cbegin(), m_ingoingNodes.cend());
  }

  std::pair<std::vector<std::pair<uint32_t,uint32_t>>::const_iterator, std::vector<std::pair<uint32_t,uint32_t>>::const_iterator> GetOutgoingIterators() const
  {
    return make_pair(m_outgoingNodes.cbegin(), m_outgoingNodes.cend());
  }

  WritedEdgeWeight getAdjacencyCost(std::vector<uint32_t>::const_iterator ingoing_iter, std::vector<std::pair<uint32_t,uint32_t>>::const_iterator outgoing_iter) const
  {
    return m_adjacencyMatrix[GetIndexInAdjMatrix(ingoing_iter, outgoing_iter)];
  }

  void Load(Reader const & r)
  {
    //Already loaded check
    if (m_adjacencyMatrix.size())
      return;

    uint32_t size, pos = 0;
    r.Read(pos, &size, sizeof(uint32_t));
    pos += sizeof(uint32_t);
    m_ingoingNodes.resize(size);
    r.Read(pos, &m_ingoingNodes[0], sizeof(uint32_t)*size);
    pos += sizeof(uint32_t) * size;

    r.Read(pos, &size, sizeof(uint32_t));
    pos += sizeof(uint32_t);
    m_outgoingNodes.resize(size);
    r.Read(pos, &(m_outgoingNodes[0]), sizeof(std::pair<uint32_t,uint32_t>) * size);
    pos += sizeof(std::pair<uint32_t,uint32_t>) * size;

    m_adjacencyMatrix.resize(m_ingoingNodes.size() * m_outgoingNodes.size());
    r.Read(pos, &m_adjacencyMatrix[0], sizeof(WritedEdgeWeight) * m_adjacencyMatrix.size());
    pos += sizeof(WritedEdgeWeight) * m_adjacencyMatrix.size();

    uint32_t strsize;
    r.Read(pos, &strsize, sizeof(uint32_t));
    pos += sizeof(uint32_t);
    for (uint32_t i = 0; i < strsize; ++i)
    {
      vector<char> tmpString;
      r.Read(pos, &size, sizeof(uint32_t));
      pos += sizeof(uint32_t);
      tmpString.resize(size);
      r.Read(pos, &tmpString[0], size);
      m_neighborMwmList.push_back(string(&tmpString[0], size));
      pos += size;
    }
  }

  //Writing part
  void Save(Writer & w)
  {
    sort(m_ingoingNodes.begin(), m_ingoingNodes.end());
    uint32_t size = m_ingoingNodes.size();
    w.Write(&size, sizeof(uint32_t));
    w.Write(&(m_ingoingNodes[0]), sizeof(uint32_t) * size);

    size = m_outgoingNodes.size();
    w.Write(&size, sizeof(uint32_t));
    w.Write(&(m_outgoingNodes[0]), sizeof(std::pair<uint32_t,uint32_t>) * size);

    CHECK(m_adjacencyMatrix.size() == m_outgoingNodes.size()*m_ingoingNodes.size(), ());
    w.Write(&(m_adjacencyMatrix[0]), sizeof(WritedEdgeWeight) * m_adjacencyMatrix.size());

    size = m_neighborMwmList.size();
    w.Write(&size, sizeof(uint32_t));
    for (string & neighbor: m_neighborMwmList)
    {
      size = neighbor.size();
      w.Write(&size, sizeof(uint32_t));
      w.Write(neighbor.c_str(), neighbor.size());
    }
  }

  void addIngoingNode(size_t const nodeId) {m_ingoingNodes.push_back(nodeId);}

  void addOutgoingNode(size_t const nodeId, string const & targetMwm)
  {
    auto it = find(m_neighborMwmList.begin(), m_neighborMwmList.end(), targetMwm);
    if (it == m_neighborMwmList.end())
      it = m_neighborMwmList.insert(m_neighborMwmList.end(), targetMwm);
    m_outgoingNodes.push_back(std::make_pair(nodeId, std::distance(m_neighborMwmList.begin(), it)));
  }

  void reserveAdjacencyMatrix() {m_adjacencyMatrix.resize(m_ingoingNodes.size() * m_outgoingNodes.size());}

  void setAdjacencyCost(std::vector<uint32_t>::const_iterator ingoing_iter, std::vector<std::pair<uint32_t,uint32_t>>::const_iterator outgoin_iter, WritedEdgeWeight value)
  {
    m_adjacencyMatrix[GetIndexInAdjMatrix(ingoing_iter, outgoin_iter)] = value;
  }
};

}
