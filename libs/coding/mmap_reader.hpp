#pragma once

#include "coding/reader.hpp"

#include <memory>
#include <string>

/// @note Not thread-safe like all readers.
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

  class MmapData;
  MmapReader(std::string const & fileName, std::shared_ptr<MmapData> data);

  /// @return Data handle that can be shared via multiple readers in different threads.
  std::shared_ptr<MmapData> GetInnerData() const { return m_data; }

  uint64_t Size() const override;
  void Read(uint64_t pos, void * p, size_t size) const override;
  std::unique_ptr<MemoryRegion> GetMemoryRegion(uint64_t pos, size_t size) const override;
  std::unique_ptr<Reader> CreateSubReader(uint64_t pos, uint64_t size) const override;

protected:
  void CheckPosAndSize(uint64_t pos, uint64_t size) const;
  // Used in special derived readers.
  void SetOffsetAndSize(uint64_t offset, uint64_t size);

private:
  using base_type = ModelReader;

  MmapReader(MmapReader const & reader, uint64_t offset, uint64_t size);

  std::shared_ptr<MmapData> m_data;
  uint64_t m_offset;
  uint64_t m_size;
};
