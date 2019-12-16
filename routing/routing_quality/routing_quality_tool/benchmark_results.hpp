#pragma once

#include "routing/routing_callbacks.hpp"

#include <map>
#include <string>
#include <vector>

namespace routing_quality::routing_quality_tool
{
class BenchmarkResults
{
public:
  void PushError(routing::RouterResultCode code);
  double GetAverageBuildTime() const;
  void PushBuildTime(double time);

  std::vector<double> const & GetBuildTimes() const { return m_buildTimes; }
  std::map<std::string, size_t> const & GetErrorsDistribution() const { return m_errorCounter; }

private:
  double m_summaryBuildTimeSeconds = 0.0;
  // m_buildTimes[i] stores build time of i-th route.
  std::vector<double> m_buildTimes;
  // string representation of RouterResultCode to number of such codes.
  std::map<std::string, size_t> m_errorCounter;
};
}  // namespace routing_quality::routing_quality_tool
