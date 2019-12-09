#include "routing/routing_quality/routing_quality_tool/benchmark_stat.hpp"

#include "routing/routing_quality/routing_quality_tool/utils.hpp"

#include "routing/routes_builder/routes_builder.hpp"

#include "routing/routing_callbacks.hpp"

#include "geometry/point2d.hpp"

#include "base/assert.hpp"
#include "base/file_name_utils.hpp"
#include "base/logging.hpp"

#include <algorithm>
#include <map>

namespace
{
bool IsErrorCode(routing::RouterResultCode code)
{
  return code != routing::RouterResultCode::NoError;
}
}

namespace routing_quality
{
namespace routing_quality_tool
{
using namespace routing;
using namespace routes_builder;

// Shows distribution of routes time building.
static std::string const kPythonDistTimeBuilding = "show_route_time_building_dist.py";
// Shows graph of "how many routes in percents build in less time than some T".
static std::string const kPythonGraphTimeAndCount = "show_time_count_graph.py";
// Bar graph of routing errors. Labels - string representation of errors, heights - number of such
// errors.
static std::string const kPythonBarError = "show_errors_bar.py";
// TODO comment
static std::string const kPythonBarBoostPercentDistr = "show_boost_distr.py";

void RunBenchmarkStat(
    std::vector<std::pair<RoutesBuilder::Result, std::string>> const & mapsmeResults,
    std::string const & dirForResults)
{
  BenchmarkResults benchmarkResults;
  for (auto const & resultItem : mapsmeResults)
  {
    auto const & result = resultItem.first;
    benchmarkResults.PushError(result.m_code);
    if (result.m_code != RouterResultCode::NoError)
      continue;

    CHECK(!result.m_routes.empty(), ());
    benchmarkResults.PushBuildTime(result.m_buildTimeSeconds);
  }

  auto pythonScriptPath = base::JoinPath(dirForResults, kPythonDistTimeBuilding);
  CreatePythonScriptForDistribution(pythonScriptPath, "Route building time, seconds",
                                    benchmarkResults.GetBuildTimes());

  LOG(LINFO, ("Average route time building:", benchmarkResults.GetAverageBuildTime(), "seconds."));

  auto times = benchmarkResults.GetBuildTimes();
  std::sort(times.begin(), times.end());
  std::vector<m2::PointD> countToTimes(times.size());
  for (size_t i = 0; i < countToTimes.size(); ++i)
  {
    countToTimes[i].x = times[i];
    countToTimes[i].y = static_cast<double>(i + 1) / times.size() * 100.0;
  }

  pythonScriptPath = base::JoinPath(dirForResults, kPythonGraphTimeAndCount);
  CreatePythonGraphByPointsXY(pythonScriptPath,
                              "Building time, seconds" /* xlabel */,
                              "Percent of routes built less than" /* ylabel */,
                              {countToTimes}, {"mapsme"} /* legends */);

  pythonScriptPath = base::JoinPath(dirForResults, kPythonBarError);

  std::vector<std::string> labels;
  std::vector<double> heights;
  for (auto const & item : benchmarkResults.GetErrorsDistribution())
  {
    labels.emplace_back(item.first);
    heights.emplace_back(item.second);
  }

  CreatePythonBarByMap(pythonScriptPath, labels, {heights}, {"mapsme"} /* legends */,
                       "Type of errors" /* xlabel */, "Number of errors" /* ylabel */);
}

WorldGraphMode GetWorldGraphMode(m2::PointD const & start, m2::PointD const & finish)
{
  auto & builder = RoutesBuilder::GetSimpleRoutesBuilder();
  RoutesBuilder::Params params;
  params.m_checkpoints = {start, finish};
  params.m_timeoutSeconds = 0;
  auto const result = builder.ProcessTask(params);
  return result.m_routingAlgorithMode;
}

void RunBenchmarkComparison(
    std::vector<std::pair<RoutesBuilder::Result, std::string>> && mapsmeResults,
    std::vector<std::pair<RoutesBuilder::Result, std::string>> && mapsmeOldResults,
    std::string const & dirForResults)
{
  BenchmarkResults benchmarkResults;
  BenchmarkResults benchmarkOldResults;

  std::vector<std::future<RoutesBuilder::Result>> modes;
  RoutesBuilder builder(std::thread::hardware_concurrency());
  for (auto & item : mapsmeResults)
  {
    auto const & mapsmeResult = item.first;
    RoutesBuilder::Params params;
    auto const & start = mapsmeResult.m_params.m_checkpoints.GetStart();
    auto const & finish = mapsmeResult.m_params.m_checkpoints.GetFinish();
    params.m_checkpoints = {start, finish};
    params.m_timeoutSeconds = 0;
    auto future = builder.ProcessTaskAsync(params);
    modes.emplace_back(std::move(future));
  }

  for (size_t i = 0; i < mapsmeResults.size(); ++i)
  {
    auto const & mapsmeResult = mapsmeResults[i].first;

    std::pair<RoutesBuilder::Result, std::string> mapsmeOldResultPair;
    if (!FindAnotherResponse(mapsmeResult, mapsmeOldResults, mapsmeOldResultPair))
    {
      LOG(LDEBUG, ("Can not find pair for:", i));
      continue;
    }

    auto const result = modes[i].get();
    WorldGraphMode mode = result.m_routingAlgorithMode;
    if (mode != WorldGraphMode::LeapsOnly)
      continue;

    auto const & mapsmeOldResult = mapsmeOldResultPair.first;

    benchmarkResults.PushError(mapsmeResult.m_code);
    benchmarkOldResults.PushError(mapsmeOldResult.m_code);

    if (IsErrorCode(mapsmeResult.m_code) || IsErrorCode(mapsmeOldResult.m_code))
      continue;

    benchmarkResults.PushBuildTime(mapsmeResult.m_buildTimeSeconds);
    benchmarkOldResults.PushBuildTime(mapsmeOldResult.m_buildTimeSeconds);
  }

  LOG(LINFO, ("Get", benchmarkResults.GetBuildTimes().size(), "routes after filter."));

  LOG(LINFO,
      ("Average route time building. "
       "Old version:", benchmarkOldResults.GetAverageBuildTime(),
       "new version:", benchmarkResults.GetAverageBuildTime()));

  std::vector<std::vector<m2::PointD>> graphics;
  for (auto const & results : {benchmarkOldResults, benchmarkResults})
  {
    auto times = results.GetBuildTimes();
    std::sort(times.begin(), times.end());
    std::vector<m2::PointD> countToTimes(times.size());
    for (size_t i = 0; i < countToTimes.size(); ++i)
    {
      countToTimes[i].x = times[i];
      countToTimes[i].y = static_cast<double>(i + 1) / times.size() * 100.0;
    }

    graphics.emplace_back(std::move(countToTimes));
  }

  auto pythonScriptPath = base::JoinPath(dirForResults, kPythonGraphTimeAndCount);
  CreatePythonGraphByPointsXY(pythonScriptPath, "Building time, seconds" /* xlabel */,
                              "Percent of routes built less than" /* ylabel */,
                              graphics, {"old mapsme", "new mapsme"} /* legends */);

  std::vector<std::string> labels;
  std::vector<std::vector<double>> errorsCount(2);
  for (auto const & item : benchmarkOldResults.GetErrorsDistribution())
  {
    labels.emplace_back(item.first);
    errorsCount[0].emplace_back(item.second);
  }

  for (auto const & item : benchmarkResults.GetErrorsDistribution())
    errorsCount[1].emplace_back(item.second);

  pythonScriptPath = base::JoinPath(dirForResults, kPythonBarError);
  CreatePythonBarByMap(pythonScriptPath, labels, errorsCount,
                       {"old mapsme", "new mapsme"} /* legends */,
                       "Type of errors" /* xlabel */, "Number of errors" /* ylabel */);

  std::vector<double> boostPercents;
  for (size_t i = 0; i < benchmarkOldResults.GetBuildTimes().size(); ++i)
  {
    auto const oldTime = benchmarkOldResults.GetBuildTimes()[i];
    auto const newTime = benchmarkResults.GetBuildTimes()[i];
    auto const diffPercent = (oldTime - newTime) / oldTime * 100.0;
    boostPercents.emplace_back(diffPercent);
  }

  pythonScriptPath = base::JoinPath(dirForResults, kPythonBarBoostPercentDistr);
  CreatePythonScriptForDistribution(pythonScriptPath, "Boost percent" /* title */, boostPercents);
}
}  // namespace routing_quality_tool
}  // namespace routing_quality
