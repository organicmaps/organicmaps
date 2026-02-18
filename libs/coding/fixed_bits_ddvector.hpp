#pragma once

#include "bit_streams.hpp"
#include "byte_stream.hpp"
#include "dd_vector.hpp"
#include "reader.hpp"
#include "write_to_sink.hpp"

#include <algorithm>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

/// Disk driven vector for optimal storing small values with rare big values.
/// Format:
/// 4 bytes to store vector's size
/// Buffer of ceil(Size * Bits / 8) bytes, e.g. vector of Bits-sized elements.
/// - values in range [0, (1 << Bits) - 2] stored as is
/// - value (1 << Bits) - 2 tells that actual value is stored in the exceptions table below.
/// - value (1 << Bits) - 1 tells that the value is undefined.
/// Buffer with exceptions table, e.g. vector of (index, value) pairs till the end of the reader,
/// sorted by index parameter.
/// Component is stored and used in host's endianness, without any conversions.

template <size_t Bits,                /// number of fixed bits
          class TReader,              /// reader with random offset read functions
          typename TSize = uint32_t,  /// vector index type (platform independent)
          typename TValue = uint32_t  /// vector value type (platform independent)
          >
class FixedBitsDDVector
{
  static_assert(std::is_unsigned<TSize>::value, "");
  static_assert(std::is_unsigned<TValue>::value, "");
  // 16 - is the maximum bits count to get all needed bits in random access within uint32_t.
  static_assert(Bits > 0, "");
  static_assert(Bits <= 16, "");

  using TSelf = FixedBitsDDVector<Bits, TReader, TSize, TValue>;

  struct IndexValue
  {
    TSize m_index;
    TValue m_value;
    bool operator<(IndexValue const & rhs) const { return m_index < rhs.m_index; }
  };

  TReader m_bits;
  DDVector<IndexValue, TReader, TSize> m_vector;

#ifdef DEBUG
  TSize const m_size;
#endif

  using TBlock = uint32_t;

  static uint64_t AlignBytesCount(uint64_t count) { return std::max(count, static_cast<uint64_t>(sizeof(TBlock))); }

  static TBlock constexpr kMask = (1 << Bits) - 1;
  static TBlock constexpr kLargeValue = kMask - 1;
  static TBlock constexpr kUndefined = kMask;

  TValue FindInVector(TSize index) const
  {
    auto const it = std::lower_bound(m_vector.begin(), m_vector.end(), IndexValue{index, 0});
    ASSERT(it != m_vector.end() && it->m_index == index, ());
    return it->m_value;
  }

  FixedBitsDDVector(TReader const & bitsReader, TReader const & vecReader, TSize size)
    : m_bits(bitsReader)
    , m_vector(vecReader)
#ifdef DEBUG
    , m_size(size)
#endif
  {}

public:
  static std::unique_ptr<TSelf> Create(TReader const & reader)
  {
    TSize const size = ReadPrimitiveFromPos<TSize>(reader, 0);

    uint64_t const off1 = sizeof(TSize);
    uint64_t const off2 = AlignBytesCount((size * Bits + CHAR_BIT - 1) / CHAR_BIT) + off1;

    // We cannot use make_unique here because contsructor is private.
    return std::unique_ptr<TSelf>(
        new TSelf(reader.SubReader(off1, off2 - off1), reader.SubReader(off2, reader.Size() - off2), size));
  }

  bool Get(TSize index, TValue & value) const
  {
    ASSERT_LESS(index, m_size, ());
    uint64_t const bitsOffset = index * Bits;

    uint64_t bytesOffset = bitsOffset / CHAR_BIT;
    size_t constexpr kBlockSize = sizeof(TBlock);
    if (bytesOffset + kBlockSize > m_bits.Size())
      bytesOffset = m_bits.Size() - kBlockSize;

    TBlock v = ReadPrimitiveFromPos<TBlock>(m_bits, bytesOffset);
    v >>= (bitsOffset - bytesOffset * CHAR_BIT);
    v &= kMask;
    if (v == kUndefined)
      return false;

    value = v < kLargeValue ? v : FindInVector(index);
    return true;
  }

  template <class TWriter>
  class Builder
  {
    using TData = std::vector<uint8_t>;
    using TempWriter = PushBackByteSink<TData>;
    using TBits = BitWriter<TempWriter>;

    TData m_data;
    TempWriter m_writer;
    std::unique_ptr<TBits> m_bits;

    std::vector<IndexValue> m_excepts;
    TSize m_count = 0;
    TSize m_optCount = 0;

    TWriter & m_finalWriter;

  public:
    using ValueType = TValue;

    explicit Builder(TWriter & writer) : m_writer(m_data), m_bits(new TBits(m_writer)), m_finalWriter(writer) {}

    ~Builder()
    {
      // Final serialization is in dtor only.
      // You can't do any intermediate flushes during building vector.

      // Reset the bit stream first.
      m_bits.reset();

      // Write size of vector.
      WriteToSink(m_finalWriter, m_count);

      // Write bits vector, alignes at least to 4 bytes.
      m_data.resize(AlignBytesCount(m_data.size()));
      m_finalWriter.Write(m_data.data(), m_data.size());

      // Write exceptions table.
      m_finalWriter.Write(m_excepts.data(), m_excepts.size() * sizeof(IndexValue));
    }

    void PushBack(TValue v)
    {
      if (v >= kLargeValue)
      {
        m_bits->WriteAtMost32Bits(kLargeValue, Bits);
        m_excepts.push_back({m_count, v});
      }
      else
      {
        ++m_optCount;
        m_bits->WriteAtMost32Bits(v, Bits);
      }

      ++m_count;
    }

    // Pushes a special (undefined) value.
    void PushBackUndefined()
    {
      m_bits->WriteAtMost32Bits(kUndefined, Bits);
      ++m_optCount;
      ++m_count;
    }

    /// @return (number of stored as-is elements, number of all elements)
    std::pair<TSize, TSize> GetCount() const { return std::make_pair(m_optCount, m_count); }
  };
};
