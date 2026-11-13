#pragma once

#include "base/assert.hpp"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>

namespace indexer_tests
{
/// A reader of a buffer that counts what is read from it. The templates on a reader take it for a
/// reader of memory, or for another one, like a reader of a file.
template <bool IsMemoryReader>
class CountingReader
{
public:
  static constexpr bool kIsMemoryReader = IsMemoryReader;

  explicit CountingReader(std::vector<uint8_t> const & bytes) : m_bytes(bytes) {}

  uint64_t Size() const { return m_bytes.size(); }

  void Read(uint64_t pos, void * p, size_t size) const
  {
    CHECK_LESS_OR_EQUAL(pos + size, m_bytes.size(), ());
    memcpy(p, m_bytes.data() + pos, size);
    ++m_reads;
    m_readBytes += size;
  }

  uint64_t GetReads() const { return m_reads; }
  uint64_t GetReadBytes() const { return m_readBytes; }
  void ResetCounters() const { m_reads = m_readBytes = 0; }

private:
  std::vector<uint8_t> const & m_bytes;
  mutable uint64_t m_reads = 0;
  mutable uint64_t m_readBytes = 0;
};
}  // namespace indexer_tests
