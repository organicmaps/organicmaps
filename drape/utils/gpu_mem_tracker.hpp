#pragma once

#include "../../base/mutex.hpp"

#include "../../std/map.hpp"
#include "../../std/noncopyable.hpp"

//#define TRACK_GPU_MEM

namespace dp
{

class GPUMemTracker : private noncopyable
{
public:
  static GPUMemTracker & Inst();

  string Report();
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
