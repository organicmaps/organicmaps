#pragma once

#include "../coding/file_container.hpp"

#include "../std/string.hpp"
#include "../std/vector.hpp"
#include "../std/unordered_map.hpp"
#include "../std/utility.hpp"

#include "../3party/succinct/elias_fano_compressed_list.hpp"

#include "../defines.hpp"
namespace routing
{

typedef uint32_t OsrmNodeIdT;
extern OsrmNodeIdT const INVALID_NODE_ID;

namespace OsrmMappingTypes {
#pragma pack (push, 1)
  struct FtSeg
  {
    uint32_t m_fid;
    uint16_t m_pointStart;
    uint16_t m_pointEnd;

    static const uint32_t INVALID_FID = -1;

    // No need to initialize something her (for vector<FtSeg>).
    FtSeg() {}
    FtSeg(uint32_t fid, uint32_t ps, uint32_t pe);

    explicit FtSeg(uint64_t x);
    uint64_t Store() const;

    bool Merge(FtSeg const & other);

    bool operator == (FtSeg const & other) const
    {
      return (other.m_fid == m_fid)
          && (other.m_pointEnd == m_pointEnd)
          && (other.m_pointStart == m_pointStart);
    }

    bool IsIntersect(FtSeg const & other) const;

    bool IsValid() const
    {
      return m_fid != INVALID_FID;
    }

    void Swap(FtSeg & other)
    {
      swap(m_fid, other.m_fid);
      swap(m_pointStart, other.m_pointStart);
      swap(m_pointEnd, other.m_pointEnd);
    }

    friend string DebugPrint(FtSeg const & seg);
  };

  struct SegOffset
  {
    OsrmNodeIdT m_nodeId;
    uint32_t m_offset;

    SegOffset() : m_nodeId(0), m_offset(0) {}

    SegOffset(uint32_t nodeId, uint32_t offset)
      : m_nodeId(nodeId), m_offset(offset)
    {
    }

    friend string DebugPrint(SegOffset const & off);
  };

  struct FtSegLess
  {
    bool operator() (FtSeg const * a, FtSeg const * b) const
    {
      return a->Store() < b->Store();
    }
  };
#pragma pack (pop)
}

class OsrmFtSegBackwardIndex
{
  typedef pair<uint32_t, uint32_t> IndexRecordTypeT;
  vector<IndexRecordTypeT> m_index;

  void Save(FilesMappingContainer const & parentCont)
  {
    const indexName = parentCont.GetName()+FTSEG_MAPPING_BACKWARD_INDEX;
    Platform & pl = GetPlatform();
    string const fPath = pl.WritablePathForFile(indexName);
    if (!pl.IsFileExistsByFullPath(fPath))
    {
      return;
    }

    FilesContainerW container(fName);
    FileWriter w = container.GetWriter(VERSION_FILE_TAG);
    WriterSrc wrt(w);

    FileReader r2 = parentCont.GetReader(VERSION_FILE_TAG);
    ReaderSrc src2(r2);

    ver::WriteTimestamp(wrt, ver::ReadTimestamp(src2));

    size_t count = static_cast<uint32_t>(m_index.size());
    FileWriter w2 = container.GetWriter(FTSEG_MAPPING_BACKWARD_INDEX);
    WriterSrc wrt2(w2);
    WriteVarUint<uint32_t>(wrt2, count);
    for(size_t i = 0; i < count; ++i)
    {
      WriteVarUint<uint32_t>(src, m_index[i].first);
      WriteVarUint<uint32_t>(src, m_index[i].second);
    }
  }

