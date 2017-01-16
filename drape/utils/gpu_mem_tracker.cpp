#include "drape/utils/gpu_mem_tracker.hpp"

#include "std/tuple.hpp"
#include "std/sstream.hpp"

namespace dp
{

string GPUMemTracker::GPUMemorySnapshot::ToString() const
{
  ostringstream ss;
  ss << " Summary Allocated = " << m_summaryAllocatedInMb << "Mb\n";
  ss << " Summary Used = " << m_summaryUsedInMb << "Mb\n";
  ss << " Tags registered = " << m_tagStats.size() << "\n";

  for (auto const it : m_tagStats)
  {
    ss << " Tag = " << it.first << " \n";
    ss << "   Object count = " << it.second.m_objectsCount << "\n";
    ss << "   Allocated    = " << it.second.m_alocatedInMb << "Mb\n";
    ss << "   Used         = " << it.second.m_usedInMb << "Mb\n";
  }

  return ss.str();
}

GPUMemTracker & GPUMemTracker::Inst()
{
  static GPUMemTracker s_inst;
  return s_inst;
}

GPUMemTracker::GPUMemorySnapshot GPUMemTracker::GetMemorySnapshot()
{
  GPUMemorySnapshot memStat;

  {
    threads::MutexGuard g(m_mutex);
    for (auto const it : m_memTracker)
    {
      TagMemorySnapshot & tagStat = memStat.m_tagStats[it.first.first];
      tagStat.m_objectsCount++;
      tagStat.m_alocatedInMb += it.second.first;
      tagStat.m_usedInMb += it.second.second;

      memStat.m_summaryAllocatedInMb += it.second.first;
      memStat.m_summaryUsedInMb += it.second.second;
    }
  }

  float byteToMb = static_cast<float>(1024 * 1024);
  for (auto & it : memStat.m_tagStats)
  {
    it.second.m_alocatedInMb /= byteToMb;
    it.second.m_usedInMb /= byteToMb;
  }
  memStat.m_summaryAllocatedInMb /= byteToMb;
  memStat.m_summaryUsedInMb /= byteToMb;

  return memStat;
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
  ASSERT_LESS_OR_EQUAL(node.second, node.first, ("Can't use more than allocated"));
}

void GPUMemTracker::RemoveDeallocated(string const & tag, uint32_t id)
{
  threads::MutexGuard g(m_mutex);
  m_memTracker.erase(make_pair(tag, id));
}

} // namespace dp
