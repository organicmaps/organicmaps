#pragma once

#include "../coding/file_container.hpp"

#include "../std/string.hpp"
#include "../std/vector.hpp"
#include "../std/unordered_map.hpp"
#include "../std/utility.hpp"

namespace routing
{

class CrossRoutingContext
{
  std::vector<size_t> m_ingoingNodes;
  std::vector<std::pair<size_t,size_t> > m_outgoingNodes;
  std::vector<int> m_adjacencyMatrix;
  std::vector<string> m_neighborMwmList;

public:

  const string & getOutgoingMwmName(size_t mwmIndex) const
  {
    CHECK(mwmIndex<m_neighborMwmList.size(), (mwmIndex,m_neighborMwmList.size()));
    return m_neighborMwmList[mwmIndex];
  }

  std::pair<std::vector<size_t>::const_iterator, std::vector<size_t>::const_iterator> GetIngoingIterators() const
  {
    return make_pair(m_ingoingNodes.cbegin(), m_ingoingNodes.cend());
  }

  std::pair<std::vector<std::pair<size_t,size_t>>::const_iterator, std::vector<std::pair<size_t,size_t>>::const_iterator> GetOutgoingIterators() const
  {
    return make_pair(m_outgoingNodes.cbegin(), m_outgoingNodes.cend());
  }

  int getAdjacencyCost(std::vector<size_t>::const_iterator ingoing_iter, std::vector<std::pair<size_t,size_t>>::const_iterator outgoin_iter) const
  {
    size_t ingoing_index = std::distance(m_ingoingNodes.cbegin(), ingoing_iter);
    size_t outgoing_index = std::distance(m_outgoingNodes.cbegin(), outgoin_iter);
    ASSERT_LESS(ingoing_index, m_ingoingNodes.size(), ("ingoing index out of range"));
    ASSERT_LESS(outgoing_index, m_outgoingNodes.size(), ("outgoing index out of range"));
    return m_adjacencyMatrix[m_ingoingNodes.size() * ingoing_index + outgoing_index];
  }

  void Load(Reader const & r)
  {
    if (m_adjacencyMatrix.size())
      return; //Already loaded
    size_t size, pos=0;
    r.Read(pos, &size, sizeof(size_t));
    pos += sizeof(size_t);
    m_ingoingNodes.resize(size);
    r.Read(pos, &m_ingoingNodes[0], sizeof(size_t)*size);
    pos += sizeof(size_t)*size;

    r.Read(pos, &size, sizeof(size_t));
    pos += sizeof(size_t);
    m_outgoingNodes.resize(size);
    r.Read(pos, &(m_outgoingNodes[0]), sizeof(std::pair<size_t,size_t>) * size);
    pos += sizeof(std::pair<size_t,size_t>) * size;

    m_adjacencyMatrix.resize(m_ingoingNodes.size()*m_outgoingNodes.size());
    r.Read(pos, &m_adjacencyMatrix[0], sizeof(int)*m_adjacencyMatrix.size());
    pos += sizeof(int)*m_adjacencyMatrix.size();

    size_t strsize;
    r.Read(pos, &strsize, sizeof(size_t));
    pos += sizeof(size_t);
    for (int i=0; i<strsize; ++i)
    {
      char * tmpString;
      r.Read(pos, &size, sizeof(size_t));
      pos += sizeof(size_t);
      tmpString = new char[size];
      r.Read(pos, tmpString, size);
      m_neighborMwmList.push_back(string(tmpString, size));
      pos += size;
      delete [] tmpString;
    }
  }

  //Writing part
  void Save(Writer & w)
  {
    sort(m_ingoingNodes.begin(), m_ingoingNodes.end());
    size_t size = m_ingoingNodes.size();
    w.Write(&size, sizeof(size_t));
    w.Write(&(m_ingoingNodes[0]), sizeof(size_t) * size);

    size = m_outgoingNodes.size();
    w.Write(&size, sizeof(size_t));
    w.Write(&(m_outgoingNodes[0]), sizeof(std::pair<size_t,size_t>) * size);

    CHECK(m_adjacencyMatrix.size() == m_outgoingNodes.size()*m_ingoingNodes.size(), ());
    w.Write(&(m_adjacencyMatrix[0]), sizeof(int) * m_adjacencyMatrix.size());

    size = m_neighborMwmList.size();
    w.Write(&size, sizeof(size_t));
    for (string & neighbor: m_neighborMwmList)
    {
      size = neighbor.size();
      w.Write(&size, sizeof(size_t));
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

  void setAdjacencyCost(std::vector<size_t>::const_iterator ingoing_iter, std::vector<std::pair<size_t,size_t>>::const_iterator outgoin_iter, int value)
  {
    size_t ingoing_index = std::distance(m_ingoingNodes.cbegin(), ingoing_iter);
    size_t outgoing_index = std::distance(m_outgoingNodes.cbegin(), outgoin_iter);
    ASSERT_LESS(ingoing_index, m_ingoingNodes.size(), ("ingoing index out of range"));
    ASSERT_LESS(outgoing_index, m_outgoingNodes.size(), ("outgoing index out of range"));
    m_adjacencyMatrix[m_ingoingNodes.size() * ingoing_index + outgoing_index] = value;
  }
};

}
