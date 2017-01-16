#pragma once

#include "drape_frontend/frontend_renderer.hpp"

#include "geometry/point2d.hpp"

#include "base/stl_add.hpp"
#include "base/thread.hpp"

#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <vector>

#define SCENARIO_ENABLE

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
    CenterViewportAction(m2::PointD const & pt, int zoomLevel)
      : m_center(pt), m_zoomLevel(zoomLevel) {}

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

    WaitForTimeAction(Duration const & duration)
      : m_duration(duration) {}

    ActionType GetType() override { return ActionType::WaitForTime; }

    Duration const & GetDuration() const { return m_duration; }
  private:
    Duration m_duration;
  };

  using Scenario = std::vector<std::unique_ptr<Action>>;
  using OnFinishHandler = std::function<void()>;

  ScenarioManager(FrontendRenderer * frontendRenderer);
  ~ScenarioManager();

  bool RunScenario(Scenario && scenario, OnFinishHandler const & handler);
  void Interrupt();
  bool IsRunning();

private:
  void ThreadRoutine();
  void InterruptImpl();

  FrontendRenderer * m_frontendRenderer;

  std::mutex m_mutex;
  Scenario m_scenario;
  bool m_needInterrupt;
  bool m_isFinished;
  OnFinishHandler m_onFinishHandler;
#ifdef DEBUG
  std::thread::id m_threadId;
#endif
  std::unique_ptr<threads::SimpleThread> m_thread;
};

} //  namespace df
