#include "osrm2feature_map.hpp"

#include "../defines.hpp"

#include "../coding/varint.hpp"

#include "../base/assert.hpp"
#include "../base/logging.hpp"
#include "../base/math.hpp"
#include "../base/scope_guard.hpp"

#include "../std/fstream.hpp"
#include "../std/sstream.hpp"

#include "../3party/succinct/mapper.hpp"


namespace routing
{

OsrmNodeIdT const INVALID_NODE_ID = -1;

OsrmFtSegMapping::FtSeg::FtSeg(uint32_t fid, uint32_t ps, uint32_t pe)
  : m_fid(fid),
    m_pointStart(static_cast<uint16_t>(ps)),
    m_pointEnd(static_cast<uint16_t>(pe))
{
  CHECK_NOT_EQUAL(ps, pe, ());
  CHECK_EQUAL(m_pointStart, ps, ());
  CHECK_EQUAL(m_pointEnd, pe, ());
}

OsrmFtSegMapping::FtSeg::FtSeg(uint64_t x)
  : m_fid(x & 0xFFFFFFFF), m_pointStart(x >> 48), m_pointEnd((x >> 32) & 0xFFFF)
{
}

uint64_t OsrmFtSegMapping::FtSeg::Store() const
{
  return (uint64_t(m_pointStart) << 48) + (uint64_t(m_pointEnd) << 32) + uint64_t(m_fid);
}

bool OsrmFtSegMapping::FtSeg::Merge(FtSeg const & other)
{
  if (other.m_fid != m_fid)
    return false;

  bool const dir = other.m_pointEnd > other.m_pointStart;
  if (dir != (m_pointEnd > m_pointStart))
    return false;

  auto const s1 = min(m_pointStart, m_pointEnd);
  auto const e1 = max(m_pointStart, m_pointEnd);
  auto const s2 = min(other.m_pointStart, other.m_pointEnd);
  auto const e2 = max(other.m_pointStart, other.m_pointEnd);

  if (my::IsIntersect(s1, e1, s2, e2))
  {
    m_pointStart = min(s1, s2);
    m_pointEnd = max(e1, e2);
    if (!dir)
      swap(m_pointStart, m_pointEnd);

    return true;
  }
  else
    return false;
}

bool OsrmFtSegMapping::FtSeg::IsIntersect(FtSeg const & other) const
{
  if (other.m_fid != m_fid)
    return false;

  auto const s1 = min(m_pointStart, m_pointEnd);
  auto const e1 = max(m_pointStart, m_pointEnd);
  auto const s2 = min(other.m_pointStart, other.m_pointEnd);
  auto const e2 = max(other.m_pointStart, other.m_pointEnd);

  return my::IsIntersect(s1, e1, s2, e2);
}

string DebugPrint(OsrmFtSegMapping::FtSeg const & seg)
{
  stringstream ss;
  ss << "{ fID = " << seg.m_fid <<
        ", pStart = " << seg.m_pointStart <<
        ", pEnd = " << seg.m_pointEnd << " }";
  return ss.str();
}

string DebugPrint(OsrmFtSegMapping::SegOffset const & off)
{
  stringstream ss;
  ss << "{ " << off.m_nodeId << ", " << off.m_offset << " }";
  return ss.str();
}


void OsrmFtSegMapping::Clear()
{
  m_offsets.clear();
  m_handle.Unmap();
}

void OsrmFtSegMapping::Load(FilesMappingContainer & cont)
{
  Clear();

  {
    ReaderSource<FileReader> src(cont.GetReader(ROUTING_NODEIND_TO_FTSEGIND_FILE_TAG));
    uint32_t const count = ReadVarUint<uint32_t>(src);
    m_offsets.resize(count);
    for (uint32_t i = 0; i < count; ++i)
    {
      m_offsets[i].m_nodeId = ReadVarUint<OsrmNodeIdT>(src);
      m_offsets[i].m_offset = ReadVarUint<uint32_t>(src);
    }
  }

  Map(cont);
}

void OsrmFtSegMapping::Map(FilesMappingContainer & cont)
{
  m_handle.Assign(cont.Map(ROUTING_FTSEG_FILE_TAG));
  ASSERT(m_handle.IsValid(), ());
  succinct::mapper::map(m_segments, m_handle.GetData<char>());
}

void OsrmFtSegMapping::Unmap()
{
  m_handle.Unmap();
}

bool OsrmFtSegMapping::IsMapped() const
{
  return m_handle.IsValid();
}

void OsrmFtSegMapping::DumpSegmentsByFID(uint32_t fID) const
{
#ifdef DEBUG
  for (size_t i = 0; i < m_segments.size(); ++i)
  {
    FtSeg s(m_segments[i]);
    if (s.m_fid == fID)
      LOG(LDEBUG, (s));
  }
#endif
}

void OsrmFtSegMapping::DumpSegmentByNode(OsrmNodeIdT nodeId) const
{
#ifdef DEBUG
  ForEachFtSeg(nodeId, [] (FtSeg const & s)
  {
    LOG(LDEBUG, (s));
  });
#endif
}


void OsrmFtSegMapping::GetOsrmNodes(FtSegSetT & segments, OsrmNodesT & res, volatile bool const & requestCancel) const
{
  auto addResFn = [&](uint64_t seg, size_t idx, bool forward)
  {
    OsrmNodeIdT const nodeId = GetNodeId(idx);
    auto it = res.insert({ seg, { forward ? nodeId : INVALID_NODE_ID,
                                  forward ? INVALID_NODE_ID : nodeId } });
    if (it.second)
      return false;
    else
    {
      if (forward)
      {
        ASSERT_EQUAL(it.first->second.first, INVALID_NODE_ID, ());
        it.first->second.first = nodeId;
      }
      else
      {
        ASSERT_EQUAL(it.first->second.second, INVALID_NODE_ID, ());
        it.first->second.second = nodeId;
      }
    }
    return true;
  };

  size_t const count = GetSegmentsCount();
  for (size_t i = 0; i < count && !segments.empty(); ++i)
  {
    FtSeg s(m_segments[i]);

    if (requestCancel)
      return;

    for (auto it = segments.begin(); it != segments.end(); ++it)
    {
      FtSeg const & seg = *(*it);
      if (s.m_fid != seg.m_fid)
        continue;

      if (s.m_pointStart <= s.m_pointEnd)
      {
        if (seg.m_pointStart >= s.m_pointStart && seg.m_pointEnd <= s.m_pointEnd)
        {
          if (addResFn(seg.Store(), i, true))
          {
            segments.erase(it);
            break;
          }
        }
      }
      else
      {
        if (seg.m_pointStart >= s.m_pointEnd && seg.m_pointEnd <= s.m_pointStart)
        {
          if (addResFn(seg.Store(), i, false))
          {
            segments.erase(it);
            break;
          }
        }
      }
    }
  }
}

pair<size_t, size_t> OsrmFtSegMapping::GetSegmentsRange(OsrmNodeIdT nodeId) const
{
  SegOffsetsT::const_iterator it = lower_bound(m_offsets.begin(), m_offsets.end(), SegOffset(nodeId, 0),
                                               [] (SegOffset const & o, SegOffset const & val)
                                               {
                                                  return (o.m_nodeId < val.m_nodeId);
                                               });

  size_t const index = distance(m_offsets.begin(), it);
  size_t const start = (index > 0  ? m_offsets[index - 1].m_offset + nodeId : nodeId);

  if (index < m_offsets.size() && m_offsets[index].m_nodeId == nodeId)
    return make_pair(start, m_offsets[index].m_offset + nodeId + 1);
  else
    return make_pair(start, start + 1);
}

OsrmNodeIdT OsrmFtSegMapping::GetNodeId(size_t segInd) const
{
  SegOffsetsT::const_iterator it = lower_bound(m_offsets.begin(), m_offsets.end(), SegOffset(segInd, 0),
                                               [] (SegOffset const & o, SegOffset const & val)
                                               {
                                                  return (o.m_nodeId + o.m_offset < val.m_nodeId);
                                               });

  size_t const index = distance(m_offsets.begin(), it);
  uint32_t const prevOffset = index > 0 ? m_offsets[index-1].m_offset : 0;

  if ((index < m_offsets.size()) &&
      (segInd >= prevOffset + m_offsets[index].m_nodeId) &&
      (segInd <= m_offsets[index].m_offset + m_offsets[index].m_nodeId))
  {
    return m_offsets[index].m_nodeId;
  }

  return (segInd - prevOffset);
}

OsrmFtSegMappingBuilder::OsrmFtSegMappingBuilder()
  : m_lastOffset(0)
{
}

void OsrmFtSegMappingBuilder::Append(OsrmNodeIdT nodeId, FtSegVectorT const & data)
{
  size_t const count = data.size();

  if (count == 0)
    m_buffer.push_back(FtSeg(FtSeg::INVALID_FID, 0, 1).Store());
  else
  {
    for (size_t i = 0; i < count; ++i)
      m_buffer.push_back(data[i].Store());
  }

  if (count > 1)
  {
    m_lastOffset += (data.size() - 1);

    uint32_t const off = static_cast<uint32_t>(m_lastOffset);
    CHECK_EQUAL(m_lastOffset, off, ());

    m_offsets.push_back(SegOffset(nodeId, off));
  }
}
void OsrmFtSegMappingBuilder::Save(FilesContainerW & cont) const
{
  {
    FileWriter writer = cont.GetWriter(ROUTING_NODEIND_TO_FTSEGIND_FILE_TAG);
    uint32_t const count = static_cast<uint32_t>(m_offsets.size());
    WriteVarUint(writer, count);
    for (uint32_t i = 0; i < count; ++i)
    {
      WriteVarUint(writer, m_offsets[i].m_nodeId);
      WriteVarUint(writer, m_offsets[i].m_offset);
    }

    // Write padding to make next elias_fano start address multiple of 4.
    writer.WritePadding(4);
  }

  string const fName = cont.GetFileName() + "." ROUTING_FTSEG_FILE_TAG;
  MY_SCOPE_GUARD(deleteFileGuard, bind(&FileWriter::DeleteFileX, cref(fName)));

  succinct::elias_fano_compressed_list compressed(m_buffer);
  succinct::mapper::freeze(compressed, fName.c_str());
  cont.Write(fName, ROUTING_FTSEG_FILE_TAG);
}

}
