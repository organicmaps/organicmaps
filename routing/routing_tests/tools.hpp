#pragma once

#include "platform/platform_tests_support/async_gui_thread.hpp"

#include "routing/routing_session.hpp"

#include <memory>

namespace routing
{
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
