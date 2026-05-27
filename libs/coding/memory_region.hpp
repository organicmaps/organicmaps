#pragma once

#include "coding/mmap_reader.hpp"

#include "base/macros.hpp"

#include <vector>

class MappedMemoryRegion : public MemoryRegion
{
public:
  MappedMemoryRegion(uint8_t const * ptr, size_t size, std::shared_ptr<MmapReader::MmapData> handle)
    : m_ptr(ptr)
    , m_size(size)
    , m_handle(std::move(handle))
  {}

  // MemoryRegion overrides:
  uint64_t Size() const override { return m_size; }
  uint8_t const * ImmutableData() const override { return m_ptr; }

private:
  uint8_t const * m_ptr;
  size_t m_size;
  std::shared_ptr<MmapReader::MmapData> m_handle;

  DISALLOW_COPY(MappedMemoryRegion);
};

class CopiedMemoryRegion : public MemoryRegion
{
public:
  explicit CopiedMemoryRegion(std::vector<uint8_t> && buffer) : m_buffer(std::move(buffer)) {}

  // MemoryRegion overrides:
  uint64_t Size() const override { return m_buffer.size(); }
  uint8_t const * ImmutableData() const override { return m_buffer.data(); }

  uint8_t * MutableData() { return m_buffer.data(); }

private:
  std::vector<uint8_t> m_buffer;

  DISALLOW_COPY(CopiedMemoryRegion);
};
