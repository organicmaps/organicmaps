#pragma once

#include "platform/gui_thread.hpp"
#include "platform/platform.hpp"

#include "base/thread_pool_delayed.hpp"

#include <memory>

namespace platform
{
namespace tests_support
{
class AsyncGuiThread
{
public:
  AsyncGuiThread()
  {
    GetPlatform().SetGuiThread(
        std::make_unique<base::DelayedThreadPool>(1 /* threadsCount */, base::DelayedThreadPool::Exit::ExecPending));
  }

  virtual ~AsyncGuiThread() { GetPlatform().SetGuiThread(std::make_unique<platform::GuiThread>()); }

private:
  Platform::ThreadRunner m_runner;
};
}  // namespace tests_support
}  // namespace platform
