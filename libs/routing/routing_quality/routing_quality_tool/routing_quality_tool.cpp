#include "routing/routing_quality/routing_quality_tool/utils.hpp"
#include "routing/routing_quality/routing_quality_tool/benchmark_stat.hpp"
#include "routing/routing_quality/routing_quality_tool/error_type_counter.hpp"

#include "routing/routing_quality/api/api.hpp"

#include "routing/routing_quality/waypoints.hpp"

#include "routing/routes_builder/routes_builder.hpp"

#include "platform/platform.hpp"

#include "base/exception.hpp"
#include "base/file_name_utils.hpp"
#include "base/logging.hpp"

#include <algorithm>
#include <exception>
#include <limits>
#include <utility>
#include <vector>

#include <gflags/gflags.h>

DEFINE_string(mapsme_old_results, "", "Path to directory with previous mapsme router results.");
DEFINE_string(mapsme_results, "", "Path to directory with mapsme router results.");
DEFINE_string(api_results, "", "Path to directory with api router results.");

DEFINE_string(save_results, "", "The directory where results of tool will be saved.");

DEFINE_double(kml_percent, 0.0, "The percent of routes for which kml file will be generated."
                                "With kml files you can make screenshots with desktop app of OMaps");

DEFINE_bool(benchmark_stat, false, "Dump statistics about route time building.");

namespace
{
// Shows distribution of simularity in comparison mode.
static std::string const kPythonDistribution = "show_distribution.py";
static std::string const kPythonBarDistributionError = "show_errors_distribution.py";

double constexpr kBadETADiffPercent = std::numeric_limits<double>::max();

void PrintHelpAndExit()
{
  std::stringstream usage;
  usage << std::boolalpha;
  usage << R"(Description:
This tool takes two paths to directory with routes, that were dumped by routes_builder_tool and calculate some metrics. Here possible options of execution:

    --mapsme_results and --api_results are required to compare mapsme and api.
    or
    --mapsme_results and --mapsme_old_results are required to compare mapsme and old mapsme version.
    or
    --mapsme_results and --benchmark_stat are required to calculate benchmark statistic of mapsme.
    or
    --mapsme_results and --mapsme_old_results and --benchmark_stat are required to calculate different statistics of comparison mapsme and old mapsme version.

    --save_results is required for saving results in this directory.
    --kml_percent may be used in non-benchamrk mode for dumping kmls and visual comparison different routes.
)";

  auto const addStringInfo = [&usage](auto const & arg, auto const & argValue) {
    using T = decltype(argValue);
    usage << "\n\t" << arg << " is";
    if (argValue == T{})
      usage << " not set.";
    else
      usage << " set to: " << argValue;
  };

  addStringInfo("--mapsme_results", FLAGS_mapsme_results);
  addStringInfo("--api_results", FLAGS_api_results);
  addStringInfo("--mapsme_old_results", FLAGS_mapsme_old_results);
  addStringInfo("--benchmark_stat", FLAGS_benchmark_stat);
  addStringInfo("--save_results", FLAGS_save_results);
  addStringInfo("--kml_percent", FLAGS_kml_percent);

  usage << "\n\nType --help for usage.\n";
  std::cout << usage.str();
  exit(0);
}

bool HasHelpFlags(int argc, char ** argv)
{
  for (int i = 0; i < argc; ++i)
  {
    auto const value = std::string(argv[i]);
    if (value == "--help" || value == "-h")
      return true;
  }
  return false;
}
} // namespace

using namespace routing;
using namespace routes_builder;
using namespace routing_quality;
using namespace routing_quality_tool;

void PrintResults(std::vector<Result> && results, RoutesSaver & routesSaver)
{
  double sumSimilarity = 0.0;
  double sumETADiffPercent = 0.0;
  double goodETANumber = 0.0;
  for (auto const & result : results)
  {
    sumSimilarity += result.m_similarity;
    if (result.m_etaDiffPercent != kBadETADiffPercent)
    {
      sumETADiffPercent += result.m_etaDiffPercent;
      goodETANumber += 1;
    }
  }

  LOG(LINFO, ("Matched routes:", results.size()));
  LOG(LINFO, ("Average similarity:", sumSimilarity / results.size()));
  LOG(LINFO, ("Average eta difference by fullmathed routes:", sumETADiffPercent / goodETANumber, "%"));

  auto const pythonScriptPath = base::JoinPath(FLAGS_save_results, kPythonDistribution);

  std::vector<double> values;
  values.reserve(results.size());
  for (auto const & result : results)
    values.emplace_back(result.m_similarity);

  CreatePythonScriptForDistribution(pythonScriptPath, "Simularity distribution", values);

  SimilarityCounter similarityCounter(routesSaver);

  std::sort(results.begin(), results.end());
  for (auto const & result : results)
    similarityCounter.Push(result);

  similarityCounter.CreateKmlFiles(FLAGS_kml_percent, results);
}

