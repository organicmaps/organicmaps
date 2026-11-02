#pragma once

#include "testing/testing.hpp"

#include "routing/routing_session.hpp"

#include "platform/platform.hpp"
#include "platform/platform_tests_support/async_gui_thread.hpp"

#include <chrono>
#include <future>
#include <memory>
#include <type_traits>
#include <utility>

namespace routing
{
template <typename Fn>
auto RunOnGui(Fn && fn)
{
  // Return GUI-thread assertion failures to the test thread, keeping the task alive on timeout.
  auto task = std::make_shared<std::packaged_task<std::invoke_result_t<Fn>()>>(std::forward<Fn>(fn));
  auto done = task->get_future();
  GetPlatform().RunTask(Platform::Thread::Gui, [task] { (*task)(); });
  TEST(done.wait_for(std::chrono::seconds(30)) == std::future_status::ready, ("GUI task did not finish."));
  return done.get();
}

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
