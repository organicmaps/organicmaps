#include "routing/routing_quality/routing_quality_tool/benchmark_stat.hpp"

#include "routing/routing_quality/routing_quality_tool/utils.hpp"

#include "routing/routing_callbacks.hpp"

#include "geometry/point2d.hpp"

#include "base/assert.hpp"
#include "base/file_name_utils.hpp"
#include "base/logging.hpp"

#include <algorithm>
#include <map>

namespace routing_quality
{
namespace routing_quality_tool
{
using namespace routing;
using namespace routes_builder;

// Shows distribution of routes time building.
static std::string const kPythonDistTimeBuilding = "show_route_time_building_dist.py";
// Shows graph time(dist).
static std::string const kPythonGraphLenAndTime = "show_len_time_graph.py";
// Shows graph of "how many routes in percents build in less time than some T".
static std::string const kPythonGraphTimeAndCount = "show_time_count_graph.py";
// Bar graph of routing errors. Labels - string representation of errors, heights - number of such
// errors.
static std::string const kPythonBarError = "show_errors_bar.py";

void RunBenchmarkStat(
    std::vector<std::pair<RoutesBuilder::Result, std::string>> const & mapsmeResults,
    std::string const & dirForResults)
{
  double averageTimeSeconds = 0.0;
  std::vector<m2::PointD> distToTime;
  std::vector<double> times;
  times.reserve(mapsmeResults.size());

  std::map<std::string, size_t> m_errorCounter;

  for (auto const & resultItem : mapsmeResults)
  {
    auto const & result = resultItem.first;
    ++m_errorCounter[DebugPrint(result.m_code)];
    if (result.m_code != RouterResultCode::NoError)
      continue;

    CHECK(!result.m_routes.empty(), ());
    averageTimeSeconds += result.m_buildTimeSeconds;
    distToTime.emplace_back(result.m_routes.back().m_distance, result.m_buildTimeSeconds);
    times.emplace_back(result.m_buildTimeSeconds);
  }

  auto pythonScriptPath = base::JoinPath(dirForResults, kPythonDistTimeBuilding);
  CreatePythonScriptForDistribution(pythonScriptPath, "Route building time, seconds", times);

  pythonScriptPath = base::JoinPath(dirForResults, kPythonGraphLenAndTime);
  std::sort(distToTime.begin(), distToTime.end(),
            [](auto const & lhs, auto const & rhs) { return lhs.x < rhs.x; });
  CreatePythonGraphByPointsXY(pythonScriptPath, "Distance", "Building time", distToTime);

  averageTimeSeconds /= static_cast<double>(
      mapsmeResults.empty() ? 1.0 : m_errorCounter[DebugPrint(RouterResultCode::NoError)]);

  LOG(LINFO, ("Average route time building:", averageTimeSeconds, "seconds."));

  std::sort(times.begin(), times.end());
  std::vector<m2::PointD> countToTimes(times.size());
  for (size_t i = 0; i < countToTimes.size(); ++i)
  {
    countToTimes[i].x = times[i];
    countToTimes[i].y = static_cast<double>(i + 1) / times.size() * 100.0;
  }

  pythonScriptPath = base::JoinPath(dirForResults, kPythonGraphTimeAndCount);
  CreatePythonGraphByPointsXY(pythonScriptPath, "Building time, seconds",
                              "Percent of routes builded less than", countToTimes);

  pythonScriptPath = base::JoinPath(dirForResults, kPythonBarError);
  CreatePythonBarByMap(pythonScriptPath, m_errorCounter, "Type of errors", "Number of errors");
}

void RunBenchmarkComparison(
    std::vector<std::pair<RoutesBuilder::Result, std::string>> && mapsmeResults,
    std::vector<std::pair<RoutesBuilder::Result, std::string>> && mapsmeOldResults,
    std::string const & dirForResults)
{
  UNREACHABLE();
}
}  // namespace routing_quality_tool
}  // namespace routing_quality
