#pragma once

#include "platform/platform_tests_support/async_gui_thread.hpp"

#include "routing/routing_session.hpp"

#include <memory>

class AsyncGuiThreadTest
{
  platform::tests_support::AsyncGuiThread m_asyncGuiThread;
};

class AsyncGuiThreadTestWithRoutingSession : public AsyncGuiThreadTest
{
public:
  std::unique_ptr<routing::RoutingSession> m_session;
};