bool IsMapsmeVsApi()
{
  return !FLAGS_mapsme_results.empty() && !FLAGS_api_results.empty();
}

bool IsMapsmeVsMapsme()
{
  return !FLAGS_mapsme_results.empty() && !FLAGS_mapsme_old_results.empty() && !FLAGS_benchmark_stat;
}

bool IsMapsmeBenchmarkStat()
{
  return !FLAGS_mapsme_results.empty() && FLAGS_benchmark_stat && FLAGS_mapsme_old_results.empty();
}

bool IsMapsmeVsMapsmeBenchmarkStat()
{
  return !FLAGS_mapsme_results.empty() && FLAGS_benchmark_stat && !FLAGS_mapsme_old_results.empty();
}

void CheckDirExistence(std::string const & dir)
{
  CHECK(Platform::IsDirectory(dir), ("Can not find directory:", dir));
}

template <typename AnotherResult>
void RunComparison(std::vector<std::pair<RoutesBuilder::Result, std::string>> && mapsmeResults,
                   std::vector<std::pair<AnotherResult, std::string>> && anotherResults)
{
  ErrorTypeCounter mapsmeErrorCounter;
  ErrorTypeCounter anotherErrorCounter;
  ComparisonType type = IsMapsmeVsApi() ? ComparisonType::MapsmeVsApi
                                        : ComparisonType::MapsmeVsMapsme;
  RoutesSaver routesSaver(FLAGS_save_results, type);
  std::vector<Result> results;
  size_t apiErrors = 0;

  for (size_t i = 0; i < mapsmeResults.size(); ++i)
  {
    auto const & mapsmeResult = mapsmeResults[i].first;
    auto const & mapsmeFile = mapsmeResults[i].second;

    std::pair<AnotherResult, std::string> anotherResultPair;
    if (!FindAnotherResponse(mapsmeResult, anotherResults, anotherResultPair))
    {
      LOG(LDEBUG, ("Can not find pair for:", i));
      continue;
    }

    auto const & anotherResult = anotherResultPair.first;
    auto const & anotherFile = anotherResultPair.second;

    auto const & startLatLon = mercator::ToLatLon(anotherResult.GetStartPoint());
    auto const & finishLatLon = mercator::ToLatLon(anotherResult.GetFinishPoint());

    mapsmeErrorCounter.PushError(mapsmeResult.m_code);
    anotherErrorCounter.PushError(anotherResult.m_code);

    if (!mapsmeResult.IsCodeOK() && anotherResult.IsCodeOK())
    {
      routesSaver.PushError(mapsmeResult.m_code, startLatLon, finishLatLon);
      continue;
    }

    if (anotherResult.GetRoutes().empty() || !anotherResult.IsCodeOK())
    {
      routesSaver.PushRivalError(startLatLon, finishLatLon);
      ++apiErrors;
      continue;
    }

    auto maxSimilarity = std::numeric_limits<Similarity>::min();
    double etaDiff = kBadETADiffPercent;
    auto const & mapsmeRoute = mapsmeResult.GetRoutes().back();
    for (auto const & route : anotherResult.GetRoutes())
    {
      auto const similarity =
          metrics::CompareByNumberOfMatchedWaypoints(mapsmeRoute.m_followedPolyline,
                                                     route.GetWaypoints());

      if (maxSimilarity < similarity)
      {
        maxSimilarity = similarity;
        if (maxSimilarity == 1.0)
        {
          etaDiff = 100.0 * std::abs(route.GetETA() - mapsmeRoute.GetETA()) /
                    std::max(route.GetETA(), mapsmeRoute.GetETA());
        }
      }
    }

    results.emplace_back(mapsmeFile, anotherFile, startLatLon, finishLatLon, maxSimilarity, etaDiff);
  }

  std::string const anotherSourceName = IsMapsmeVsMapsme() ? "old mapsme," : "api,";
  LOG(LINFO, (apiErrors, "routes can not build via", anotherSourceName, "but mapsme do built them."));

  PrintResults(std::move(results), routesSaver);

  std::vector<std::string> errorLabels;
  std::vector<std::vector<double>> errorsCount;
  FillLabelsAndErrorTypeDistribution(errorLabels, errorsCount, mapsmeErrorCounter,
                                     anotherErrorCounter);

  auto const pythonScriptPath = base::JoinPath(FLAGS_save_results, kPythonBarDistributionError);
  CreatePythonBarByMap(pythonScriptPath, errorLabels, errorsCount,
                       {"mapsme", IsMapsmeVsMapsme() ? "old mapsme" : "api"} /* legends */,
                       "Type of errors" /* xlabel */, "Number of errors" /* ylabel */);
}

