#include "map/nogps/scheduler.hpp"

#include "platform/platform.hpp"

#include <chrono>

namespace nogps
{
PlatformScheduler::PlatformScheduler() : m_timerThread(1 /* threadsCount */, base::DelayedThreadPool::Exit::SkipPending)
{}

void PlatformScheduler::Post(int64_t delayMs, Task && task)
{
  m_timerThread.PushDelayed(std::chrono::milliseconds(delayMs), [task = std::move(task)]() mutable
  { GetPlatform().RunTask(Platform::Thread::Gui, std::move(task)); });
}

void Timer::Start(int64_t delayMs, Scheduler::Task && task)
{
  m_token = std::make_shared<bool>(true);
  m_scheduler.Post(delayMs, [token = std::weak_ptr<bool>(m_token), task = std::move(task)]
  {
    auto const current = token.lock();
    if (!current || !*current)
      return;
    *current = false;
    task();
  });
}
}  // namespace nogps
