#include "gpu_mem_tracker.hpp"

#include "../../std/tuple.hpp"
#include "../../std/sstream.hpp"

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
    stat.get<0>()++;
    stat.get<1>() += it.second.first;
    stat.get<2>() += it.second.second;

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
    ss << "   Object count = " << it.second.get<0>() << "\n";
    ss << "   Allocated    = " << it.second.get<1>() / byteToMb << "\n";
    ss << "   Used         = " << it.second.get<2>() / byteToMb << "\n";
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
