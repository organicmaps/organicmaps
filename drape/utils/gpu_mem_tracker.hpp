#pragma once

#include "drape/drape_diagnostics.hpp"

#include "base/macros.hpp"

#include <cstdint>
#include <map>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>

namespace dp
{
class GPUMemTracker
{
public:
  struct TagMemorySnapshot
  {
    uint32_t m_objectsCount = 0;
    uint32_t m_alocatedInMb = 0;
    uint32_t m_usedInMb = 0;
  };

  struct AverageAllocation
  {
    uint64_t m_totalSizeInBytes = 0;
    uint32_t m_count = 0;

    uint64_t GetAverage() const { return m_count == 0 ? 0 : m_totalSizeInBytes / m_count; }
  };

  struct GPUMemorySnapshot
  {
    std::string ToString() const;

    uint32_t m_summaryAllocatedInMb = 0;
    uint32_t m_summaryUsedInMb = 0;
    std::map<std::string, TagMemorySnapshot> m_tagStats;
    std::unordered_map<uint64_t, AverageAllocation> m_averageAllocations;
  };

  static GPUMemTracker & Inst();

  GPUMemorySnapshot GetMemorySnapshot();

  void AddAllocated(std::string const & tag, uint32_t id, uint32_t size);
  void SetUsed(std::string const & tag, uint32_t id, uint32_t size);
  void RemoveDeallocated(std::string const & tag, uint32_t id);

  void TrackAverageAllocation(uint64_t hash, uint64_t size);

private:
  GPUMemTracker() = default;

  using TAlocUsedMem = std::pair<uint32_t, uint32_t>;
  using TMemTag = std::pair<std::string, uint32_t>;
  std::map<TMemTag, TAlocUsedMem> m_memTracker;
  std::unordered_map<uint64_t, AverageAllocation> m_averageAllocations;

  std::mutex m_mutex;

  DISALLOW_COPY_AND_MOVE(GPUMemTracker);
};
}  // namespace dp
