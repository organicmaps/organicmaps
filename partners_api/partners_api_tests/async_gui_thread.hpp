#pragma once

#include "platform/gui_thread.hpp"
#include "platform/platform.hpp"

#include "base/stl_add.hpp"
#include "base/worker_thread.hpp"

namespace partners_api
{
class AsyncGuiThread
{
public:
  AsyncGuiThread()
  {
    GetPlatform().SetGuiThread(my::make_unique<base::WorkerThread>());
  }

  virtual ~AsyncGuiThread()
  {
    GetPlatform().SetGuiThread(my::make_unique<platform::GuiThread>());
  }
};
}  // namespace partners_api