void CheckArgs()
{
  bool const modeIsChosen = IsMapsmeVsApi() || IsMapsmeVsMapsme() || IsMapsmeBenchmarkStat() ||
                            IsMapsmeVsMapsmeBenchmarkStat();
  if (!modeIsChosen)
    PrintHelpAndExit();

  CHECK(!FLAGS_save_results.empty(),
        ("\n\n\t--save_results is required. Tool will save results there.",
         "\n\nType --help for usage."));
}

int Main(int argc, char ** argv)
{
  if (HasHelpFlags(argc, argv))
    PrintHelpAndExit();

  gflags::ParseCommandLineNonHelpFlags(&argc, &argv, true);

  CheckArgs();

  if (Platform::IsFileExistsByFullPath(FLAGS_save_results))
    CheckDirExistence(FLAGS_save_results);
  else
    CHECK_EQUAL(Platform::MkDir(FLAGS_save_results), Platform::EError::ERR_OK,());

  CheckDirExistence(FLAGS_mapsme_results);

  CHECK(0.0 <= FLAGS_kml_percent && FLAGS_kml_percent <= 100.0,
        ("--kml_percent should be in interval: [0.0, 100.0]."));

  LOG(LINFO, ("Start loading mapsme results."));
  auto mapsmeResults = LoadResults<RoutesBuilder::Result>(FLAGS_mapsme_results);
  LOG(LINFO, ("Receive:", mapsmeResults.size(), "routes from --mapsme_results."));

  if (IsMapsmeVsApi())
  {
    LOG(LINFO, ("Start loading api results."));
    auto apiResults = LoadResults<api::Response>(FLAGS_api_results);
    LOG(LINFO, ("Receive:", apiResults.size(), "routes from --api_results."));
    RunComparison(std::move(mapsmeResults), std::move(apiResults));
  }
  else if (IsMapsmeVsMapsmeBenchmarkStat())
  {
    LOG(LINFO, ("Benchmark different mapsme versions. Start loading old mapsme results."));
    auto oldMapsmeResults = LoadResults<RoutesBuilder::Result>(FLAGS_mapsme_old_results);
    LOG(LINFO, ("Receive:", oldMapsmeResults.size(), "routes from --mapsme_old_results."));
    RunBenchmarkComparison(std::move(mapsmeResults), std::move(oldMapsmeResults),
                           FLAGS_save_results);
  }
  else if (IsMapsmeVsMapsme())
  {
    LOG(LINFO, ("Start loading another mapsme results."));
    auto oldMapsmeResults = LoadResults<RoutesBuilder::Result>(FLAGS_mapsme_old_results);
    LOG(LINFO, ("Receive:", oldMapsmeResults.size(), "routes from --mapsme_old_results."));
    RunComparison(std::move(mapsmeResults), std::move(oldMapsmeResults));
  }
  else if (IsMapsmeBenchmarkStat())
  {
    LOG(LINFO, ("Running in benchmark stat mode."));
    RunBenchmarkStat(mapsmeResults, FLAGS_save_results);
  }
  else
  {
    UNREACHABLE();
  }

  return 0;
}

int main(int argc, char ** argv)
{
  try
  {
    return Main(argc, argv);
  }
  catch (RootException const & e)
  {
    LOG(LCRITICAL, ("Core exception:", e.Msg()));
  }
  catch (std::exception const & e)
  {
    LOG(LCRITICAL, ("Std exception:", e.what()));
  }
  catch (...)
  {
    LOG(LCRITICAL, ("Unknown exception."));
  }

  LOG(LINFO, ("Done."));

  return 0;
}
