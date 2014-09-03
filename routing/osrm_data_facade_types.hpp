#pragma once

#include "../base/assert.hpp"
#include "../std/string.hpp"
#include "../std/vector.hpp"
#include "../std/fstream.hpp"
#include "../std/unordered_map.hpp"

namespace routing
{

#define SPECIAL_OSRM_NODE_ID  -1
typedef uint32_t OsrmNodeIdT;

class OsrmFtSegMapping
{
public:

  struct FtSeg
  {
    uint32_t m_fid;
    uint32_t m_pointStart;
    uint32_t m_pointEnd;
  };

  typedef vector<FtSeg> FtSegVectorT;

  void Save(string const & filename)
  {
    ofstream stream;
    stream.open(filename);

    if (!stream.is_open())
      return;

    uint32_t count = m_osrm2FtSeg.size();
    stream.write((char*)&count, sizeof(count));

    for (uint32_t i = 0; i < count; ++i)
    {
      auto it = m_osrm2FtSeg.find(i);
      CHECK(it != m_osrm2FtSeg.end(), ());
      FtSegVectorT const & v = it->second;

      uint32_t vc = v.size();
      stream.write((char*)&vc, sizeof(vc));
      stream.write((char*)v.data(), sizeof(OsrmFtSegMapping::FtSeg) * vc);
    }

    stream.close();
  }

  void Load(string const & filename)
  {
    ifstream stream;
    stream.open(filename);

    if (!stream.is_open())
      return;

    uint32_t count = 0;
    stream.read((char*)&count, sizeof(count));

    for (uint32_t i = 0; i < count; ++i)
    {
      uint32_t vc = 0;
      stream.read((char*)&vc, sizeof(vc));

      FtSegVectorT v;
      v.resize(vc);
      stream.read((char*)v.data(), sizeof(FtSeg) * vc);

      m_osrm2FtSeg[i] = v;
    }

    stream.close();
  }

  void Append(OsrmNodeIdT osrmNodeId, FtSegVectorT & data)
  {
    ASSERT(m_osrm2FtSeg.find(osrmNodeId) == m_osrm2FtSeg.end(), ());
    m_osrm2FtSeg[osrmNodeId] = data;
  }

  FtSegVectorT const & GetSegVector(OsrmNodeIdT nodeId) const
  {
    auto it = m_osrm2FtSeg.find(nodeId);
    ASSERT(it != m_osrm2FtSeg.end(), ());
    return it->second;
  }

  void GetOsrmNode(FtSeg const & seg, OsrmNodeIdT & forward, OsrmNodeIdT & reverse) const
  {
    forward = SPECIAL_OSRM_NODE_ID;
    reverse = SPECIAL_OSRM_NODE_ID;

    for (unordered_map<OsrmNodeIdT, FtSegVectorT>::const_iterator it = m_osrm2FtSeg.begin(); it != m_osrm2FtSeg.end(); ++it)
    {
      for (auto s : it->second)
      {
        if (s.m_pointStart <= s.m_pointEnd)
        {
          if (seg.m_pointStart >= s.m_pointStart && seg.m_pointEnd <= s.m_pointEnd)
            forward = it->first;
        }
        else
        {
          if (seg.m_pointStart >= s.m_pointEnd && seg.m_pointEnd <= s.m_pointStart)
            reverse = it->first;
        }
      }
    }
  }

private:
  unordered_map<OsrmNodeIdT, FtSegVectorT> m_osrm2FtSeg;
};

}
