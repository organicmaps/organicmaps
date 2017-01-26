#pragma once

#include "drape/drape_diagnostics.hpp"

#include "base/mutex.hpp"

#include "std/map.hpp"
#include "std/noncopyable.hpp"
#include "std/string.hpp"
#include "std/utility.hpp"

namespace dp
{

class GPUMemTracker : private noncopyable
{
public:
  struct TagMemorySnapshot
  {
    uint32_t m_objectsCount = 0;
    uint32_t m_alocatedInMb = 0;
    uint32_t m_usedInMb = 0;
  };

  struct GPUMemorySnapshot
  {
    string ToString() const;

    uint32_t m_summaryAllocatedInMb = 0;
    uint32_t m_summaryUsedInMb = 0;
    map<string, TagMemorySnapshot> m_tagStats;
  };

  static GPUMemTracker & Inst();

  GPUMemorySnapshot GetMemorySnapshot();

  void AddAllocated(string const & tag, uint32_t id, uint32_t size);
  void SetUsed(string const & tag, uint32_t id, uint32_t size);
  void RemoveDeallocated(string const & tag, uint32_t id);

private:
  GPUMemTracker() = default;

private:
  typedef pair<uint32_t, uint32_t> TAlocUsedMem;
  typedef pair<string, uint32_t> TMemTag;
  map<TMemTag, TAlocUsedMem> m_memTracker;

  threads::Mutex m_mutex;
};

} // namespace dp
