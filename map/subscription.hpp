#pragma once

#include "base/thread_checker.hpp"

#include <atomic>
#include <functional>
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
  enum class ValidationCode
  {
    // Do not change the order.
    Active,    // Subscription is active.
    NotActive, // Subscription is not active.
    Failure,   // Validation failed, real subscription status is unknown, current one acts.
  };

  using ValidationCallback = std::function<void(ValidationCode)>;

  Subscription();
  void Register(SubscriptionListener * listener);
  void SetValidationCallback(ValidationCallback && callback);

  bool IsActive() const;
  void Validate(std::string const & receiptData, std::string const & accessToken);

private:
  void ApplyValidation(ValidationCode code);

  std::atomic<bool> m_isActive;
  std::string m_subscriptionId;
  std::vector<SubscriptionListener *> m_listeners;

  ValidationCallback m_validationCallback;

  ThreadChecker m_threadChecker;
};
