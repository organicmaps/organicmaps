#pragma once

#include "coding/file_container.hpp"

#include "base/macros.hpp"

#include "std/cstdint.hpp"
#include "std/utility.hpp"
#include "std/vector.hpp"

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
  explicit MappedMemoryRegion(FilesMappingContainer::Handle && handle) : m_handle(move(handle)) {}

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
  explicit CopiedMemoryRegion(vector<uint8_t> && buffer) : m_buffer(move(buffer)) {}

  // MemoryRegion overrides:
  uint64_t Size() const override { return m_buffer.size(); }
  uint8_t const * ImmutableData() const override { return m_buffer.data(); }

  inline uint8_t * MutableData() { return m_buffer.data(); }

private:
  vector<uint8_t> m_buffer;

  DISALLOW_COPY(CopiedMemoryRegion);
};
