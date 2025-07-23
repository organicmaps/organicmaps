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

bool ScenarioManager::RunScenario(ScenarioData && scenarioData, ScenarioCallback const & onStartFn,
                                  ScenarioCallback const & onFinishFn)
{
  std::lock_guard<std::mutex> lock(m_mutex);
  if (m_thread != nullptr)
  {
    if (m_isFinished)
      InterruptImpl();
    else
      return false;  // The only scenario can be executed currently.
  }

  std::swap(m_scenarioData, scenarioData);
  m_onStartHandler = onStartFn;
  m_onFinishHandler = onFinishFn;
  m_thread = std::make_unique<threads::SimpleThread>(&ScenarioManager::ThreadRoutine, this);
#ifdef DEBUG
  m_threadId = m_thread->get_id();
#endif
  return true;
}

bool ScenarioManager::IsRunning()
{
  std::lock_guard<std::mutex> lock(m_mutex);
  if (m_thread == nullptr)
    return false;

  if (m_isFinished)
  {
    InterruptImpl();
    return false;
  }
  return true;
}

void ScenarioManager::ThreadRoutine()
{
  std::string const scenarioName = m_scenarioData.m_name;
  if (m_onStartHandler != nullptr)
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
      CenterViewportAction * centerViewportAction = static_cast<CenterViewportAction *>(action.get());
      m_frontendRenderer->AddUserEvent(make_unique_dp<SetCenterEvent>(
          centerViewportAction->GetCenter(), centerViewportAction->GetZoomLevel(), true /* isAnim */,
          false /* trackVisibleViewport */, nullptr /* parallelAnimCreator */));
      break;
    }

    case ActionType::WaitForTime:
    {
      WaitForTimeAction * waitForTimeAction = static_cast<WaitForTimeAction *>(action.get());
      std::this_thread::sleep_for(waitForTimeAction->GetDuration());
      break;
    }

    default: LOG(LINFO, ("Unknown action in scenario"));
    }
  }

  ScenarioCallback handler = nullptr;
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_scenarioData.m_scenario.clear();
    m_isFinished = true;
    if (m_onFinishHandler != nullptr)
    {
      handler = m_onFinishHandler;
      m_onFinishHandler = nullptr;
    }
  }

  if (handler != nullptr)
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

}  //  namespace df
