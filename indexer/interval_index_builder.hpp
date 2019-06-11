#pragma once

#include "indexer/interval_index.hpp"

#include "coding/byte_stream.hpp"
#include "coding/endianness.hpp"
#include "coding/varint.hpp"
#include "coding/write_to_sink.hpp"

#include "base/assert.hpp"
#include "base/base.hpp"
#include "base/bits.hpp"
#include "base/checked_cast.hpp"
#include "base/logging.hpp"

#include <cstdint>
#include <limits>
#include <vector>

// +------------------------------+
// |            Header            |
// +------------------------------+
// |        Leaves  offset        |
// +------------------------------+
// |        Level 1 offset        |
// +------------------------------+
// |             ...              |
// +------------------------------+
// |        Level N offset        |
// +------------------------------+
// |        Leaves  data          |
// +------------------------------+
// |        Level 1 data          |
// +------------------------------+
// |             ...              |
// +------------------------------+
// |        Level N data          |
// +------------------------------+
class IntervalIndexBuilder
{
public:
  IntervalIndexBuilder(uint32_t keyBits, uint32_t leafBytes, uint32_t bitsPerLevel = 8)
    : IntervalIndexBuilder(IntervalIndexVersion::V1, keyBits, leafBytes, bitsPerLevel)
  { }

  IntervalIndexBuilder(IntervalIndexVersion version, uint32_t keyBits, uint32_t leafBytes,
                       uint32_t bitsPerLevel = 8)
    : m_version{version}, m_BitsPerLevel(bitsPerLevel), m_LeafBytes(leafBytes)
  {
    CHECK_GREATER_OR_EQUAL(
        static_cast<uint8_t>(version), static_cast<uint8_t>(IntervalIndexVersion::V1), ());
    CHECK_LESS_OR_EQUAL(
        static_cast<uint8_t>(version), static_cast<uint8_t>(IntervalIndexVersion::V2), ());
    CHECK_GREATER(leafBytes, 0, ());
    CHECK_LESS(keyBits, 63, ());
    int const nodeKeyBits = keyBits - (m_LeafBytes << 3);
    CHECK_GREATER(nodeKeyBits, 0, (keyBits, leafBytes));
    m_Levels = (nodeKeyBits + m_BitsPerLevel - 1) / m_BitsPerLevel;
    m_LastBitsMask = (1 << m_BitsPerLevel) - 1;
  }

  uint32_t GetLevelCount() const { return m_Levels; }

  template <class Writer, typename CellIdValueIter>
  void BuildIndex(Writer & writer, CellIdValueIter const & beg, CellIdValueIter const & end)
  {
    CHECK(CheckIntervalIndexInputSequence(beg, end), ());

    if (beg == end)
    {
      IntervalIndexBase::Header header;
      header.m_Version = static_cast<uint8_t>(m_version);
      header.m_BitsPerLevel = 0;
      header.m_Levels = 0;
      header.m_LeafBytes = 0;
      writer.Write(&header, sizeof(header));
      return;
    }

    uint64_t const initialPos = writer.Pos();
    WriteZeroesToSink(writer, sizeof(IntervalIndexBase::Header));
    WriteZeroesToSink(writer, (m_version == IntervalIndexVersion::V1 ? 4 : 8) * (m_Levels + 2));
    uint64_t const afterHeaderPos = writer.Pos();

    std::vector<uint64_t> levelOffset;
    {
      std::vector<uint64_t> offsets;
      levelOffset.push_back(writer.Pos());
      BuildLeaves(writer, beg, end, offsets);
      levelOffset.push_back(writer.Pos());
      for (int i = 1; i <= static_cast<int>(m_Levels); ++i)
      {
        std::vector<uint64_t> nextOffsets;
        BuildLevel(writer, beg, end, i, &offsets[0], &offsets[0] + offsets.size(), nextOffsets);
        nextOffsets.swap(offsets);
        levelOffset.push_back(writer.Pos());
      }
    }

    uint64_t const lastPos = writer.Pos();
    writer.Seek(initialPos);

    // Write header.
    {
      IntervalIndexBase::Header header;
      header.m_Version = static_cast<uint8_t>(m_version);
      header.m_BitsPerLevel = static_cast<uint8_t>(m_BitsPerLevel);
      ASSERT_EQUAL(header.m_BitsPerLevel, m_BitsPerLevel, ());
      header.m_Levels = static_cast<uint8_t>(m_Levels);
      ASSERT_EQUAL(header.m_Levels, m_Levels, ());
      header.m_LeafBytes = static_cast<uint8_t>(m_LeafBytes);
      ASSERT_EQUAL(header.m_LeafBytes, m_LeafBytes, ());
      writer.Write(&header, sizeof(header));
    }

    // Write level offsets.
    for (size_t i = 0; i < levelOffset.size(); ++i)
    {
      if (m_version == IntervalIndexVersion::V1)
        WriteToSink(writer, base::checked_cast<uint32_t>(levelOffset[i]));
      else
        WriteToSink(writer, levelOffset[i]);
    }

    uint64_t const pos = writer.Pos();
    CHECK_EQUAL(pos, afterHeaderPos, ());
    writer.Seek(lastPos);
  }