  bool Load(FilesMappingContainer const & parentCont)
  {
    const indexName = parentCont.GetName()+FTSEG_MAPPING_BACKWARD_INDEX;
    Platform & pl = GetPlatform();
    string const fPath = pl.WritablePathForFile(indexName);
    if (!pl.IsFileExistsByFullPath(fPath))
    {
      return false;
    }
    // Open new container and check that file has equal timestamp.
    FilesMappingContainer container;
    container.Open(fPath);
    {
      FileReader r1 = container.GetReader(VERSION_FILE_TAG);
      ReaderSrc src1(r1);
      ModelReaderPtr r2 = parentCont.GetReader(VERSION_FILE_TAG);
      ReaderSrc src2(r2.GetPtr());

      if (ver::ReadTimestamp(src1) != ver::ReadTimestamp(src2))
        return false;
    }

    FileReader r = container.GetReader(FTSEG_MAPPING_BACKWARD_INDEX);
    ReaderSource<FileReader> src(r);
    uint32_t const count = ReadVarUint<uint32_t>(src);
    for(size_t i = 0; i < count; ++i)
    {
      uint32_t fid = ReadVarUint<uint32_t>(src);
      uint32_t index = ReadVarUint<uint32_t>(src);
      m_index[i] = make_pair(fid, index);
    }
    return true;
  }

public:
  void Construct(succinct::elias_fano_compressed_list const & segments, FilesMappingContainer const & parentCont)
  {
    if (Load(parentCont))
      return;
    size_t const count = segments.size();
    m_index.resize(count);
    for (size_t i = 0; i < count; ++i)
    {
      OsrmMappingTypes::FtSeg s(segments[i]);
      m_index[i] = make_pair(s.m_fid, i);
    }
    sort(m_index.begin(), m_index.end(), [](IndexRecordTypeT const & a, IndexRecordTypeT const & b)
    {
     return a.first < b.first;
    });
    Save(parentCont);
  }

  void GetIndexesByFid(uint32_t const fid, vector<size_t> & results) const
  {
    size_t const start = distance(m_index.begin(), lower_bound(m_index.begin(), m_index.end(), make_pair(fid, 0),
                                                               [](IndexRecordTypeT const & a, IndexRecordTypeT const & b)
                                                               {
                                                                return a.first < b.first;
                                                               }));
    size_t stop = start;
    while (stop<m_index.size() && m_index[stop].first == fid)
    {
      results.push_back(m_index[stop].second);
      ++stop;
    }
    return;
  }

};

class OsrmFtSegMapping
{
public:
  typedef set<OsrmMappingTypes::FtSeg*, OsrmMappingTypes::FtSegLess> FtSegSetT;

  void Clear();
  void Load(FilesMappingContainer & cont);

  void Map(FilesMappingContainer & cont);
  void Unmap();
  bool IsMapped() const;

  template <class ToDo> void ForEachFtSeg(OsrmNodeIdT nodeId, ToDo toDo) const
  {
    pair<size_t, size_t> r = GetSegmentsRange(nodeId);
    while (r.first != r.second)
    {
      OsrmMappingTypes::FtSeg s(m_segments[r.first]);
      if (s.IsValid())
        toDo(s);
      ++r.first;
    }
  }

  typedef unordered_map<uint64_t, pair<OsrmNodeIdT, OsrmNodeIdT> > OsrmNodesT;
  void GetOsrmNodes(FtSegSetT & segments, OsrmNodesT & res, volatile bool const & requestCancel) const;

  void GetSegmentByIndex(size_t idx, OsrmMappingTypes::FtSeg & seg) const;

  /// @name For debug purpose only.
  //@{
  void DumpSegmentsByFID(uint32_t fID) const;
  void DumpSegmentByNode(OsrmNodeIdT nodeId) const;
  //@}

  /// @name For unit test purpose only.
  //@{
  /// @return STL-like range [s, e) of segments indexies for passed node.
  pair<size_t, size_t> GetSegmentsRange(uint32_t nodeId) const;
  /// @return Node id for segment's index.
  OsrmNodeIdT GetNodeId(size_t segInd) const;

  size_t GetSegmentsCount() const { return m_segments.size(); }
  //@}

protected:
  typedef vector<OsrmMappingTypes::SegOffset> SegOffsetsT;
  SegOffsetsT m_offsets;

private:
  succinct::elias_fano_compressed_list m_segments;
  FilesMappingContainer::Handle m_handle;
  OsrmFtSegBackwardIndex m_backwardIndex;
};

class OsrmFtSegMappingBuilder : public OsrmFtSegMapping
{
public:
  OsrmFtSegMappingBuilder();

  typedef vector<OsrmMappingTypes::FtSeg> FtSegVectorT;

  void Append(OsrmNodeIdT nodeId, FtSegVectorT const & data);
  void Save(FilesContainerW & cont) const;

private:
  vector<uint64_t> m_buffer;
  uint64_t m_lastOffset;
};

}
