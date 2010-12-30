#pragma once
#include "interval_index.hpp"
#include "../coding/endianness.hpp"
#include "../coding/write_to_sink.hpp"
#include "../base/assert.hpp"
#include "../base/base.hpp"
#include "../base/logging.hpp"
#include "../std/vector.hpp"
#include "../std/memcpy.hpp"

// TODO: BuildIntervalIndex() shouldn't rely on indexing cellid-feature pairs.
template <int kCellIdBytes, typename CellIdValueIterT, class SinkT>
void BuildIntervalIndex(CellIdValueIterT const & beg, CellIdValueIterT const & end,
                        SinkT & writer, int const leafBytes = 1)
{
#ifdef DEBUG
  // Check that [beg, end) is sorted and log most populous cell.
  if (beg != end)
  {
    uint32_t count = 0;
    uint32_t maxCount = 0;
    typename CellIdValueIterT::value_type mostPopulousCell;
    CellIdValueIterT it = beg;
    int64_t prev = it->first;
    for (++it; it != end; ++it)
    {
      ASSERT_GREATER(it->first, 0, ());
      ASSERT_LESS_OR_EQUAL(prev, it->first, ());
      count = (prev == it->first ? count + 1 : 0);
      if (count > maxCount)
      {
        maxCount = count;
        mostPopulousCell = *it;
      }
      prev = it->first;
    }
    if (maxCount > 0)
    {
      LOG(LINFO, ("Most populous cell:", maxCount,
                  mostPopulousCell.first, mostPopulousCell.second));
    }
  }
  for (CellIdValueIterT it = beg; it != end; ++it)
    ASSERT_LESS(it->first, 1ULL << 8 * kCellIdBytes, ());
#endif
  // Write header.
  {
    IntervalIndexBase::Header header;
    header.m_CellIdLeafBytes = leafBytes;
    writer.Write(&header, sizeof(header));
  }

#ifdef DEBUG
  vector<uint32_t> childOffsets;
  childOffsets.push_back(static_cast<uint32_t>(writer.Pos()));
#endif
  uint32_t childOffset = static_cast<uint32_t>(writer.Pos()) + sizeof(IntervalIndexBase::Index);
  uint32_t thisLevelCount = 1;
  for (int level = kCellIdBytes - 1; level >= leafBytes; --level)
  {
    LOG(LINFO, ("Building interval index, level", level));
#ifdef DEBUG
    ASSERT_EQUAL(childOffsets.back(), writer.Pos(), ());
    childOffsets.push_back(childOffset);
#endif
    uint64_t const initialWriterPos = writer.Pos();
    uint32_t childCount = 0, totalChildCount = 0;
    IntervalIndexBase::Index index;
    memset(&index, 0, sizeof(index));
    uint64_t prevParentBytes = 0;
    uint8_t prevByte = 0;
    for (CellIdValueIterT it = beg; it != end; ++it)
    {
      uint64_t id = it->GetCell();
      uint64_t const thisParentBytes = id >> 8 * (level + 1);
      uint8_t const thisByte = static_cast<uint8_t>(0xFF & (id >> 8 * level));
      if (it != beg && prevParentBytes != thisParentBytes)
      {
        // Writing index for previous parent.
        index.SetBaseOffset(childOffset);
        for (uint32_t i = 0; i < 256; ++i)
          index.m_Count[i] = SwapIfBigEndian(index.m_Count[i]);
        writer.Write(&index, sizeof(index));
        memset(&index, 0, sizeof(index));
        if (level != leafBytes)
          childOffset += childCount * sizeof(index);
        else
          childOffset += (sizeof(beg->GetFeature()) + leafBytes) * childCount;
        childCount = 0;
        --thisLevelCount;
      }

      if (level == leafBytes || it == beg ||
          prevByte != thisByte || prevParentBytes != thisParentBytes)
      {
        ++childCount;
        ++totalChildCount;
        ++index.m_Count[thisByte];
        CHECK_LESS(index.m_Count[thisByte], 65535, (level, leafBytes, prevByte, thisByte, \
                                                    prevParentBytes, thisParentBytes, \
                                                    it->GetCell()));
      }

      prevParentBytes = thisParentBytes;
      prevByte = thisByte;
    }
    index.SetBaseOffset(childOffset);
    for (uint32_t i = 0; i < 256; ++i)
      index.m_Count[i] = SwapIfBigEndian(index.m_Count[i]);
    writer.Write(&index, sizeof(index));
    memset(&index, 0, sizeof(index));
    // if level == leafBytes, this is wrong, but childOffset is not needed any more.
    childOffset += childCount * sizeof(IntervalIndexBase::Index);
    --thisLevelCount;
    CHECK_EQUAL(thisLevelCount, 0, (kCellIdBytes, leafBytes));
    thisLevelCount = totalChildCount;

    LOG(LINFO, ("Level size:", writer.Pos() - initialWriterPos));

    if (beg == end)
      break;
  }

  // Writing values.
  ASSERT_EQUAL(childOffsets.back(), writer.Pos(), ());
  LOG(LINFO, ("Building interval, leaves."));
  uint64_t const initialWriterPos = writer.Pos();
  uint32_t const mask = (1ULL << 8 * leafBytes) - 1;
  for (CellIdValueIterT it = beg; it != end; ++it)
  {
    WriteToSink(writer, it->GetFeature());
    uint32_t cellId = static_cast<uint32_t>(it->GetCell() & mask);
    cellId = SwapIfBigEndian(cellId);
    writer.Write(&cellId, leafBytes);
  }
  LOG(LINFO, ("Level size:", writer.Pos() - initialWriterPos));
  LOG(LINFO, ("Interval index building done."));
}