  template <typename CellIdValueIter>
  bool CheckIntervalIndexInputSequence(CellIdValueIter const & beg, CellIdValueIter const & end)
  {
    // Check that [beg, end) is sorted and log most populous cell.
    if (beg != end)
    {
      uint64_t count = 0;
      uint64_t maxCount = 0;
      typename CellIdValueIter::value_type mostPopulousCell = *beg;
      CellIdValueIter it = beg;
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
        LOG(LINFO, ("Most populous cell:", maxCount, mostPopulousCell.GetCell(),
                    mostPopulousCell.GetValue()));
      }
    }

    uint32_t const keyBits = 8 * m_LeafBytes + m_Levels * m_BitsPerLevel;
    for (CellIdValueIter it = beg; it != end; ++it)
    {
      CHECK_LESS(it->GetCell(), 1ULL << keyBits, ());
      // We use static_cast<int64_t>(value) in BuildLeaves to store values difference as VarInt.
      CHECK_LESS_OR_EQUAL(it->GetValue(), static_cast<uint64_t>(std::numeric_limits<int64_t>::max()), ());
    }

    return true;
  }

  template <class SinkT>
  uint64_t WriteNode(SinkT & sink, uint64_t offset, uint64_t * childSizes)
  {
    std::vector<uint8_t> bitmapSerial, listSerial;
    bitmapSerial.reserve(1024);
    listSerial.reserve(1024);
    PushBackByteSink<std::vector<uint8_t> > bitmapSink(bitmapSerial), listSink(listSerial);
    WriteBitmapNode(bitmapSink, offset, childSizes);
    WriteListNode(listSink, offset, childSizes);
    if (bitmapSerial.size() <= listSerial.size())
    {
      sink.Write(&bitmapSerial[0], bitmapSerial.size());
      ASSERT_EQUAL(bitmapSerial.size(), static_cast<uint32_t>(bitmapSerial.size()), ());
      return bitmapSerial.size();
    }
    else
    {
      sink.Write(&listSerial[0], listSerial.size());
      ASSERT_EQUAL(listSerial.size(), static_cast<uint32_t>(listSerial.size()), ());
      return listSerial.size();
    }
  }

  template <class Writer, typename CellIdValueIter>
  void BuildLevel(Writer & writer, CellIdValueIter const & beg, CellIdValueIter const & end,
                  int level, uint64_t const * childSizesBeg, uint64_t const * childSizesEnd,
                  std::vector<uint64_t> & sizes)
  {
    UNUSED_VALUE(childSizesEnd);
    ASSERT_GREATER(level, 0, ());
    uint32_t const skipBits = m_LeafBytes * 8 + (level - 1) * m_BitsPerLevel;
    std::vector<uint64_t> expandedSizes(1 << m_BitsPerLevel);
    uint64_t prevKey = static_cast<uint64_t>(-1);
    uint64_t childOffset = 0;
    uint64_t nextChildOffset = 0;
    for (CellIdValueIter it = beg; it != end; ++it)
    {
      uint64_t const key = it->GetCell() >> skipBits;
      if (key == prevKey)
        continue;

      if (it != beg && (key >> m_BitsPerLevel) != (prevKey >> m_BitsPerLevel))
      {
        sizes.push_back(WriteNode(writer, childOffset, &expandedSizes[0]));
        childOffset = nextChildOffset;
        expandedSizes.assign(expandedSizes.size(), 0);
      }

      nextChildOffset += *childSizesBeg;
      CHECK_EQUAL(expandedSizes[key & m_LastBitsMask], 0, ());
      expandedSizes[key & m_LastBitsMask] = *childSizesBeg;
      ++childSizesBeg;
      prevKey = key;
    }
    sizes.push_back(WriteNode(writer, childOffset, &expandedSizes[0]));
    ASSERT_EQUAL(childSizesBeg, childSizesEnd, ());
  }

  template <class Writer, typename CellIdValueIter>
  void BuildLeaves(Writer & writer, CellIdValueIter const & beg, CellIdValueIter const & end,
                   std::vector<uint64_t> & sizes)
  {
    using Value = typename CellIdValueIter::value_type::ValueType;

    uint32_t const skipBits = 8 * m_LeafBytes;
    uint64_t prevKey = 0;
    uint64_t prevValue = 0;
    uint64_t prevPos = writer.Pos();
    for (CellIdValueIter it = beg; it != end; ++it)
    {
      uint64_t const key = it->GetCell();
      Value const value = it->GetValue();
      if (it != beg && (key >> skipBits) != (prevKey >> skipBits))
      {
        sizes.push_back(writer.Pos() - prevPos);
        prevValue = 0;
        prevPos = writer.Pos();
      }
      uint64_t const keySerial = SwapIfBigEndianMacroBased(key);
      writer.Write(&keySerial, m_LeafBytes);
      WriteVarInt(writer, static_cast<int64_t>(value) - static_cast<int64_t>(prevValue));
      prevKey = key;
      prevValue = value;
    }
    sizes.push_back(writer.Pos() - prevPos);
  }

  template <class SinkT>
  void WriteBitmapNode(SinkT & sink, uint64_t offset, uint64_t * childSizes)
  {
    ASSERT_GREATER_OR_EQUAL(m_BitsPerLevel, 3, ());

    if (m_version == IntervalIndexVersion::V1)
      CHECK_LESS_OR_EQUAL(offset, std::numeric_limits<uint32_t>::max() >> 1, ());
    else
      CHECK_LESS_OR_EQUAL(offset, std::numeric_limits<uint64_t>::max() >> 1, ());
    uint64_t const offsetAndFlag = (offset << 1) + 1;
    WriteVarUint(sink, offsetAndFlag);

    buffer_vector<uint8_t, 32> bitMask(1 << (m_BitsPerLevel - 3));
    for (uint32_t i = 0; i < static_cast<uint32_t>(1 << m_BitsPerLevel); ++i)
      if (childSizes[i])
        bits::SetBitTo1(&bitMask[0], i);
    sink.Write(&bitMask[0], bitMask.size());

    for (uint32_t i = 0; i < static_cast<uint32_t>(1 << m_BitsPerLevel); ++i)
    {
      uint64_t size = childSizes[i];
      if (!size)
        continue;

      if (m_version == IntervalIndexVersion::V1)
        CHECK_LESS_OR_EQUAL(size, std::numeric_limits<uint32_t>::max(), ());
      WriteVarUint(sink, size);
    }
  }

  template <class SinkT>
  void WriteListNode(SinkT & sink, uint64_t offset, uint64_t * childSizes)
  {
    ASSERT_LESS_OR_EQUAL(m_BitsPerLevel, 8, ());

    if (m_version == IntervalIndexVersion::V1)
      CHECK_LESS_OR_EQUAL(offset, std::numeric_limits<uint32_t>::max() >> 1, ());
    else
      CHECK_LESS_OR_EQUAL(offset, std::numeric_limits<uint64_t>::max() >> 1, ());
    uint64_t const offsetAndFlag = offset << 1;
    WriteVarUint(sink, offsetAndFlag);

    for (uint32_t i = 0; i < static_cast<uint32_t>(1 << m_BitsPerLevel); ++i)
    {
      uint64_t size = childSizes[i];
      if (!size)
        continue;

      WriteToSink(sink, static_cast<uint8_t>(i));

      if (m_version == IntervalIndexVersion::V1)
        CHECK_LESS_OR_EQUAL(size, std::numeric_limits<uint32_t>::max(), ());
      WriteVarUint(sink, size);
    }
  }

private:
  IntervalIndexVersion m_version;
  uint32_t m_Levels, m_BitsPerLevel, m_LeafBytes, m_LastBitsMask;
};

template <class Writer, typename CellIdValueIter>
void BuildIntervalIndex(CellIdValueIter const & beg, CellIdValueIter const & end, Writer & writer,
                        uint32_t keyBits,
                        IntervalIndexVersion version = IntervalIndexVersion::V1)
{
  IntervalIndexBuilder(version, keyBits, 1).BuildIndex(writer, beg, end);
}
