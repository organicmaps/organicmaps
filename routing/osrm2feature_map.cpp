#include "routing/osrm2feature_map.hpp"

#include "defines.hpp"

#include "indexer/data_header.hpp"

#include "platform/local_country_file_utils.hpp"

#include "coding/file_name_utils.hpp"
#include "coding/internal/file_data.hpp"
#include "coding/read_write_utils.hpp"
#include "coding/varint.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"
#include "base/math.hpp"
#include "base/scope_guard.hpp"

#include "std/fstream.hpp"
#include "std/sstream.hpp"
#include "std/unordered_map.hpp"

#include "3party/succinct/mapper.hpp"

using platform::CountryIndexes;

namespace
{
const routing::TNodesList kEmptyList;
}  // namespace

namespace routing
{

namespace OsrmMappingTypes
{
FtSeg::FtSeg(uint32_t fid, uint32_t ps, uint32_t pe)
  : m_fid(fid),
    m_pointStart(static_cast<uint16_t>(ps)),
    m_pointEnd(static_cast<uint16_t>(pe))
{
  CHECK_NOT_EQUAL(ps, pe, ());
  CHECK_EQUAL(m_pointStart, ps, ());
  CHECK_EQUAL(m_pointEnd, pe, ());
}

FtSeg::FtSeg(uint64_t x)
  : m_fid(x & 0xFFFFFFFF), m_pointStart(x >> 48), m_pointEnd((x >> 32) & 0xFFFF)
{
}

uint64_t FtSeg::Store() const
{
  return (uint64_t(m_pointStart) << 48) + (uint64_t(m_pointEnd) << 32) + uint64_t(m_fid);
}

bool FtSeg::Merge(FtSeg const & other)
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

bool FtSeg::IsIntersect(FtSeg const & other) const
{
  if (other.m_fid != m_fid)
    return false;

  auto const s1 = min(m_pointStart, m_pointEnd);
  auto const e1 = max(m_pointStart, m_pointEnd);
  auto const s2 = min(other.m_pointStart, other.m_pointEnd);
  auto const e2 = max(other.m_pointStart, other.m_pointEnd);

  return my::IsIntersect(s1, e1, s2, e2);
}

string DebugPrint(FtSeg const & seg)
{
  stringstream ss;
  ss << "{ fID = " << seg.m_fid <<
        ", pStart = " << seg.m_pointStart <<
        ", pEnd = " << seg.m_pointEnd << " }";
  return ss.str();
}

string DebugPrint(SegOffset const & off)
{
  stringstream ss;
  ss << "{ " << off.m_nodeId << ", " << off.m_offset << " }";
  return ss.str();
}

bool IsInside(FtSeg const & bigSeg, FtSeg const & smallSeg)
{
  ASSERT_EQUAL(bigSeg.m_fid, smallSeg.m_fid, ());
  ASSERT_EQUAL(smallSeg.m_pointEnd - smallSeg.m_pointStart, 1, ());

  auto segmentLeft = min(bigSeg.m_pointStart, bigSeg.m_pointEnd);
  auto segmentRight = max(bigSeg.m_pointStart, bigSeg.m_pointEnd);

  return (segmentLeft <= smallSeg.m_pointStart && segmentRight >= smallSeg.m_pointEnd);
}

FtSeg SplitSegment(FtSeg const & segment, FtSeg const & splitter)
{
  FtSeg resultSeg;
  resultSeg.m_fid = segment.m_fid;

  if (segment.m_pointStart < segment.m_pointEnd)
  {
    resultSeg.m_pointStart = segment.m_pointStart;
    resultSeg.m_pointEnd = splitter.m_pointEnd;
  }
  else
  {
    resultSeg.m_pointStart = segment.m_pointStart;
    resultSeg.m_pointEnd = splitter.m_pointStart;
  }
  return resultSeg;
}
}  // namespace OsrmMappingTypes

void OsrmFtSegMapping::Clear()
{
  m_offsets.clear();
  m_handle.Unmap();
}

void OsrmFtSegMapping::Load(FilesMappingContainer & cont, platform::LocalCountryFile const & localFile)
{
  Clear();

  {
    ReaderSource<FileReader> src(cont.GetReader(ROUTING_NODEIND_TO_FTSEGIND_FILE_TAG));
    uint32_t const count = ReadVarUint<uint32_t>(src);
    m_offsets.resize(count);
    for (uint32_t i = 0; i < count; ++i)
    {
      m_offsets[i].m_nodeId = ReadVarUint<TOsrmNodeId>(src);
      m_offsets[i].m_offset = ReadVarUint<uint32_t>(src);
    }
  }

  m_backwardIndex.Construct(*this, m_offsets.back().m_nodeId, cont, localFile);
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
    OsrmMappingTypes::FtSeg s(m_segments[i]);
    if (s.m_fid == fID)
      LOG(LDEBUG, (s));
  }
#endif
}

