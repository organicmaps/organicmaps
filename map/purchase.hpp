#pragma once

#include "base/thread_checker.hpp"

#include <atomic>
#include <functional>
#include <string>
#include <vector>

enum class SubscriptionType
{
  RemoveAds
};

class SubscriptionListener
{
public:
  virtual ~SubscriptionListener() = default;
  virtual void OnSubscriptionChanged(SubscriptionType type, bool isActive) = 0;
};

class Purchase
{
public:
  Purchase();
  void RegisterSubscription(SubscriptionListener * listener);
  bool IsSubscriptionActive(SubscriptionType type) const;

  void SetSubscriptionEnabled(SubscriptionType type, bool isEnabled);

  enum class ValidationCode
  {
    // Do not change the order.
    Verified,    // Purchase is verified.
    NotVerified, // Purchase is not verified.
    ServerError, // Server error during validation.
  };

  struct ValidationInfo
  {
    std::string m_serverId;
    std::string m_vendorId;
    std::string m_receiptData;

    bool IsValid() const { return !m_serverId.empty() && !m_vendorId.empty() && !m_receiptData.empty(); }
  };

  using ValidationCallback = std::function<void(ValidationCode, ValidationInfo const &)>;

  void SetValidationCallback(ValidationCallback && callback);
  void Validate(ValidationInfo const & validationInfo, std::string const & accessToken);

private:
  struct RemoveAdsSubscriptionData
  {
    std::atomic<bool> m_isActive;
    std::string m_subscriptionId;
  };
  RemoveAdsSubscriptionData m_removeAdsSubscriptionData;

  std::vector<SubscriptionListener *> m_listeners;

  ValidationCallback m_validationCallback;

  ThreadChecker m_threadChecker;
};
