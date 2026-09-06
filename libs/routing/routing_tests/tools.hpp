#pragma once

#include "platform/platform_tests_support/async_gui_thread.hpp"

#include "routing/routing_session.hpp"

#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>

namespace routing
{
/// \brief One-shot signal a test thread waits on until it fires or the deadline passes.
class TimedSignal
{
public:
  void Signal()
  {
    std::lock_guard<std::mutex> guard(m_mutex);
    m_flag = true;
    m_cv.notify_one();
  }

  bool WaitUntil(std::chrono::steady_clock::time_point const & time)
  {
    std::unique_lock<std::mutex> lock(m_mutex);
    m_cv.wait_until(lock, time, [this, &time] { return m_flag || std::chrono::steady_clock::now() > time; });
    return m_flag;
  }

private:
  std::mutex m_mutex;
  std::condition_variable m_cv;
  bool m_flag = false;
};

class AsyncGuiThreadTest
{
  platform::tests_support::AsyncGuiThread m_asyncGuiThread;
};

class AsyncGuiThreadTestWithRoutingSession : public AsyncGuiThreadTest
{
public:
  void InitRoutingSession();

  std::unique_ptr<RoutingSession> m_session;
  size_t m_onNewTurnCallbackCounter = 0;
};

void RouteSegmentsFrom(std::vector<Segment> const & segments, std::vector<m2::PointD> const & path,
                       std::vector<turns::TurnItem> const & turns,
                       std::vector<RouteSegment::RoadNameInfo> const & names,
                       std::vector<RouteSegment> & routeSegments);
}  // namespace routing
