#include "drape_frontend/scenario_manager.hpp"

#include "drape_frontend/user_event_stream.hpp"

#include <memory>

namespace df
{

ScenarioManager::ScenarioManager(FrontendRenderer * frontendRenderer)
  : m_frontendRenderer(frontendRenderer)
  , m_needInterrupt(false)
  , m_isFinished(false)
{
  ASSERT(m_frontendRenderer != nullptr, ());
}

ScenarioManager::~ScenarioManager()
{
  std::lock_guard<std::mutex> lock(m_mutex);
  if (m_thread != nullptr)
  {
    m_thread->join();
    m_thread.reset();
  }
}

void ScenarioManager::Interrupt()
{
  std::lock_guard<std::mutex> lock(m_mutex);
  InterruptImpl();
}

bool ScenarioManager::RunScenario(ScenarioData && scenarioData, ScenarioCallback const & onStartFn, ScenarioCallback const & onFinishFn)
{
  std::lock_guard<std::mutex> lock(m_mutex);
  // The only scenario can be executed currently.
  if (IsRunningImpl())
    return false;

  std::swap(m_scenarioData, scenarioData);
  m_onStartHandler = onStartFn;
  m_onFinishHandler = onFinishFn;
  m_thread = std::make_unique<threads::SimpleThread>(&ScenarioManager::ThreadRoutine, this);
#ifdef DEBUG
  m_threadId = m_thread->get_id();
#endif
  return true;
}

bool ScenarioManager::IsRunningImpl()
{
  if (m_thread == nullptr)
    return false;

  if (m_isFinished)
  {
    InterruptImpl();
    return false;
  }
  return true;
}

bool ScenarioManager::IsRunning()
{
  std::lock_guard<std::mutex> lock(m_mutex);
  return IsRunningImpl();
}

void ScenarioManager::ThreadRoutine()
{
  std::string const scenarioName = m_scenarioData.m_name;
  if (m_onStartHandler)
    m_onStartHandler(scenarioName);

  for (auto const & action : m_scenarioData.m_scenario)
  {
    // Interrupt scenario if it's necessary.
    {
      std::lock_guard<std::mutex> lock(m_mutex);
      if (m_needInterrupt)
      {
        m_needInterrupt = false;
        break;
      }
    }

    switch (action->GetType())
    {
    case ActionType::CenterViewport:
      {
        auto const * centerViewportAction = static_cast<CenterViewportAction const *>(action.get());
        m_frontendRenderer->AddUserEvent(make_unique_dp<SetCenterEvent>(centerViewportAction->GetCenter(),
                                                                        centerViewportAction->GetZoomLevel(),
                                                                        true /* isAnim */,
                                                                        false /* trackVisibleViewport */,
                                                                        nullptr /* parallelAnimCreator */));
        break;
      }

    case ActionType::WaitForTime:
      {
        auto const * waitForTimeAction = static_cast<WaitForTimeAction const *>(action.get());
        std::this_thread::sleep_for(waitForTimeAction->GetDuration());
        break;
      }

    default:
      LOG(LINFO, ("Unknown action in scenario"));
    }
  }

  ScenarioCallback handler;
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_scenarioData.m_scenario.clear();
    m_isFinished = true;
    if (m_onFinishHandler)
    {
      handler = std::move(m_onFinishHandler);
      m_onFinishHandler = {};
    }
  }

  if (handler)
    handler(scenarioName);
}

void ScenarioManager::InterruptImpl()
{
  if (m_thread == nullptr)
    return;

  ASSERT_NOT_EQUAL(m_threadId, std::this_thread::get_id(), ());

  if (m_isFinished)
  {
    m_thread->join();
    m_thread.reset();
    m_isFinished = false;
    m_needInterrupt = false;
  }
  else
  {
    m_needInterrupt = true;
  }
}

} //  namespace df
