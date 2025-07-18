#pragma once

#include "coding/reader.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

/// @TODO Add Windows support
class MmapReader : public ModelReader
{
public:
  enum class Advice
  {
    Normal,
    Random,
    Sequential
  };

  explicit MmapReader(std::string const & fileName, Advice advice = Advice::Normal);

  uint64_t Size() const override;
  void Read(uint64_t pos, void * p, size_t size) const override;
  std::unique_ptr<Reader> CreateSubReader(uint64_t pos, uint64_t size) const override;

  /// Direct file/memory access
  uint8_t * Data() const;

protected:
  // Used in special derived readers.
  void SetOffsetAndSize(uint64_t offset, uint64_t size);

private:
  using base_type = ModelReader;
  class MmapData;

  MmapReader(MmapReader const & reader, uint64_t offset, uint64_t size);

  std::shared_ptr<MmapData> m_data;
  uint64_t m_offset;
  uint64_t m_size;
};
