#pragma once

#include "../base/assert.hpp"
#include "../base/logging.hpp"

#include "../std/string.hpp"
#include "../std/vector.hpp"
#include "../std/fstream.hpp"
#include "../std/unordered_map.hpp"
#include "../std/sstream.hpp"


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

    FtSeg()
      : m_fid(-1), m_pointStart(-1), m_pointEnd(-1)
    {

    }

    FtSeg(uint32_t fid, uint32_t ps, uint32_t pe)
      : m_fid(fid), m_pointStart(ps), m_pointEnd(pe)
    {
    }

    bool Merge(FtSeg const & other)
    {
      if (other.m_fid != m_fid)
        return false;

      if (other.m_pointEnd > other.m_pointStart)
      {
        if (m_pointStart >= other.m_pointEnd)
          return false;

        if (other.m_pointStart == m_pointEnd)
        {
          m_pointEnd = other.m_pointEnd;
          return true;
        }
      }else
      {
        if (m_pointEnd >= other.m_pointStart)
          return false;

        if (other.m_pointEnd == m_pointStart)
        {
          m_pointStart = other.m_pointStart;
          return true;
        }
      }

      return false;
    }

    friend string DebugPrint(FtSeg const & seg)
    {
      stringstream ss;
      ss << "{ fID = " << seg.m_fid <<
            "; pStart = " << seg.m_pointStart <<
            "; pEnd = " << seg.m_pointEnd << " }";
      return ss.str();
    }
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
    if (it != m_osrm2FtSeg.end())
      return it->second;
    else
      return m_empty;
  }

  void DumpSegmentsByFID(uint32_t fID) const
  {
    LOG(LINFO, ("Dump segments for feature:", fID));

    for (auto it = m_osrm2FtSeg.begin(); it != m_osrm2FtSeg.end(); ++it)
      for (auto const & s : it->second)
        if (s.m_fid == fID)
          LOG(LINFO, (s));
  }

  void DumpSgementByNode(uint32_t nodeId)
  {
    LOG(LINFO, ("Dump segments for node:", nodeId));

    auto it = m_osrm2FtSeg.find(nodeId);
    if (it == m_osrm2FtSeg.end())
      return;

    for (auto const & s : it->second)
      LOG(LINFO, (s));
  }

  void GetOsrmNode(FtSeg const & seg, OsrmNodeIdT & forward, OsrmNodeIdT & reverse) const
  {
    ASSERT_LESS(seg.m_pointStart, seg.m_pointEnd, ());

    forward = SPECIAL_OSRM_NODE_ID;
    reverse = SPECIAL_OSRM_NODE_ID;

    for (auto it = m_osrm2FtSeg.begin(); it != m_osrm2FtSeg.end(); ++it)
    {
      /// @todo Do break in production here when both are found.

      for (auto const & s : it->second)
      {
        if (s.m_fid != seg.m_fid)
          continue;

        if (s.m_pointStart <= s.m_pointEnd)
        {
          if (seg.m_pointStart >= s.m_pointStart && seg.m_pointEnd <= s.m_pointEnd)
          {
            ASSERT_EQUAL(forward, SPECIAL_OSRM_NODE_ID, ());
            forward = it->first;
          }
        }
        else
        {
          if (seg.m_pointStart >= s.m_pointEnd && seg.m_pointEnd <= s.m_pointStart)
          {
            ASSERT_EQUAL(reverse, SPECIAL_OSRM_NODE_ID, ());
            reverse = it->first;
          }
        }
      }
    }
  }

private:
  unordered_map<OsrmNodeIdT, FtSegVectorT> m_osrm2FtSeg;
  FtSegVectorT m_empty;
};

}
