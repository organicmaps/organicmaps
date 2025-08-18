#include "routing/routing_quality/routing_quality_tool/benchmark_results.hpp"

namespace routing_quality::routing_quality_tool
{
double BenchmarkResults::GetAverageBuildTime() const
{
  if (m_buildTimes.empty())
    return 0.0;

  return m_summaryBuildTimeSeconds / static_cast<double>(m_buildTimes.size());
}

void BenchmarkResults::PushBuildTime(double time)
{
  m_summaryBuildTimeSeconds += time;
  m_buildTimes.emplace_back(time);
}
}  // namespace routing_quality::routing_quality_tool
