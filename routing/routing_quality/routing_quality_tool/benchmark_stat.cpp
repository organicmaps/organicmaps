#include "routing/routing_quality/routing_quality_tool/benchmark_stat.hpp"

#include "routing/routing_quality/routing_quality_tool/benchmark_results.hpp"
#include "routing/routing_quality/routing_quality_tool/utils.hpp"

#include "routing/routing_callbacks.hpp"

#include "geometry/point2d.hpp"

#include "base/assert.hpp"
#include "base/file_name_utils.hpp"
#include "base/logging.hpp"

#include <algorithm>

namespace
{
using namespace routing;
using namespace routes_builder;

bool IsErrorCode(RouterResultCode code)
{
  return code != RouterResultCode::NoError;
}

void CheckConsistency(RoutesBuilder::Result const & oldRes, RoutesBuilder::Result const & newRes)
{
  auto const start = mercator::ToLatLon(oldRes.m_params.m_checkpoints.GetStart());
  auto const finish = mercator::ToLatLon(oldRes.m_params.m_checkpoints.GetFinish());
  CHECK(!oldRes.m_routes.empty(), ());
  CHECK(!newRes.m_routes.empty(), ());
  auto const & oldRoute = oldRes.m_routes.back();
  auto const & newRoute = newRes.m_routes.back();

  bool const sameETA = base::AlmostEqualAbs(oldRoute.m_eta, newRoute.m_eta, 1.0);
  bool const sameDistance = base::AlmostEqualAbs(oldRoute.m_distance, newRoute.m_distance, 1.0);
  if (!sameETA || !sameDistance)
  {
    LOG(LINFO, ("old ETA:", oldRoute.m_eta, "old distance:", oldRoute.m_distance, "new ETA:",
                newRoute.m_eta, "new distance:", newRoute.m_distance, start, finish));
  }
}
}  // namespace

namespace routing_quality::routing_quality_tool
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
// Shows percent of boost distribution. Where boost percent equals:
// 100.0 * (old_time - new_time) / old_time
static std::string const kPythonBarBoostPercentDistr = "show_boost_distr.py";
// Shows distribution of non-zero difference of ETA between old and new mapsme version.
static std::string const kPythonEtaDiff = "eta_diff.py";
// The same as above but in percents.
static std::string const kPythonEtaDiffPercent = "eta_diff_percent.py";

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
  for (auto const & [errorName, errorCount] : benchmarkResults.GetErrorsDistribution())
  {
    labels.emplace_back(errorName);
    heights.emplace_back(errorCount);
  }

  CreatePythonBarByMap(pythonScriptPath, labels, {heights}, {"mapsme"} /* legends */,
                       "Type of errors" /* xlabel */, "Number of errors" /* ylabel */);
}

void RunBenchmarkComparison(
    std::vector<std::pair<RoutesBuilder::Result, std::string>> && mapsmeResults,
    std::vector<std::pair<RoutesBuilder::Result, std::string>> && mapsmeOldResults,
    std::string const & dirForResults)
{
  BenchmarkResults benchmarkResults;
  BenchmarkResults benchmarkOldResults;
  std::vector<double> etaDiffsPercent;
  std::vector<double> etaDiffs;

  for (size_t i = 0; i < mapsmeResults.size(); ++i)
  {
    auto const & mapsmeResult = mapsmeResults[i].first;

    std::pair<RoutesBuilder::Result, std::string> mapsmeOldResultPair;
    if (!FindAnotherResponse(mapsmeResult, mapsmeOldResults, mapsmeOldResultPair))
    {
      LOG(LDEBUG, ("Can not find pair for:", i));
      continue;
    }

    auto const & mapsmeOldResult = mapsmeOldResultPair.first;

    benchmarkResults.PushError(mapsmeResult.m_code);
    benchmarkOldResults.PushError(mapsmeOldResult.m_code);

    if (IsErrorCode(mapsmeResult.m_code) && !IsErrorCode(mapsmeOldResult.m_code))
    {
      auto const start = mercator::ToLatLon(mapsmeResult.m_params.m_checkpoints.GetStart());
      auto const finish = mercator::ToLatLon(mapsmeResult.m_params.m_checkpoints.GetFinish());
      LOG_FORCE(LWARNING, ("New version returns error code:", mapsmeResult.m_code,
                           "but old returns NoError for:", start, finish));
    }

    if (IsErrorCode(mapsmeResult.m_code) || IsErrorCode(mapsmeOldResult.m_code))
      continue;

    CheckConsistency(mapsmeOldResult, mapsmeResult);

    auto const etaDiff =
        (mapsmeOldResult.m_routes.back().m_eta - mapsmeResult.m_routes.back().m_eta);
    auto const etaDiffPercent = etaDiff / mapsmeOldResult.m_routes.back().m_eta * 100.0;

    etaDiffs.emplace_back(etaDiff);
    etaDiffsPercent.emplace_back(etaDiffPercent);

    benchmarkResults.PushBuildTime(mapsmeResult.m_buildTimeSeconds);
    benchmarkOldResults.PushBuildTime(mapsmeOldResult.m_buildTimeSeconds);
  }

  LOG(LINFO, ("Comparing", benchmarkResults.GetBuildTimes().size(), "routes."));

  LOG(LINFO, ("Average route time building. "
              "Old version:", benchmarkOldResults.GetAverageBuildTime(),
              "New version:", benchmarkResults.GetAverageBuildTime()));

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
                              "Percent of routes built less than" /* ylabel */, graphics,
                              {"old mapsme", "new mapsme"} /* legends */);

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
                       {"old mapsme", "new mapsme"} /* legends */, "Type of errors" /* xlabel */,
                       "Number of errors" /* ylabel */);

  std::vector<double> boostPercents;
  for (size_t i = 0; i < benchmarkOldResults.GetBuildTimes().size(); ++i)
  {
    auto const oldTime = benchmarkOldResults.GetBuildTimes()[i];
    auto const newTime = benchmarkResults.GetBuildTimes()[i];
    if (base::AlmostEqualAbs(oldTime, newTime, 1e-2))
      continue;

    auto const diffPercent = (oldTime - newTime) / oldTime * 100.0;
    boostPercents.emplace_back(diffPercent);
  }

  pythonScriptPath = base::JoinPath(dirForResults, kPythonBarBoostPercentDistr);
  CreatePythonScriptForDistribution(pythonScriptPath, "Boost percent" /* title */, boostPercents);

  pythonScriptPath = base::JoinPath(dirForResults, kPythonEtaDiff);
  CreatePythonScriptForDistribution(pythonScriptPath, "ETA diff distribution" /* title */,
                                    etaDiffs);

  pythonScriptPath = base::JoinPath(dirForResults, kPythonEtaDiffPercent);
  CreatePythonScriptForDistribution(pythonScriptPath, "ETA diff percent distribution" /* title */,
                                    etaDiffsPercent);
}
}  // namespace routing_quality::routing_quality_tool
