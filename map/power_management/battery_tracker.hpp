#pragma once

#include <chrono>
#include <cstdint>
#include <vector>

namespace power_management
{
class BatteryLevelTracker
{
public:
  class Subscriber
  {
  public:
    virtual void OnBatteryLevelReceived(uint8_t level) = 0;
  };

  void Subscribe(Subscriber * subscriber);
  void Unsubscribe(Subscriber * subscriber);
  void UnsubscribeAll();

private:
  uint8_t GetBatteryLevel();
  void RequestBatteryLevel();

  std::vector<Subscriber *> m_subscribers;
  std::chrono::system_clock::time_point m_lastRequestedTime;
  uint8_t m_lastReceivedlevel = 0;
};
}  // power_management
