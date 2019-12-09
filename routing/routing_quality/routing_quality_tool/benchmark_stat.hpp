#pragma once

#include "routing/routes_builder/routes_builder.hpp"

#include "routing/routing_callbacks.hpp"

#include <string>
#include <utility>
#include <vector>

namespace routing_quality
{
namespace routing_quality_tool
{
class BenchmarkResults
{
public:
  void PushError(routing::RouterResultCode code)
  {
    ++m_errorCounter[DebugPrint(code)];
  }

  double GetAverageBuildTime()
  {
    auto const n = m_errorCounter[DebugPrint(routing::RouterResultCode::NoError)];
    if (n == 0)
      return 0.0;

    return m_summaryBuildTimeSeconds / static_cast<double>(n);
  }

  void PushBuildTime(double time)
  {
    m_summaryBuildTimeSeconds += time;
    m_buildTimes.emplace_back(time);
  }

  std::vector<double> const & GetBuildTimes() const { return m_buildTimes; }
  std::map<std::string, size_t> const & GetErrorsDistribution() const { return m_errorCounter; }

private:
  double m_summaryBuildTimeSeconds = 0.0;
  // m_buildTimes[i] stores build time of i-th route.
  std::vector<double> m_buildTimes;
  // string representation of RouterResultCode to number of such codes.
  std::map<std::string, size_t> m_errorCounter;
};

using RouteResult = routing::routes_builder::RoutesBuilder::Result;
void RunBenchmarkStat(std::vector<std::pair<RouteResult, std::string>> const & mapsmeResults,
                      std::string const & dirForResults);

void RunBenchmarkComparison(
    std::vector<std::pair<RouteResult, std::string>> && mapsmeResults,
    std::vector<std::pair<RouteResult, std::string>> && mapsmeOldResults,
    std::string const & dirForResults);
}  // namespace routing_quality_tool
}  // namespace routing_quality
