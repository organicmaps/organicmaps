#include "map/purchase.hpp"

#include "web_api/utils.hpp"

#include "platform/http_client.hpp"
#include "platform/platform.hpp"

#include "coding/serdes_json.hpp"
#include "coding/sha1.hpp"
#include "coding/writer.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"
#include "base/stl_helpers.hpp"
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

std::array<std::string, static_cast<size_t>(SubscriptionType::Count)> const kSubscriptionSuffix =
{
  "",  // removeAds (empty string for back compatibility)
  "_BookmarkCatalog",  // bookmarks city + outdoor (AllPass)
  "_BookmarksSights"   // bookmarks city
};

uint32_t constexpr kFirstWaitingTimeInSec = 1;
uint32_t constexpr kWaitingTimeScaleFactor = 2;
uint8_t constexpr kMaxAttemptIndex = 2;

std::string GetSubscriptionId(SubscriptionType type)
{
  return coding::SHA1::CalculateBase64ForString(GetPlatform().UniqueClientId() +
    kSubscriptionSuffix[base::Underlying(type)]);
}

std::string GetSubscriptionKey(SubscriptionType type)
{
  return kSubscriptionId + kSubscriptionSuffix[base::Underlying(type)];
}

std::string ValidationUrl()
{
  if (kServerUrl.empty())
    return {};
  return kServerUrl + "registrar/register";
}

std::string StartTransactionUrl()
{
  if (kServerUrl.empty())
    return {};
  return kServerUrl + "registrar/preorder";
}

std::string TrialEligibilityUrl()
{
  if (kServerUrl.empty())
    return {};
  return kServerUrl + "registrar/check_trial";
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
  bool m_isValid = false;
  bool m_isTrial = false;
  bool m_isTrialAvailable = false;
  std::string m_reason;

  DECLARE_VISITOR(visitor(m_isValid, false, "valid"),
                  visitor(m_isTrial, false, "is_trial_period"),
                  visitor(m_isTrialAvailable, false, "is_trial_available"),
                  visitor(m_reason, std::string(), "reason"))
};

ValidationResult DeserializeResponse(std::string const & response, int httpCode)
{
  ValidationResult result;
  try
  {
    coding::DeserializerJson deserializer(response);
    deserializer(result);
  }
  catch(std::exception const & e)
  {
    LOG(LWARNING, ("Bad server response. Code =", httpCode, ". Reason =", e.what()));
  }

  return result;
}
}  // namespace

Purchase::Purchase(InvalidTokenHandler && onInvalidToken)
  : m_onInvalidToken(std::move(onInvalidToken))
{
  for (size_t i = 0; i < base::Underlying(SubscriptionType::Count); ++i)
  {
    auto const t = static_cast<SubscriptionType>(i);

    std::string id;
    UNUSED_VALUE(GetPlatform().GetSecureStorage().Load(GetSubscriptionKey(t), id));

    auto const sid = GetSubscriptionId(t);
    m_subscriptionData.emplace_back(std::make_unique<SubscriptionData>(id == sid, sid));
  }
  CHECK_EQUAL(m_subscriptionData.size(), base::Underlying(SubscriptionType::Count), ());
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

void Purchase::SetStartTransactionCallback(StartTransactionCallback && callback)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  m_startTransactionCallback = std::move(callback);
}

bool Purchase::IsSubscriptionActive(SubscriptionType type) const
{
  CHECK(type != SubscriptionType::Count, ());
  return m_subscriptionData[base::Underlying(type)]->m_isActive;
}

void Purchase::SetSubscriptionEnabled(SubscriptionType type, bool isEnabled, bool isTrialActive)
{
  CHECK(type != SubscriptionType::Count, ());

  auto & data = m_subscriptionData[base::Underlying(type)];
  data->m_isActive = isEnabled;
  if (isEnabled)
    GetPlatform().GetSecureStorage().Save(GetSubscriptionKey(type), data->m_subscriptionId);
  else
    GetPlatform().GetSecureStorage().Remove(GetSubscriptionKey(type));

  for (auto & listener : m_listeners)
    listener->OnSubscriptionChanged(type, isEnabled);

  auto const nowStr = GetPlatform().GetMarketingService().GetPushWooshTimestamp();
  if (isTrialActive)
  {
    GetPlatform().GetMarketingService().SendPushWooshTag(
      marketing::kSubscriptionBookmarksAllTrialEnabled, nowStr);
  }
  if (type == SubscriptionType::BookmarksSights)
  {
    GetPlatform().GetMarketingService().SendPushWooshTag(isEnabled ?
      marketing::kSubscriptionBookmarksSightsEnabled :
      marketing::kSubscriptionBookmarksSightsDisabled, nowStr);
  }
  else if (type == SubscriptionType::BookmarksAll)
  {
    GetPlatform().GetMarketingService().SendPushWooshTag(isEnabled ?
      marketing::kSubscriptionBookmarksAllEnabled :
      marketing::kSubscriptionBookmarksAllDisabled, nowStr);
  }
  else if (type == SubscriptionType::RemoveAds)
  {
    GetPlatform().GetMarketingService().SendPushWooshTag(isEnabled ?
      marketing::kRemoveAdsSubscriptionEnabled :
      marketing::kRemoveAdsSubscriptionDisabled, nowStr);
  }
}

