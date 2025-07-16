#pragma once

#include "drape_frontend/frontend_renderer.hpp"

#include "drape/drape_diagnostics.hpp"

#include "geometry/point2d.hpp"

#include "base/stl_helpers.hpp"
#include "base/thread.hpp"

#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <vector>

namespace df
{

class ScenarioManager
{
public:
  enum class ActionType
  {
    CenterViewport,
    WaitForTime
  };

  class Action
  {
  public:
    virtual ~Action() {}
    virtual ActionType GetType() = 0;
  };

  class CenterViewportAction : public Action
  {
  public:
    CenterViewportAction(m2::PointD const & pt, int zoomLevel) : m_center(pt), m_zoomLevel(zoomLevel) {}

    ActionType GetType() override { return ActionType::CenterViewport; }

    m2::PointD const & GetCenter() const { return m_center; }
    int GetZoomLevel() const { return m_zoomLevel; }

  private:
    m2::PointD const m_center;
    int const m_zoomLevel;
  };

  class WaitForTimeAction : public Action
  {
  public:
    using Duration = std::chrono::steady_clock::duration;

    WaitForTimeAction(Duration const & duration) : m_duration(duration) {}

    ActionType GetType() override { return ActionType::WaitForTime; }

    Duration const & GetDuration() const { return m_duration; }

  private:
    Duration m_duration;
  };

  using Scenario = std::vector<std::unique_ptr<Action>>;
  using ScenarioCallback = std::function<void(std::string const & name)>;

  struct ScenarioData
  {
    std::string m_name;
    Scenario m_scenario;
  };

  ScenarioManager(FrontendRenderer * frontendRenderer);
  ~ScenarioManager();

  bool RunScenario(ScenarioData && scenarioData, ScenarioCallback const & startHandler,
                   ScenarioCallback const & finishHandler);
  void Interrupt();
  bool IsRunning();

private:
  void ThreadRoutine();
  void InterruptImpl();

  FrontendRenderer * m_frontendRenderer;

  std::mutex m_mutex;
  ScenarioData m_scenarioData;
  bool m_needInterrupt;
  bool m_isFinished;
  ScenarioCallback m_onStartHandler;
  ScenarioCallback m_onFinishHandler;
#ifdef DEBUG
  std::thread::id m_threadId;
#endif
  std::unique_ptr<threads::SimpleThread> m_thread;
};

}  //  namespace df