void OsrmFtSegMapping::DumpSegmentByNode(TOsrmNodeId nodeId) const
{
#ifdef DEBUG
  ForEachFtSeg(nodeId, [] (OsrmMappingTypes::FtSeg const & s)
  {
    LOG(LDEBUG, (s));
  });
#endif
}

void OsrmFtSegMapping::GetOsrmNodes(TFtSegVec const & segments, OsrmNodesT & res) const
{
  auto addResFn = [&] (uint64_t seg, TOsrmNodeId nodeId, bool forward)
  {
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

  for (auto it = segments.begin(); it != segments.end(); ++it)
  {
    OsrmMappingTypes::FtSeg const & seg = *it;

    TNodesList const & nodeIds = m_backwardIndex.GetNodeIdByFid(seg.m_fid);

    for (uint32_t nodeId : nodeIds)
    {
      auto const & range = GetSegmentsRange(nodeId);
      for (size_t i = range.first; i != range.second; ++i)
      {
        OsrmMappingTypes::FtSeg const s(m_segments[i]);
        if (s.m_fid != seg.m_fid)
          continue;

        if (s.m_pointStart <= s.m_pointEnd)
        {
          if (seg.m_pointStart >= s.m_pointStart && seg.m_pointEnd <= s.m_pointEnd)
          {
            if (addResFn(seg.Store(), nodeId, true))
            {
              break;
            }
          }
        }
        else
        {
          if (seg.m_pointStart >= s.m_pointEnd && seg.m_pointEnd <= s.m_pointStart)
          {
            if (addResFn(seg.Store(), nodeId, false))
            {
              break;
            }
          }
        }
      }
    }
  }
}

void OsrmFtSegMapping::GetSegmentByIndex(size_t idx, OsrmMappingTypes::FtSeg & seg) const
{
  ASSERT_LESS(idx, m_segments.size(), ());
  OsrmMappingTypes::FtSeg(m_segments[idx]).Swap(seg);
}

pair<size_t, size_t> OsrmFtSegMapping::GetSegmentsRange(TOsrmNodeId nodeId) const
{
  SegOffsetsT::const_iterator it = lower_bound(m_offsets.begin(), m_offsets.end(), OsrmMappingTypes::SegOffset(nodeId, 0),
                                               [] (OsrmMappingTypes::SegOffset const & o, OsrmMappingTypes::SegOffset const & val)
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

TOsrmNodeId OsrmFtSegMapping::GetNodeId(uint32_t segInd) const
{
  SegOffsetsT::const_iterator it = lower_bound(m_offsets.begin(), m_offsets.end(), OsrmMappingTypes::SegOffset(segInd, 0),
                                               [] (OsrmMappingTypes::SegOffset const & o, OsrmMappingTypes::SegOffset const & val)
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

void OsrmFtSegMappingBuilder::Append(TOsrmNodeId nodeId, FtSegVectorT const & data)
{
  size_t const count = data.size();

  if (count == 0)
  {
    m_buffer.emplace_back(OsrmMappingTypes::FtSeg(kInvalidFid, 0, 1).Store());
  }
  else
  {
    for (size_t i = 0; i < count; ++i)
      m_buffer.emplace_back(data[i].Store());
  }

  if (count > 1)
  {
    m_lastOffset += (data.size() - 1);

    uint32_t const off = static_cast<uint32_t>(m_lastOffset);
    CHECK_EQUAL(m_lastOffset, off, ());

    m_offsets.emplace_back(OsrmMappingTypes::SegOffset(nodeId, off));
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
    writer.WritePaddingByEnd(4);
  }

  string const fName = cont.GetFileName() + "." ROUTING_FTSEG_FILE_TAG;
  MY_SCOPE_GUARD(deleteFileGuard, bind(&FileWriter::DeleteFileX, cref(fName)));

  succinct::elias_fano_compressed_list compressed(m_buffer);
  succinct::mapper::freeze(compressed, fName.c_str());
  cont.Write(fName, ROUTING_FTSEG_FILE_TAG);
}

void OsrmFtSegBackwardIndex::Save(string const & nodesFileName, string const & bitsFileName)
{
  {
    LOG(LINFO, ("Saving routing nodes index to ", nodesFileName));
    string const nodesFileNameTmp = nodesFileName + EXTENSION_TMP;
    FileWriter nodesFile(nodesFileNameTmp);
    WriteVarUint(nodesFile, static_cast<uint32_t>(m_nodeIds.size()));
    for (auto const bucket : m_nodeIds)
    {
      rw::WriteVectorOfPOD(nodesFile, bucket);
    }
    my::RenameFileX(nodesFileNameTmp, nodesFileName);
  }
  {
    LOG(LINFO, ("Saving features routing bits to ", bitsFileName));
    string const bitsFileNameTmp = bitsFileName + EXTENSION_TMP;
    succinct::mapper::freeze(m_rankIndex, bitsFileNameTmp.c_str());
    my::RenameFileX(bitsFileNameTmp, bitsFileName);
  }
}

bool OsrmFtSegBackwardIndex::Load(string const & nodesFileName, string const & bitsFileName)
{
  Platform & pl = GetPlatform();
  if (!pl.IsFileExistsByFullPath(nodesFileName) || !pl.IsFileExistsByFullPath(bitsFileName))
    return false;

  m_mappedBits.reset(new MmapReader(bitsFileName));

  {
    FileReader nodesFile(nodesFileName);
    ReaderSource<FileReader> nodesSource(nodesFile);
    uint32_t size = ReadVarUint<uint32_t>(nodesSource);
    m_nodeIds.resize(size);
    for (uint32_t i = 0; i < size; ++i)
    {
      rw::ReadVectorOfPOD(nodesSource, m_nodeIds[i]);
    }
  }
  succinct::mapper::map(m_rankIndex, reinterpret_cast<char const *>(m_mappedBits->Data()));

  return true;
}

void OsrmFtSegBackwardIndex::Construct(OsrmFtSegMapping & mapping, uint32_t maxNodeId,
                                       FilesMappingContainer & routingFile,
                                       platform::LocalCountryFile const & localFile)
{
  Clear();

  feature::DataHeader header(localFile.GetPath(MapOptions::Map));
  m_oldFormat = header.GetFormat() < version::v5;
  if (m_oldFormat)
    LOG(LINFO, ("Using old format index for", localFile.GetCountryName()));

  CountryIndexes::PreparePlaceOnDisk(localFile);
  string const bitsFileName = CountryIndexes::GetPath(localFile, CountryIndexes::Index::Bits);
  string const nodesFileName = CountryIndexes::GetPath(localFile, CountryIndexes::Index::Nodes);

  m_table = feature::FeaturesOffsetsTable::CreateIfNotExistsAndLoad(localFile);

  if (Load(bitsFileName, nodesFileName))
    return;

  LOG(LINFO, ("Backward routing index is absent! Creating new one."));
  mapping.Map(routingFile);

  // Generate temporary index to speedup processing
  unordered_map<uint32_t, TNodesList> temporaryBackwardIndex;
  for (uint32_t i = 0; i < maxNodeId; ++i)
  {
    auto indexes = mapping.GetSegmentsRange(i);
    for (; indexes.first != indexes.second; ++indexes.first)
    {
      OsrmMappingTypes::FtSeg seg;
      mapping.GetSegmentByIndex(indexes.first, seg);
      temporaryBackwardIndex[seg.m_fid].push_back(i);
    }
  }

  mapping.Unmap();
  LOG(LINFO, ("Temporary index constructed"));

  // Create final index
  vector<bool> inIndex(m_table->size(), false);
  m_nodeIds.reserve(temporaryBackwardIndex.size());

  size_t removedNodes = 0;
  for (size_t i = 0; i < m_table->size(); ++i)
  {
    uint32_t const fid = m_oldFormat ? m_table->GetFeatureOffset(i) : static_cast<uint32_t>(i);
    auto it = temporaryBackwardIndex.find(fid);
    if (it != temporaryBackwardIndex.end())
    {
      inIndex[i] = true;
      m_nodeIds.emplace_back(move(it->second));

      // Remove duplicates nodes emmited by equal choises on a generation route step.
      TNodesList & nodesList = m_nodeIds.back();
      size_t const foundNodes = nodesList.size();
      sort(nodesList.begin(), nodesList.end());
      nodesList.erase(unique(nodesList.begin(), nodesList.end()), nodesList.end());
      removedNodes += foundNodes - nodesList.size();
    }
  }

  LOG(LDEBUG, ("Backward index constructor removed", removedNodes, "duplicates."));

  // Pack and save index
  succinct::rs_bit_vector(inIndex).swap(m_rankIndex);

  LOG(LINFO, ("Writing additional indexes to data files", bitsFileName, nodesFileName));
  Save(bitsFileName, nodesFileName);
}

TNodesList const & OsrmFtSegBackwardIndex::GetNodeIdByFid(uint32_t fid) const
{
  ASSERT(m_table, ());

  size_t const index = m_oldFormat ? m_table->GetFeatureIndexbyOffset(fid) : fid;
  ASSERT_LESS(index, m_table->size(), ("Can't find feature index in offsets table"));
  if (!m_rankIndex[index])
    return kEmptyList;

  size_t nodeIdx = m_rankIndex.rank(index);
  ASSERT_LESS(nodeIdx, m_nodeIds.size(), ());
  return m_nodeIds[nodeIdx];
}

void OsrmFtSegBackwardIndex::Clear()
{
  m_nodeIds.clear();
  succinct::rs_bit_vector().swap(m_rankIndex);
  m_table.reset();
  m_mappedBits.reset();
}

}
