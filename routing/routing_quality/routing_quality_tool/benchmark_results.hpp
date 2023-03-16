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
  double GetAverageBuildTime() const;
  void PushBuildTime(double time);

  std::vector<double> const & GetBuildTimes() const { return m_buildTimes; }

private:
  double m_summaryBuildTimeSeconds = 0.0;
  // m_buildTimes[i] stores build time of i-th route.
  std::vector<double> m_buildTimes;
};

struct TimeInfo
{
  TimeInfo(double oldTime, double newTime) : m_oldTime(oldTime), m_newTime(newTime) {}
  double m_oldTime;
  double m_newTime;
};
}  // namespace routing_quality::routing_quality_tool
