#pragma once

#include <chrono>
#include <cstdint>
#include <vector>

namespace platform
{
// Note: this class is NOT thread-safe.
class BatteryLevelTracker
{
public:
  class Subscriber
  {
  public:
    virtual void OnBatteryLevelReceived(uint8_t level) = 0;

  protected:
    virtual ~Subscriber() = default;
  };

  void Subscribe(Subscriber * subscriber);
  void Unsubscribe(Subscriber * subscriber);
  void UnsubscribeAll();

private:
  void RequestBatteryLevel();

  std::vector<Subscriber *> m_subscribers;
  std::chrono::system_clock::time_point m_lastRequestTime;
  uint8_t m_lastReceivedLevel = 0;
  bool m_isTrackingInProgress = false;
};
}  // namespace platform