void Purchase::Validate(ValidationInfo const & validationInfo, std::string const & accessToken)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  std::string const url = ValidationUrl();
  auto const status = GetPlatform().ConnectionStatus();
  if (url.empty() || status == Platform::EConnectionType::CONNECTION_NONE || !validationInfo.IsValid())
  {
    if (m_validationCallback)
      m_validationCallback(ValidationCode::ServerError, ValidationResponse(validationInfo));
    return;
  }

  GetPlatform().RunTask(Platform::Thread::Network, [this, url, validationInfo, accessToken]()
  {
    ValidateImpl(url, validationInfo, accessToken, RequestType::Validation,
                 0 /* attemptIndex */, kFirstWaitingTimeInSec);
  });
}

void Purchase::StartTransaction(std::string const & serverId, std::string const & vendorId,
                                std::string const & accessToken)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  ValidationInfo info;
  info.m_serverId = serverId;
  info.m_vendorId = vendorId;

  std::string const url = StartTransactionUrl();
  auto const status = GetPlatform().ConnectionStatus();
  if (url.empty() || status == Platform::EConnectionType::CONNECTION_NONE)
  {
    if (m_startTransactionCallback)
      m_startTransactionCallback(false /* success */, serverId, vendorId);
    return;
  }

  GetPlatform().RunTask(Platform::Thread::Network, [this, url, info, accessToken]()
  {
    ValidateImpl(url, info, accessToken, RequestType::StartTransaction,
                 0 /* attemptIndex */, kFirstWaitingTimeInSec);
  });
}

void Purchase::SetTrialEligibilityCallback(TrialEligibilityCallback && callback)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  m_trialEligibilityCallback = std::move(callback);
}

void Purchase::CheckTrialEligibility(ValidationInfo const & validationInfo)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  std::string const url = TrialEligibilityUrl();
  auto const status = GetPlatform().ConnectionStatus();
  if (url.empty() || status == Platform::EConnectionType::CONNECTION_NONE || !validationInfo.IsValid())
  {
    if (m_trialEligibilityCallback)
      m_trialEligibilityCallback(TrialEligibilityCode::ServerError);
    return;
  }

  GetPlatform().RunTask(Platform::Thread::Network, [this, url, validationInfo]()
  {
    ValidateImpl(url, validationInfo, {} /* accessToken */, RequestType::TrialEligibility,
                 0 /* attemptIndex */, kFirstWaitingTimeInSec);
  });
}

void Purchase::ValidateImpl(std::string const & url, ValidationInfo const & validationInfo,
                            std::string const & accessToken, RequestType requestType,
                            uint8_t attemptIndex, uint32_t waitingTimeInSeconds)
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
    serializer(ValidationData(validationInfo, kReceiptType, web_api::DeviceId()));
  }
  request.SetBodyData(std::move(jsonStr), "application/json");

  ValidationResult result;
  ValidationCode code = ValidationCode::ServerError;
  if (request.RunHttpRequest())
  {
    auto const resultCode = request.ErrorCode();
    if (resultCode >= 200 && resultCode < 300)
    {
      result = DeserializeResponse(request.ServerResponse(), resultCode);
      code = ValidationCode::Verified;
    }
    else if (resultCode == 403)
    {
      if (m_onInvalidToken)
        m_onInvalidToken();

      code = ValidationCode::AuthError;
    }
    else if (resultCode >= 400 && resultCode < 500)
    {
      code = ValidationCode::NotVerified;

      result = DeserializeResponse(request.ServerResponse(), resultCode);

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
                                 [this, url, validationInfo, accessToken, requestType,
                                  attemptIndex, waitingTimeInSeconds]()
    {
      ValidateImpl(url, validationInfo, accessToken, requestType,
                   attemptIndex, waitingTimeInSeconds);
    });
  }
  else
  {
    GetPlatform().RunTask(Platform::Thread::Gui, [this, code, requestType, validationInfo, result]()
    {
      if (requestType == RequestType::StartTransaction)
      {
        if (m_startTransactionCallback)
        {
          m_startTransactionCallback(code == ValidationCode::Verified /* success */,
                                     validationInfo.m_serverId, validationInfo.m_vendorId);
        }
      }
      else if (requestType == RequestType::Validation)
      {
        if (m_validationCallback)
          m_validationCallback(code, {validationInfo, result.m_isTrial});
      }
      else if (requestType == RequestType::TrialEligibility)
      {
        if (m_trialEligibilityCallback)
        {
          TrialEligibilityCode eligibilityCode;
          if (code == ValidationCode::ServerError)
            eligibilityCode = TrialEligibilityCode::ServerError;
          else if (code == ValidationCode::Verified && result.m_isTrialAvailable)
            eligibilityCode = TrialEligibilityCode::Eligible;
          else
            eligibilityCode = TrialEligibilityCode::NotEligible;

          m_trialEligibilityCallback(eligibilityCode);
        }
      }
    });
  }
}
