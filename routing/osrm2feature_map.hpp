#pragma once

#include "../coding/file_container.hpp"

#include "../std/string.hpp"
#include "../std/vector.hpp"
#include "../std/utility.hpp"


namespace routing
{

typedef uint32_t OsrmNodeIdT;

class OsrmFtSegMapping
{
public:

#pragma pack (push, 1)
  struct FtSeg
  {
    uint32_t m_fid;
    uint16_t m_pointStart;
    uint16_t m_pointEnd;

    FtSeg()
      : m_fid(-1), m_pointStart(-1), m_pointEnd(-1)
    {
    }

    FtSeg(uint32_t fid, uint32_t ps, uint32_t pe);

    bool Merge(FtSeg const & other);

    bool operator == (FtSeg const & other) const
    {
      return (other.m_fid == m_fid)
          && (other.m_pointEnd == m_pointEnd)
          && (other.m_pointStart == m_pointStart);
    }

    bool IsIntersect(FtSeg const & other) const;

    friend string DebugPrint(FtSeg const & seg);
  };

  struct SegOffset
  {
    OsrmNodeIdT m_nodeId;
    uint32_t m_offset;

    SegOffset()
      : m_nodeId(0), m_offset(0)
    {
    }

    SegOffset(uint32_t nodeId, uint32_t offset)
      : m_nodeId(nodeId), m_offset(offset)
    {
    }
  };
#pragma pack (pop)

  void Clear();
  void Load(FilesMappingContainer & cont);

  pair<FtSeg const *, size_t> GetSegVector(OsrmNodeIdT nodeId) const;
  void GetOsrmNode(FtSeg const & seg, OsrmNodeIdT & forward, OsrmNodeIdT & reverse) const;

  /// @name For debug purpose only.
  //@{
  void DumpSegmentsByFID(uint32_t fID) const;
  void DumpSegmentByNode(uint32_t nodeId) const;
  //@}

  /// @name For unit test purpose only.
  //@{
  pair<size_t, size_t> GetSegmentsRange(uint32_t nodeId) const;
  OsrmNodeIdT GetNodeId(size_t segInd) const;

  FtSeg const * GetSegments() const { return m_handle.GetData<FtSeg>(); }
  size_t GetSegmentsCount() const;
  //@}

protected:
  typedef vector<SegOffset> SegOffsetsT;
  SegOffsetsT m_offsets;

private:
  FilesMappingContainer::Handle m_handle;

};


class OsrmFtSegMappingBuilder : public OsrmFtSegMapping
{
public:
  OsrmFtSegMappingBuilder();

  typedef vector<FtSeg> FtSegVectorT;

  void Append(OsrmNodeIdT osrmNodeId, FtSegVectorT const & data);
  void Save(FilesContainerW & cont) const;

private:
  FtSegVectorT m_segments;

  uint64_t m_lastOffset;
};

}
