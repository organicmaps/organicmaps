#include "map/power_management/battery_tracker.hpp"

#include "platform/platform.hpp"

namespace
{
auto const kBatteryTrackingInterval = std::chrono::minutes(10);

bool IsLevelExpired(std::chrono::system_clock::time_point lastRequestedTime)
{
  return std::chrono::system_clock::now() - lastRequestedTime > kBatteryTrackingInterval;
}
}  // namespace

namespace power_management
{
void BatteryLevelTracker::Subscribe(Subscriber * subscriber)
{
  m_subscribers.push_back(subscriber);

  if (IsLevelExpired(m_lastRequestedTime))
  {
    // Run periodic requests when first subscriber is added.
    if (m_subscribers.size() == 1)
      RequestBatteryLevel();
  }
  else
  {
    subscriber->OnBatteryLevelReceived(m_lastReceivedlevel);
  }
}

void BatteryLevelTracker::Unsubscribe(Subscriber * subscriber)
{
  m_subscribers.erase(std::remove(m_subscribers.begin(), m_subscribers.end(), subscriber),
                      m_subscribers.end());
}

void BatteryLevelTracker::UnsubscribeAll()
{
  m_subscribers.clear();
}

uint8_t BatteryLevelTracker::GetBatteryLevel()
{
  if (IsLevelExpired(m_lastRequestedTime))
  {
    m_lastReceivedlevel = GetPlatform().GetBatteryLevel();
    m_lastRequestedTime = std::chrono::system_clock::now();
  }

  return m_lastReceivedlevel;
}

void BatteryLevelTracker::RequestBatteryLevel()
{
  if (m_subscribers.empty())
    return;

  auto const level = GetBatteryLevel();
  for (auto s : m_subscribers)
  {
    s->OnBatteryLevelReceived(level);
  }

  GetPlatform().RunDelayedTask(Platform::Thread::Background, kBatteryTrackingInterval, [this]
  {
    GetPlatform().RunTask(Platform::Thread::Gui, [this]
    {
      RequestBatteryLevel();
    });
  });
}
}  // namespace power_management
