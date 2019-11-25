#pragma once

#include "base/thread_checker.hpp"

#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

enum class SubscriptionType : uint8_t
{
  RemoveAds = 0,
  BookmarksAll,
  BookmarksSights,

  Count
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
  using InvalidTokenHandler = std::function<void()>;

  explicit Purchase(InvalidTokenHandler && onInvalidToken);
  void RegisterSubscription(SubscriptionListener * listener);
  bool IsSubscriptionActive(SubscriptionType type) const;

  void SetSubscriptionEnabled(SubscriptionType type, bool isEnabled);

  enum class ValidationCode
  {
    // Do not change the order.
    Verified,    // Purchase is verified.
    NotVerified, // Purchase is not verified.
    ServerError, // Server error during validation.
    AuthError,   // Authentication error during validation.
  };

  struct ValidationInfo
  {
    std::string m_serverId;
    std::string m_vendorId;
    std::string m_receiptData;

    // We do not check serverId here, because it can be empty in some cases.
    bool IsValid() const { return !m_vendorId.empty() && !m_receiptData.empty(); }
  };

  using ValidationCallback = std::function<void(ValidationCode, ValidationInfo const &)>;
  using StartTransactionCallback = std::function<void(bool success, std::string const & serverId,
                                                      std::string const & vendorId)>;

  void SetValidationCallback(ValidationCallback && callback);
  void Validate(ValidationInfo const & validationInfo, std::string const & accessToken);

  void SetStartTransactionCallback(StartTransactionCallback && callback);
  void StartTransaction(std::string const & serverId, std::string const & vendorId,
                        std::string const & accessToken);

private:
  void ValidateImpl(std::string const & url, ValidationInfo const & validationInfo,
                    std::string const & accessToken, bool startTransaction,
                    uint8_t attemptIndex, uint32_t waitingTimeInSeconds);

  // This structure is used in multithreading environment, so
  // fields must be either constant or atomic.
  struct SubscriptionData
  {
    std::atomic<bool> m_isActive;
    std::string const m_subscriptionId;

    SubscriptionData(bool isActive, std::string const & id)
      : m_isActive(isActive), m_subscriptionId(id)
    {}
  };
  std::vector<std::unique_ptr<SubscriptionData>> m_subscriptionData;

  std::vector<SubscriptionListener *> m_listeners;

  ValidationCallback m_validationCallback;
  StartTransactionCallback m_startTransactionCallback;
  InvalidTokenHandler m_onInvalidToken;

  ThreadChecker m_threadChecker;
};
