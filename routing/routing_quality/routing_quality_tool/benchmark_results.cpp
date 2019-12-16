#include "routing/routing_quality/routing_quality_tool/benchmark_results.hpp"

namespace routing_quality::routing_quality_tool
{
double BenchmarkResults::GetAverageBuildTime() const
{
  auto const n = m_errorCounter.at(routing::ToString(routing::RouterResultCode::NoError));
  if (n == 0)
    return 0.0;

  return m_summaryBuildTimeSeconds / static_cast<double>(n);
}

void BenchmarkResults::PushBuildTime(double time)
{
  m_summaryBuildTimeSeconds += time;
  m_buildTimes.emplace_back(time);
}

void BenchmarkResults::PushError(routing::RouterResultCode code)
{
  ++m_errorCounter[routing::ToString(code)];
}
}  // namespace routing_quality::routing_quality_tool
