#include "drape/utils/gpu_mem_tracker.hpp"

#include <sstream>

namespace dp
{
std::string GPUMemTracker::GPUMemorySnapshot::ToString() const
{
  std::ostringstream ss;
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
    std::lock_guard<std::mutex> g(m_mutex);
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

  auto constexpr kByteToMb = static_cast<float>(1024 * 1024);
  for (auto & it : memStat.m_tagStats)
  {
    it.second.m_alocatedInMb /= kByteToMb;
    it.second.m_usedInMb /= kByteToMb;
  }
  memStat.m_summaryAllocatedInMb /= kByteToMb;
  memStat.m_summaryUsedInMb /= kByteToMb;

  return memStat;
}

void GPUMemTracker::AddAllocated(std::string const & tag, uint32_t id, uint32_t size)
{
  std::lock_guard<std::mutex> g(m_mutex);
  m_memTracker[make_pair(tag, id)].first = size;
}

void GPUMemTracker::SetUsed(std::string const & tag, uint32_t id, uint32_t size)
{
  std::lock_guard<std::mutex> g(m_mutex);
  TAlocUsedMem & node = m_memTracker[make_pair(tag, id)];
  node.second = size;
  ASSERT_LESS_OR_EQUAL(node.second, node.first, ("Can't use more than allocated"));
}

void GPUMemTracker::RemoveDeallocated(std::string const & tag, uint32_t id)
{
  std::lock_guard<std::mutex> g(m_mutex);
  m_memTracker.erase(make_pair(tag, id));
}
}  // namespace dp
