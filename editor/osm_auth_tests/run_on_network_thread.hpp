#pragma once

#include "platform/platform.hpp"

#include <functional>

namespace testing
{
inline void RunOnNetworkThread(std::function<void()> && f)
{
#if defined(__APPLE__)
  // ThreadRunner's destructor waits until the thread's task is finished.
  Platform::ThreadRunner threadRunner;
  // Current HttpRequest implementation on Mac requires that it is run on the non-main thread,
  // see `dispatch_group_wait(group, DISPATCH_TIME_FOREVER);`
  // TODO(AB): Refactor HTTP networking to support async callbacks.
  GetPlatform().RunTask(Platform::Thread::Network, [f = std::move(f)]
  {
    f();
  });
#else
  f();
#endif
}
}  // namespace testing