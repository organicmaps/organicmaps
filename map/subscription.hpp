#pragma once

#include "base/thread_checker.hpp"

#include <atomic>
#include <string>
#include <vector>

class SubscriptionListener
{
public:
  virtual ~SubscriptionListener() = default;
  virtual void OnSubscriptionChanged(bool isActive) = 0;
};

class Subscription
{
public:
  Subscription();
  void Register(SubscriptionListener * listener);
  bool IsActive() const;
  void Validate();

private:
  std::atomic<bool> m_isActive;
  std::string m_subscriptionId;
  std::vector<SubscriptionListener *> m_listeners;

  ThreadChecker m_threadChecker;
};
