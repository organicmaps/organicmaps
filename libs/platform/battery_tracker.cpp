#include "platform/battery_tracker.hpp"

#include "platform/platform.hpp"

namespace
{
auto const kBatteryTrackingInterval = std::chrono::minutes(10);

bool IsLevelExpired(std::chrono::system_clock::time_point lastRequestTime)
{
  return std::chrono::system_clock::now() - lastRequestTime > kBatteryTrackingInterval;
}
}  // namespace

namespace platform
{
void BatteryLevelTracker::Subscribe(Subscriber * subscriber)
{
  m_subscribers.push_back(subscriber);

  if (!IsLevelExpired(m_lastRequestTime))
    subscriber->OnBatteryLevelReceived(m_lastReceivedLevel);

  if (!m_isTrackingInProgress)
  {
    m_isTrackingInProgress = true;
    RequestBatteryLevel();
  }
}

void BatteryLevelTracker::Unsubscribe(Subscriber * subscriber)
{
  m_subscribers.erase(std::remove(m_subscribers.begin(), m_subscribers.end(), subscriber), m_subscribers.end());
}

void BatteryLevelTracker::UnsubscribeAll()
{
  m_subscribers.clear();
}

void BatteryLevelTracker::RequestBatteryLevel()
{
  if (m_subscribers.empty())
  {
    m_isTrackingInProgress = false;
    return;
  }

  if (IsLevelExpired(m_lastRequestTime))
  {
    m_lastReceivedLevel = GetPlatform().GetBatteryLevel();
    m_lastRequestTime = std::chrono::system_clock::now();
  }

  for (auto s : m_subscribers)
    s->OnBatteryLevelReceived(m_lastReceivedLevel);

  GetPlatform().RunDelayedTask(Platform::Thread::Background, kBatteryTrackingInterval, [this]
  { GetPlatform().RunTask(Platform::Thread::Gui, [this] { RequestBatteryLevel(); }); });
}
}  // namespace platform
