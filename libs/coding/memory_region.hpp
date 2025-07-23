#pragma once

#include "coding/files_container.hpp"

#include "base/macros.hpp"

#include <cstdint>
#include <utility>
#include <vector>

class MemoryRegion
{
public:
  virtual ~MemoryRegion() = default;

  virtual uint64_t Size() const = 0;
  virtual uint8_t const * ImmutableData() const = 0;
};

class MappedMemoryRegion : public MemoryRegion
{
public:
  explicit MappedMemoryRegion(FilesMappingContainer::Handle && handle) : m_handle(std::move(handle)) {}

  // MemoryRegion overrides:
  uint64_t Size() const override { return m_handle.GetSize(); }
  uint8_t const * ImmutableData() const override { return m_handle.GetData<uint8_t>(); }

private:
  FilesMappingContainer::Handle m_handle;

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
