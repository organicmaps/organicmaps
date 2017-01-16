#include "map/benchmark_tools.hpp"
#include "map/framework.hpp"

#include "drape_frontend/scenario_manager.hpp"

#include "platform/http_client.hpp"
#include "platform/platform.hpp"

#include "3party/jansson/myjansson.hpp"

#include <algorithm>
#include <atomic>
#include <set>
#include <string>
#include <vector>

namespace
{
struct ScenarioData
{
  std::string m_name;
  df::ScenarioManager::Scenario m_scenario;
};

struct BenchmarkHandle
{
  std::vector<ScenarioData> m_scenariosRoRun;
  size_t m_currentScenario = 0;
  std::vector<storage::TCountryId> m_regionsToDownload;
  size_t m_regionsToDownloadCounter = 0;
};

void RunScenario(Framework * framework, std::shared_ptr<BenchmarkHandle> handle)
{
  if (handle->m_currentScenario >= handle->m_scenariosRoRun.size())
    return;

  auto & scenarioData = handle->m_scenariosRoRun[handle->m_currentScenario];
  framework->GetDrapeEngine()->RunScenario(std::move(scenarioData.m_scenario), [framework, handle]
  {
    GetPlatform().RunOnGuiThread([framework, handle]()
    {
      handle->m_currentScenario++;
      RunScenario(framework, handle);
    });
  });
}
} //  namespace

namespace benchmark
{
void RunGraphicsBenchmark(Framework * framework)
{
#ifdef SCENARIO_ENABLE
  using namespace df;

  // Request scenarios from the server.
  platform::HttpClient request("http://osmz.ru/mwm/graphics_benchmark.json");
  if (!request.RunHttpRequest())
    return;

  std::shared_ptr<BenchmarkHandle> handle = std::make_shared<BenchmarkHandle>();

  // Parse scenarios.
  std::vector<m2::PointD> points;
  string const & result = request.ServerResponse();
  try
  {
    my::Json root(result.c_str());
    json_t * scenariosNode = json_object_get(root.get(), "scenarios");
    if (scenariosNode == nullptr || !json_is_array(scenariosNode))
      return;
    size_t const sz = json_array_size(scenariosNode);
    handle->m_scenariosRoRun.resize(sz);
    for (size_t i = 0; i < sz; ++i)
    {
      auto scenarioElem = json_array_get(scenariosNode, i);
      if (scenarioElem == nullptr)
        return;
      my::FromJSONObject(scenarioElem, "name", handle->m_scenariosRoRun[i].m_name);
      json_t * stepsNode = json_object_get(scenarioElem, "steps");
      if (stepsNode != nullptr && json_is_array(stepsNode))
      {
        size_t const stepsCount = json_array_size(stepsNode);
        auto & scenario = handle->m_scenariosRoRun[i].m_scenario;
        scenario.reserve(stepsCount);
        for (size_t j = 0; j < stepsCount; ++j)
        {
          auto stepElem = json_array_get(stepsNode, j);
          if (stepElem == nullptr)
            return;
          string actionType;
          my::FromJSONObject(stepElem, "actionType", actionType);
          if (actionType == "waitForTime")
          {
            json_int_t timeInSeconds = 0;
            my::FromJSONObject(stepElem, "time", timeInSeconds);
            scenario.push_back(std::unique_ptr<ScenarioManager::Action>(
                                 new ScenarioManager::WaitForTimeAction(seconds(timeInSeconds))));
          }
          else if (actionType == "centerViewport")
          {
            json_t * centerNode = json_object_get(stepElem, "center");
            if (centerNode == nullptr)
              return;
            double x = 0.0, y = 0.0;
            my::FromJSONObject(centerNode, "x", x);
            my::FromJSONObject(centerNode, "y", y);
            json_int_t zoomLevel = -1;
            my::FromJSONObject(stepElem, "zoomLevel", zoomLevel);
            m2::PointD const pt(x, y);
            points.push_back(pt);
            scenario.push_back(std::unique_ptr<ScenarioManager::Action>(
                                 new ScenarioManager::CenterViewportAction(pt, static_cast<int>(zoomLevel))));
          }
        }
      }
    }
  }
  catch (my::Json::Exception const & e)
  {
    return;
  }
  if (handle->m_scenariosRoRun.empty())
    return;

  // Find out regions to download.
  std::set<storage::TCountryId> regions;
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
    framework->GetStorage().Subscribe([framework, handle](storage::TCountryId const & countryId)
    {
      if (std::find(handle->m_regionsToDownload.begin(),
                    handle->m_regionsToDownload.end(), countryId) != handle->m_regionsToDownload.end())
      {
        handle->m_regionsToDownloadCounter++;
        if (handle->m_regionsToDownloadCounter == handle->m_regionsToDownload.size())
        {
          handle->m_regionsToDownload.clear();
          RunScenario(framework, handle);
        }
      }
    }, [](storage::TCountryId const &, storage::MapFilesDownloader::TProgress const &){});

    for (auto const & countryId : handle->m_regionsToDownload)
      framework->GetStorage().DownloadNode(countryId);
    return;
  }

  // Run scenartios without downloading.
  RunScenario(framework, handle);
#endif
}
} //  namespace benchmark
