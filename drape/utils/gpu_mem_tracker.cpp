#include "drape/utils/gpu_mem_tracker.hpp"

#include "std/tuple.hpp"
#include "std/sstream.hpp"

namespace dp
{

GPUMemTracker & GPUMemTracker::Inst()
{
  static GPUMemTracker s_inst;
  return s_inst;
}

string GPUMemTracker::Report()
{
  uint32_t summaryUsed = 0;
  uint32_t summaryAllocated = 0;

  typedef tuple<size_t, uint32_t, uint32_t> TTagStat;
  map<string, TTagStat> tagStats;

  for (auto const it : m_memTracker)
  {
    TTagStat & stat = tagStats[it.first.first];
    get<0>(stat)++;
    get<1>(stat) += it.second.first;
    get<2>(stat) += it.second.second;

    summaryAllocated += it.second.first;
    summaryUsed += it.second.second;
  }

  float byteToMb = static_cast<float>(1024 * 1024);

  ostringstream ss;
  ss << " ===== Mem Report ===== \n";
  ss << " Summary Allocated = " << summaryAllocated / byteToMb << "\n";
  ss << " Summary Used = " << summaryUsed / byteToMb << "\n";
  ss << " Tags registered = " << tagStats.size() << "\n";

  for (auto const it : tagStats)
  {
    ss << " Tag = " << it.first << " \n";
    ss << "   Object count = " << get<0>(it.second) << "\n";
    ss << "   Allocated    = " << get<1>(it.second) / byteToMb << "\n";
    ss << "   Used         = " << get<2>(it.second) / byteToMb << "\n";
  }

  ss << " ===== Mem Report ===== \n";

  return move(ss.str());
}

void GPUMemTracker::AddAllocated(string const & tag, uint32_t id, uint32_t size)
{
  threads::MutexGuard g(m_mutex);
  m_memTracker[make_pair(tag, id)].first = size;
}

void GPUMemTracker::SetUsed(string const & tag, uint32_t id, uint32_t size)
{
  threads::MutexGuard g(m_mutex);
  TAlocUsedMem & node = m_memTracker[make_pair(tag, id)];
  node.second = size;
  ASSERT_LESS_OR_EQUAL(node.second, node.first, ("Can't use more then allocated"));
}

void GPUMemTracker::RemoveDeallocated(string const & tag, uint32_t id)
{
  threads::MutexGuard g(m_mutex);
  m_memTracker.erase(make_pair(tag, id));
}

} // namespace dp
