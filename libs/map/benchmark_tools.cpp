#include "map/benchmark_tools.hpp"
#include "map/framework.hpp"

#include "drape_frontend/drape_measurer.hpp"
#include "drape_frontend/scenario_manager.hpp"

#include "storage/country_info_getter.hpp"

#include "platform/downloader_defines.hpp"
#include "platform/http_client.hpp"
#include "platform/platform.hpp"

#include "coding/reader.hpp"

#include "geometry/mercator.hpp"

#include "base/file_name_utils.hpp"

#include <glaze/json.hpp>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace benchmark_json
{
struct BenchmarkCenterJson
{
  double lat = 0.0;
  double lon = 0.0;
};

struct BenchmarkStepJson
{
  std::string actionType;
  int64_t time = 0;
  std::optional<BenchmarkCenterJson> center;
  int64_t zoomLevel = -1;
};

struct BenchmarkScenarioJson
{
  std::string name;
  std::vector<BenchmarkStepJson> steps;
};

struct BenchmarkDataJson
{
  std::vector<BenchmarkScenarioJson> scenarios;
};
}  // namespace benchmark_json

namespace
{
struct BenchmarkHandle
{
  std::vector<df::ScenarioManager::ScenarioData> m_scenariosToRun;
  size_t m_currentScenario = 0;
  std::vector<storage::CountryId> m_regionsToDownload;
  size_t m_regionsToDownloadCounter = 0;

#ifdef DRAPE_MEASURER_BENCHMARK
  std::vector<std::pair<std::string, df::DrapeMeasurer::DrapeStatistic>> m_drapeStatistic;
#endif
};

void RunScenario(Framework * framework, std::shared_ptr<BenchmarkHandle> handle)
{
  if (handle->m_currentScenario >= handle->m_scenariosToRun.size())
  {
#ifdef DRAPE_MEASURER_BENCHMARK
    for (auto const & it : handle->m_drapeStatistic)
    {
      LOG(LINFO, ("\n ***** Report for scenario", it.first, "*****\n", it.second.ToString(),
                  "\n ***** Report for scenario", it.first, "*****\n"));
    }
#endif
    return;
  }

  auto & scenarioData = handle->m_scenariosToRun[handle->m_currentScenario];

  framework->GetDrapeEngine()->RunScenario(std::move(scenarioData),
                                           [handle](std::string const & name)
  {
#ifdef DRAPE_MEASURER_BENCHMARK
    df::DrapeMeasurer::Instance().Start();
#endif
  }, [framework, handle](std::string const & name)
  {
#ifdef DRAPE_MEASURER_BENCHMARK
    df::DrapeMeasurer::Instance().Stop();
    auto const drapeStatistic = df::DrapeMeasurer::Instance().GetDrapeStatistic();
    handle->m_drapeStatistic.push_back(make_pair(name, drapeStatistic));
#endif
    GetPlatform().RunTask(Platform::Thread::Gui, [framework, handle]()
    {
      handle->m_currentScenario++;
      RunScenario(framework, handle);
    });
  });
}
}  // namespace

namespace benchmark
{
void RunGraphicsBenchmark(Framework * framework)
{
#ifdef SCENARIO_ENABLE
  using namespace df;

  // Load scenario from file.
  auto const fn = base::JoinPath(GetPlatform().SettingsDir(), "graphics_benchmark.json");
  if (!GetPlatform().IsFileExistsByFullPath(fn))
    return;

  std::string benchmarkData;
  try
  {
    ReaderPtr<Reader>(GetPlatform().GetReader(fn)).ReadAsString(benchmarkData);
  }
  catch (RootException const & e)
  {
    LOG(LCRITICAL, ("Error reading benchmark file: ", e.what()));
    return;
  }

  std::shared_ptr<BenchmarkHandle> handle = std::make_shared<BenchmarkHandle>();

  std::vector<m2::PointD> points;
  benchmark_json::BenchmarkDataJson benchmarkJson;
  {
    glz::opts constexpr opts{.error_on_unknown_keys = false, .error_on_missing_keys = false};
    if (auto const error = glz::read<opts>(benchmarkJson, benchmarkData); error)
      return;
  }

  // Parse scenarios.
  handle->m_scenariosToRun.reserve(benchmarkJson.scenarios.size());
  for (auto const & scenarioJson : benchmarkJson.scenarios)
  {
    ScenarioManager::ScenarioData scenarioData;
    scenarioData.m_name = scenarioJson.name;
    scenarioData.m_scenario.reserve(scenarioJson.steps.size());

    for (auto const & stepJson : scenarioJson.steps)
    {
      if (stepJson.actionType == "waitForTime")
      {
        scenarioData.m_scenario.push_back(std::unique_ptr<ScenarioManager::Action>(
            new ScenarioManager::WaitForTimeAction(std::chrono::seconds(stepJson.time))));
      }
      else if (stepJson.actionType == "centerViewport")
      {
        if (!stepJson.center)
          return;

        m2::PointD const pt = mercator::FromLatLon(stepJson.center->lat, stepJson.center->lon);
        points.push_back(pt);
        scenarioData.m_scenario.push_back(std::unique_ptr<ScenarioManager::Action>(
            new ScenarioManager::CenterViewportAction(pt, static_cast<int>(stepJson.zoomLevel))));
      }
    }

    handle->m_scenariosToRun.push_back(std::move(scenarioData));
  }

  if (handle->m_scenariosToRun.empty())
    return;

  // Find out regions to download.
  std::set<storage::CountryId> regions;
  for (m2::PointD const & pt : points)
    regions.insert(framework->GetCountryInfoGetter().GetRegionCountryId(pt));

  for (auto const & countryId : regions)
  {
    storage::NodeStatuses statuses;
    framework->GetStorage().GetNodeStatuses(countryId, statuses);
    if (statuses.m_status != storage::NodeStatus::OnDisk)
      handle->m_regionsToDownload.push_back(countryId);
  }

  // Download regions and run scenarios after downloading.
  if (!handle->m_regionsToDownload.empty())
  {
    framework->GetStorage().Subscribe([framework, handle](storage::CountryId const & countryId)
    {
      if (base::IsExist(handle->m_regionsToDownload, countryId))
      {
        handle->m_regionsToDownloadCounter++;
        if (handle->m_regionsToDownloadCounter == handle->m_regionsToDownload.size())
        {
          handle->m_regionsToDownload.clear();
          RunScenario(framework, handle);
        }
      }
    }, [](storage::CountryId const &, downloader::Progress const &) {});

    for (auto const & countryId : handle->m_regionsToDownload)
      framework->GetStorage().DownloadNode(countryId);
    return;
  }

  // Run scenarios without downloading.
  RunScenario(framework, handle);
#endif
}
}  // namespace benchmark
