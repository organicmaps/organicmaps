#pragma once

#include "indexer/features_offsets_table.hpp"

#include "coding/file_container.hpp"
#include "coding/mmap_reader.hpp"

#include "platform/platform.hpp"

#include "base/scope_guard.hpp"

#include "std/limits.hpp"
#include "std/string.hpp"
#include "std/unordered_map.hpp"
#include "std/utility.hpp"
#include "std/vector.hpp"

#include "3party/succinct/rs_bit_vector.hpp"
#include "3party/succinct/elias_fano_compressed_list.hpp"

#include "defines.hpp"


namespace routing
{

typedef uint32_t TOsrmNodeId;
typedef vector<TOsrmNodeId> TNodesList;
constexpr TOsrmNodeId INVALID_NODE_ID = numeric_limits<TOsrmNodeId>::max();
constexpr uint32_t kInvalidFid = numeric_limits<uint32_t>::max();

namespace OsrmMappingTypes
{
#pragma pack (push, 1)
  struct FtSeg
  {
    uint32_t m_fid;
    uint16_t m_pointStart;
    uint16_t m_pointEnd;


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
      return m_fid != kInvalidFid;
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
    TOsrmNodeId m_nodeId;
    uint32_t m_offset;

    SegOffset() : m_nodeId(0), m_offset(0) {}

    SegOffset(uint32_t nodeId, uint32_t offset)
      : m_nodeId(nodeId), m_offset(offset)
    {
    }

    friend string DebugPrint(SegOffset const & off);
  };
#pragma pack (pop)

/// Checks if a smallSeg is inside a bigSeg. Note that the smallSeg must be an ordered segment
/// with 1 point length.
bool IsInside(FtSeg const & bigSeg, FtSeg const & smallSeg);

/// Splits segment by splitter segment and takes part of it.
/// Warning! This function returns part from the start of the segment to the splitter, including it.
/// Splitter segment points must be ordered.
FtSeg SplitSegment(FtSeg const & segment, FtSeg const & splitter);
}  // namespace OsrmMappingTypes

class OsrmFtSegMapping;

class OsrmFtSegBackwardIndex
{
  succinct::rs_bit_vector m_rankIndex;
  vector<TNodesList> m_nodeIds;
  unique_ptr<feature::FeaturesOffsetsTable> m_table;

  unique_ptr<MmapReader> m_mappedBits;

  bool m_oldFormat;

  void Save(string const & nodesFileName, string const & bitsFileName);
  bool Load(string const & nodesFileName, string const & bitsFileName);

public:
  void Construct(OsrmFtSegMapping & mapping, uint32_t maxNodeId,
                 FilesMappingContainer & routingFile,
                 platform::LocalCountryFile const & localFile);

  TNodesList const & GetNodeIdByFid(uint32_t fid) const;

  void Clear();
};

class OsrmFtSegMapping
{
public:
  using TFtSegVec = vector<OsrmMappingTypes::FtSeg>;

  void Clear();
  void Load(FilesMappingContainer & cont, platform::LocalCountryFile const & localFile);

  void Map(FilesMappingContainer & cont);
  void Unmap();
  bool IsMapped() const;

  template <class ToDo> void ForEachFtSeg(TOsrmNodeId nodeId, ToDo toDo) const
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

  typedef unordered_map<uint64_t, pair<TOsrmNodeId, TOsrmNodeId> > OsrmNodesT;
  void GetOsrmNodes(TFtSegVec const & segments, OsrmNodesT & res) const;

  void GetSegmentByIndex(size_t idx, OsrmMappingTypes::FtSeg & seg) const;
  TNodesList const & GetNodeIdByFid(uint32_t fid) const
  {
    return m_backwardIndex.GetNodeIdByFid(fid);
  }

  /// @name For debug purpose only.
  //@{
  void DumpSegmentsByFID(uint32_t fID) const;
  void DumpSegmentByNode(TOsrmNodeId nodeId) const;
  //@}

  /// @name For unit test purpose only.
  //@{
  /// @return STL-like range [s, e) of segments indexies for passed node.
  pair<size_t, size_t> GetSegmentsRange(TOsrmNodeId nodeId) const;
  /// @return Node id for segment's index.
  TOsrmNodeId GetNodeId(uint32_t segInd) const;

  size_t GetSegmentsCount() const { return static_cast<size_t>(m_segments.size()); }
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

  void Append(TOsrmNodeId nodeId, FtSegVectorT const & data);
  void Save(FilesContainerW & cont) const;

private:
  vector<uint64_t> m_buffer;
  uint64_t m_lastOffset;
};

}
