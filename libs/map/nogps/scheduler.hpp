#pragma once

#include "base/thread_pool_delayed.hpp"

#include <cstdint>
#include <functional>
#include <memory>

namespace nogps
{
/// Runs tasks on the GUI thread later, everything of the navigation without GPS runs there.
class Scheduler
{
public:
  using Task = std::function<void()>;

  virtual ~Scheduler() = default;
  virtual void Post(int64_t delayMs, Task && task) = 0;
};

/// Delayed tasks of the GUI thread are not supported by Platform, so they wait on a thread of their own.
class PlatformScheduler : public Scheduler
{
public:
  PlatformScheduler();

  void Post(int64_t delayMs, Task && task) override;

private:
  base::DelayedThreadPool m_timerThread;
};

/// A one-shot timer: a restart or a stop cancels the pending run, the destroyed timer doesn't run.
class Timer
{
public:
  explicit Timer(Scheduler & scheduler) : m_scheduler(scheduler) {}

  void Start(int64_t delayMs, Scheduler::Task && task);
  void Stop() { m_token.reset(); }
  bool IsActive() const { return m_token != nullptr && *m_token; }

private:
  Scheduler & m_scheduler;
  // The pending run is done while it holds the current token.
  std::shared_ptr<bool> m_token;
};
}  // namespace nogps
