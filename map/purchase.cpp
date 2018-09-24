#include "map/purchase.hpp"

#include "platform/http_client.hpp"
#include "platform/platform.hpp"

#include "coding/serdes_json.hpp"
#include "coding/sha1.hpp"
#include "coding/writer.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"
#include "base/visitor.hpp"

#include "std/target_os.hpp"

#include <utility>

#include "private.h"

namespace
{
std::string const kSubscriptionId = "SubscriptionId";
std::string const kServerUrl = PURCHASE_SERVER_URL;
#if defined(OMIM_OS_IPHONE)
std::string const kReceiptType = "apple";
#elif defined(OMIM_OS_ANDROID)
std::string const kReceiptType = "google";
#else
std::string const kReceiptType {};
#endif

uint32_t constexpr kFirstWaitingTimeInSec = 1;
uint32_t constexpr kWaitingTimeScaleFactor = 2;
uint8_t constexpr kMaxAttemptIndex = 2;

std::string GetClientIdHash()
{
  return coding::SHA1::CalculateBase64ForString(GetPlatform().UniqueClientId());
}

std::string GetSubscriptionId()
{
  return GetClientIdHash();
}

std::string GetDeviceId()
{
  return GetClientIdHash();
}

std::string ValidationUrl()
{
  if (kServerUrl.empty())
    return {};
  return kServerUrl + "registrar/register";
}

struct ReceiptData
{
  ReceiptData(std::string const & data, std::string const & type)
    : m_data(data)
    , m_type(type)
  {}

  std::string m_data;
  std::string m_type;

  DECLARE_VISITOR(visitor(m_data, "data"),
                  visitor(m_type, "type"))
};

struct ValidationData
{
  ValidationData(Purchase::ValidationInfo const & validationInfo, std::string const & receiptType,
                 std::string const & deviceId)
    : m_serverId(validationInfo.m_serverId)
    , m_vendorId(validationInfo.m_vendorId)
    , m_receipt(validationInfo.m_receiptData, receiptType)
    , m_deviceId(deviceId)
  {}

  std::string m_serverId;
  std::string m_vendorId;
  ReceiptData m_receipt;
  std::string m_deviceId;

  DECLARE_VISITOR(visitor(m_serverId, "server_id"),
                  visitor(m_vendorId, "vendor"),
                  visitor(m_receipt, "receipt"),
                  visitor(m_deviceId, "device_id"))
};

struct ValidationResult
{
  std::string m_reason;

  DECLARE_VISITOR(visitor(m_reason, "reason"))
};
}  // namespace

Purchase::Purchase()
{
  std::string id;
  if (GetPlatform().GetSecureStorage().Load(kSubscriptionId, id))
    m_removeAdsSubscriptionData.m_subscriptionId = id;

  m_removeAdsSubscriptionData.m_isActive =
    (GetSubscriptionId() == m_removeAdsSubscriptionData.m_subscriptionId);
}

void Purchase::RegisterSubscription(SubscriptionListener * listener)
{
  CHECK(listener != nullptr, ());
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  m_listeners.emplace_back(listener);
}

void Purchase::SetValidationCallback(ValidationCallback && callback)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  m_validationCallback = std::move(callback);
}

bool Purchase::IsSubscriptionActive(SubscriptionType type) const
{
  switch (type)
  {
  case SubscriptionType::RemoveAds: return m_removeAdsSubscriptionData.m_isActive;
  }
  CHECK_SWITCH();
}

void Purchase::SetSubscriptionEnabled(SubscriptionType type, bool isEnabled)
{
  switch (type)
  {
  case SubscriptionType::RemoveAds:
  {
    m_removeAdsSubscriptionData.m_isActive = isEnabled;
    if (isEnabled)
    {
      m_removeAdsSubscriptionData.m_subscriptionId = GetSubscriptionId();
      GetPlatform().GetSecureStorage().Save(kSubscriptionId,
                                            m_removeAdsSubscriptionData.m_subscriptionId);
    }
    else
    {
      GetPlatform().GetSecureStorage().Remove(kSubscriptionId);
    }
    break;
  }
  }

  for (auto & listener : m_listeners)
    listener->OnSubscriptionChanged(type, isEnabled);
}

void Purchase::Validate(ValidationInfo const & validationInfo, std::string const & accessToken)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  std::string const url = ValidationUrl();
  auto const status = GetPlatform().ConnectionStatus();
  if (url.empty() || status == Platform::EConnectionType::CONNECTION_NONE || !validationInfo.IsValid())
  {
    if (m_validationCallback)
      m_validationCallback(ValidationCode::ServerError, validationInfo);
    return;
  }

  GetPlatform().RunTask(Platform::Thread::Network, [this, url, validationInfo, accessToken]()
  {
    ValidateImpl(url, validationInfo, accessToken, 0 /* attemptIndex */, kFirstWaitingTimeInSec);
  });
}

void Purchase::ValidateImpl(std::string const & url, ValidationInfo const & validationInfo,
                            std::string const & accessToken, uint8_t attemptIndex, uint32_t waitingTimeInSeconds)
{
  platform::HttpClient request(url);
  request.SetRawHeader("Accept", "application/json");
  request.SetRawHeader("User-Agent", GetPlatform().GetAppUserAgent());
  if (!accessToken.empty())
    request.SetRawHeader("Authorization", "Bearer " + accessToken);

  std::string jsonStr;
  {
    using Sink = MemWriter<std::string>;
    Sink sink(jsonStr);
    coding::SerializerJson<Sink> serializer(sink);
    serializer(ValidationData(validationInfo, kReceiptType, GetDeviceId()));
  }
  request.SetBodyData(std::move(jsonStr), "application/json");

  ValidationCode code = ValidationCode::ServerError;
  if (request.RunHttpRequest())
  {
    auto const resultCode = request.ErrorCode();
    if (resultCode >= 200 && resultCode < 300)
    {
      code = ValidationCode::Verified;
    }
    else if (resultCode >= 400 && resultCode < 500)
    {
      code = ValidationCode::NotVerified;

      ValidationResult result;
      try
      {
        coding::DeserializerJson deserializer(request.ServerResponse());
        deserializer(result);
      }
      catch(coding::DeserializerJson::Exception const &) {}

      if (!result.m_reason.empty())
        LOG(LWARNING, ("Validation error:", result.m_reason));
    }
    else
    {
      LOG(LWARNING, ("Unexpected validation error. Code =", resultCode,
        request.ServerResponse()));
    }
  }
  else
  {
    LOG(LWARNING, ("Validation request failed."));
  }

  if (code == ValidationCode::ServerError && attemptIndex < kMaxAttemptIndex)
  {
    auto const delayTime = std::chrono::seconds(waitingTimeInSeconds);
    ++attemptIndex;
    waitingTimeInSeconds *= kWaitingTimeScaleFactor;
    GetPlatform().RunDelayedTask(Platform::Thread::Network, delayTime,
                                 [this, url, validationInfo, accessToken, attemptIndex, waitingTimeInSeconds]()
    {
      ValidateImpl(url, validationInfo, accessToken, attemptIndex, waitingTimeInSeconds);
    });
  }
  else
  {
    GetPlatform().RunTask(Platform::Thread::Gui, [this, code, validationInfo]()
    {
      if (m_validationCallback)
        m_validationCallback(code, validationInfo);
    });
  }
}
