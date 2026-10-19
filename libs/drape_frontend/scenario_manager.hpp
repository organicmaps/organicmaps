#pragma once

#include "drape_frontend/frontend_renderer.hpp"

#include "drape/drape_diagnostics.hpp"

#include "geometry/point2d.hpp"

#include "base/stl_helpers.hpp"
#include "base/thread.hpp"

#include <chrono>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <vector>

namespace df
{

#ifdef SCENARIO_ENABLE
struct ScenarioViewportRequest
{
  ScenarioViewportRequest(m2::PointD const & center, int zoom) : m_center(center), m_zoom(zoom) {}

  void Complete(bool ready);
  std::optional<bool> WaitFor(std::chrono::milliseconds duration);

  m2::PointD const m_center;
  int const m_zoom;

private:
  std::mutex m_mutex;
  std::condition_variable m_condition;
  std::optional<bool> m_result;
};
#endif

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
    CenterViewportAction(m2::PointD const & pt, int zoomLevel, bool animated = true, bool waitForReady = false)
      : m_center(pt)
      , m_zoomLevel(zoomLevel)
      , m_animated(animated)
      , m_waitForReady(waitForReady)
    {}

    ActionType GetType() override { return ActionType::CenterViewport; }

    m2::PointD const & GetCenter() const { return m_center; }
    int GetZoomLevel() const { return m_zoomLevel; }
    bool IsAnimated() const { return m_animated; }
    bool WaitForReady() const { return m_waitForReady; }

  private:
    m2::PointD const m_center;
    int const m_zoomLevel;
    bool const m_animated;
    bool const m_waitForReady;
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
  std::condition_variable m_condition;
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
