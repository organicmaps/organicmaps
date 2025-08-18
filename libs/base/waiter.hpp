#pragma once

#include <chrono>
#include <condition_variable>
#include <mutex>

namespace base
{
// Class for multithreaded interruptable waiting.
class Waiter
{
public:
  enum class Result
  {
    PreviouslyNotified,
    Timeout,
    NoTimeout
  };

  void Wait()
  {
    std::unique_lock lock(m_mutex);

    if (m_notified)
      return;

    m_event.wait(lock, [this]() { return m_notified; });
  }

  template <typename Rep, typename Period>
  Result Wait(std::chrono::duration<Rep, Period> const & waitDuration)
  {
    std::unique_lock lock(m_mutex);

    if (m_notified)
      return Result::PreviouslyNotified;

    auto const result = m_event.wait_for(lock, waitDuration, [this]() { return m_notified; });

    return result ? Result::NoTimeout : Result::Timeout;
  }

  void Notify()
  {
    SetNotified(true);

    m_event.notify_all();
  }

  void Reset() { SetNotified(false); }

private:
  void SetNotified(bool notified)
  {
    std::lock_guard lock(m_mutex);
    m_notified = notified;
  }

  bool m_notified = false;
  std::mutex m_mutex;
  std::condition_variable m_event;
};

inline std::string DebugPrint(Waiter::Result result)
{
  switch (result)
  {
  case Waiter::Result::PreviouslyNotified: return "PreviouslyNotified";
  case Waiter::Result::NoTimeout: return "NoTimeout";
  case Waiter::Result::Timeout: return "Timeout";
  default: ASSERT(false, ("Unsupported value"));
  }

  return {};
}
}  // namespace base
