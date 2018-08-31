#pragma once

#include "platform/gui_thread.hpp"
#include "platform/platform.hpp"

#include "base/worker_thread.hpp"

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
    GetPlatform().SetGuiThread(std::make_unique<base::WorkerThread>());
  }

  virtual ~AsyncGuiThread()
  {
    GetPlatform().SetGuiThread(std::make_unique<platform::GuiThread>());
  }

private:
  Platform::ThreadRunner m_runner;
};
}  // namespace tests_support
}  // namespace platform
