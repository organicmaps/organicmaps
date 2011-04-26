#pragma once
#include "interval_index.hpp"
#include "../coding/byte_stream.hpp"
#include "../coding/endianness.hpp"
#include "../coding/varint.hpp"
#include "../coding/write_to_sink.hpp"
#include "../base/assert.hpp"
#include "../base/base.hpp"
#include "../base/bits.hpp"
#include "../base/logging.hpp"
#include "../std/vector.hpp"
#include "../std/memcpy.hpp"

namespace impl
{

template <class WriterT>
void WriteIntervalIndexNode(WriterT & writer, uint64_t offset, uint64_t bitMask)
{
  // At the moment, uint32_t is used as a bitMask, but this can change in the future.
  CHECK_EQUAL(static_cast<uint32_t>(bitMask), bitMask, (offset));
  CHECK_GREATER_OR_EQUAL(offset, writer.Pos() + 8, ());
  WriteToSink(writer, static_cast<uint32_t>(offset - writer.Pos() - 8));
  WriteToSink(writer, static_cast<uint32_t>(bitMask));
}

template <class SinkT> void WriteIntervalIndexLeaf(SinkT & sink, int bitsPerLevel,
                                                   uint64_t prevKey, uint64_t prevValue,
                                                   uint64_t key, uint64_t value)
{
  uint64_t const lastBitsZeroMask = (1ULL << bitsPerLevel) - 1;
  if ((key & ~lastBitsZeroMask) != (prevKey & ~lastBitsZeroMask))
    prevValue = 0;

  int64_t const delta = static_cast<int64_t>(value) - static_cast<int64_t>(prevValue);
  uint64_t const encodedDelta = bits::ZigZagEncode(delta);
  uint64_t const code = (encodedDelta << 1) + (key == prevKey ? 0 : 1);
  WriteVarUint(sink, code);
}

inline uint32_t IntervalIndexLeafSize(int bitsPerLevel,
                                      uint64_t prevKey, uint64_t prevValue,
                                      uint64_t key, uint64_t value)
{
  CountingSink sink;
  WriteIntervalIndexLeaf(sink, bitsPerLevel, prevKey, prevValue, key, value);
  return sink.GetCount();
}

template <typename CellIdValueIterT>
bool CheckIntervalIndexInputSequence(CellIdValueIterT const & beg,
                                     CellIdValueIterT const & end,
                                     uint32_t keyBits)
{
  // Check that [beg, end) is sorted and log most populous cell.
  if (beg != end)
  {
    uint32_t count = 0;
    uint32_t maxCount = 0;
    typename CellIdValueIterT::value_type mostPopulousCell = *beg;
    CellIdValueIterT it = beg;
    uint64_t prev = it->GetCell();
    for (++it; it != end; ++it)
    {
      CHECK_GREATER(it->GetCell(), 0, ());
      CHECK_LESS_OR_EQUAL(prev, it->GetCell(), ());
      count = (prev == it->GetCell() ? count + 1 : 0);
      if (count > maxCount)
      {
        maxCount = count;
        mostPopulousCell = *it;
      }
      prev = it->GetCell();
    }
    if (maxCount > 0)
    {
      LOG(LINFO, ("Most populous cell:", maxCount,
                  mostPopulousCell.GetCell(), mostPopulousCell.GetFeature()));
    }
  }
  for (CellIdValueIterT it = beg; it != end; ++it)
    CHECK_LESS(it->GetCell(), 1ULL << keyBits, ());
  return true;
}

}

// TODO: BuildIntervalIndex() shouldn't rely on indexing cellid-feature pairs.
template <typename CellIdValueIterT, class SinkT>
void BuildIntervalIndex(CellIdValueIterT const & beg, CellIdValueIterT const & end,
                        SinkT & writer, uint8_t const keyBits)
{
  CHECK_LESS(keyBits, 63, ());
  CHECK(impl::CheckIntervalIndexInputSequence(beg, end, keyBits), ());

  uint32_t const bitsPerLevel = 5;
  uint32_t const lastBitsMask = (1 << bitsPerLevel) - 1;
  int const levelCount = (keyBits + bitsPerLevel - 1) / bitsPerLevel;

  // Write header.
  {
    IntervalIndexBase::Header header;
    header.m_BitsPerLevel = bitsPerLevel;
    header.m_Levels = levelCount;
    writer.Write(&header, sizeof(header));
  }

  if (beg == end)
  {
    // Write empty index.
    CHECK_GREATER(levelCount, 1, ());
    impl::WriteIntervalIndexNode(writer, writer.Pos() + sizeof(IntervalIndexBase::Index), 0);
    LOG(LWARNING, ("Written empty index."));
    return;
  }

  // Write internal nodes.
  uint64_t childOffset = writer.Pos() + sizeof(IntervalIndexBase::Index);
  uint64_t nextChildOffset = childOffset;
  for (int level = levelCount - 1; level >= 0; --level)
  {
    // LOG(LINFO, ("Building interval index, level", level));
    uint64_t const initialLevelWriterPos = writer.Pos();

    uint64_t bitMask = 0;
    uint64_t prevKey = 0;
    uint64_t prevValue = 0;
    for (CellIdValueIterT it = beg; it != end; ++it)
    {
      uint64_t const key = it->GetCell() >> (level * bitsPerLevel);
      uint32_t const value = it->GetFeature();

      if (it != beg && (prevKey & ~lastBitsMask) != (key & ~lastBitsMask))
      {
        // Write node for the previous parent.
        impl::WriteIntervalIndexNode(writer, childOffset, bitMask);
        childOffset = nextChildOffset;
        bitMask = 0;
      }

      bitMask |= (1ULL << (key & lastBitsMask));

      if (level == 0)
        nextChildOffset += impl::IntervalIndexLeafSize(bitsPerLevel,
                                                       prevKey, prevValue, key, value);
      else if (it == beg || prevKey != key)
        nextChildOffset += sizeof(IntervalIndexBase::Index);

      prevKey = key;
      prevValue = value;
    }

    // Write the last node.
    impl::WriteIntervalIndexNode(writer, childOffset, bitMask);

    if (level == 1)
      nextChildOffset += sizeof(IntervalIndexBase::Index);

    childOffset = nextChildOffset;

    LOG(LINFO, ("Level:", level, "size:", writer.Pos() - initialLevelWriterPos));
  }

  // Write the dummy one-after-last node.
  impl::WriteIntervalIndexNode(writer, nextChildOffset, 0);

  // Write leaves.
  {
    uint64_t const initialLevelWriterPos = writer.Pos();

    uint64_t prevKey = -1;
    uint32_t prevValue = 0;
    for (CellIdValueIterT it = beg; it != end; ++it)
    {
      uint64_t const key = it->GetCell();
      uint32_t const value = it->GetFeature();
      impl::WriteIntervalIndexLeaf(writer, bitsPerLevel, prevKey, prevValue, key, value);
      prevKey = key;
      prevValue = value;
    }

    LOG(LINFO, ("Leaves size:", writer.Pos() - initialLevelWriterPos));
  }

  LOG(LINFO, ("Interval index building done."));
}
