#include "map/user.hpp"

#include "platform/http_client.hpp"
#include "platform/platform.hpp"

#include "coding/url_encode.hpp"

#include "base/logging.hpp"
#include "base/stl_helpers.hpp"
#include "base/string_utils.hpp"

#include "3party/Alohalytics/src/alohalytics.h"
#include "3party/jansson/myjansson.hpp"

#include <chrono>
#include <limits>
#include <sstream>

//#define STAGE_PASSPORT_SERVER
//#define STAGE_UGC_SERVER
#include "private.h"

namespace
{
std::string const kMapsMeTokenKey = "MapsMeToken";
std::string const kReviewIdsKey = "UserReviewIds";
std::string const kPassportServerUrl = PASSPORT_URL;
std::string const kAppName = PASSPORT_APP_NAME;
std::string const kUGCServerUrl = UGC_URL;

enum class ReviewReceiverProtocol : uint8_t
{
  v1 = 1, // October 2017. Initial version.

  LatestVersion = v1
};

std::string AuthenticationUrl(std::string const & socialToken,
                              User::SocialTokenType socialTokenType)
{
  if (kPassportServerUrl.empty())
    return {};

  std::string socialTokenStr;
  switch (socialTokenType)
  {
  case User::SocialTokenType::Facebook:
    socialTokenStr = "facebook";
    break;
  case User::SocialTokenType::Google:
    socialTokenStr = "google-oauth2";
    break;
  default:
    LOG(LWARNING, ("Unknown social token type"));
    return {};
  }

  std::ostringstream ss;
  ss << kPassportServerUrl << "/register-by-token/" << socialTokenStr
     << "/?access_token=" << UrlEncode(socialToken) << "&app=" << kAppName;
  return ss.str();
}

std::string UserDetailsUrl()
{
  if (kUGCServerUrl.empty())
    return {};

  std::ostringstream ss;
  ss << kUGCServerUrl << "/"
     << static_cast<int>(ReviewReceiverProtocol::LatestVersion)
     << "/user/reviews/";
  return ss.str();
}

std::string ReviewReceiverUrl()
{
  if (kUGCServerUrl.empty())
    return {};

  std::ostringstream ss;
  ss << kUGCServerUrl << "/"
     << static_cast<int>(ReviewReceiverProtocol::LatestVersion)
     << "/receive/";
  return ss.str();
}

std::string ParseAccessToken(std::string const & src)
{
  my::Json root(src.c_str());
  std::string tokenStr;
  FromJSONObject(root.get(), "access_token", tokenStr);
  return tokenStr;
}

std::string BuildAuthorizationToken(std::string const & accessToken)
{
  return "Bearer " + accessToken;
}

std::vector<uint64_t> DeserializeReviewIds(std::string const & reviewIdsSrc)
{
  my::Json root(reviewIdsSrc.c_str());
  if (json_array_size(root.get()) == 0)
    return {};

  std::vector<uint64_t> result;
  try
  {
    result.resize(json_array_size(root.get()));
    for (size_t i = 0; i < result.size(); ++i)
    {
      auto const item = json_array_get(root.get(), i);
      FromJSON(item, result[i]);
    }
  }
  catch(my::Json::Exception const & ex)
  {
    LOG(LWARNING, ("Review ids deserialization failed."));
    return {};
  }
  return result;
}
}  // namespace

User::User()
{
  Init();
}

void User::Init()
{
  std::lock_guard<std::mutex> lock(m_mutex);

  std::string token;
  if (GetPlatform().GetSecureStorage().Load(kMapsMeTokenKey, token))
    m_accessToken = token;

  NotifySubscribersImpl();

  std::string reviewIds;
  if (GetPlatform().GetSecureStorage().Load(kReviewIdsKey, reviewIds))
  {
    m_details.m_reviewIds = DeserializeReviewIds(reviewIds);
    std::sort(m_details.m_reviewIds.begin(), m_details.m_reviewIds.end());
  }

  // Update user details on start up.
  auto const status = GetPlatform().ConnectionStatus();
  if (!m_accessToken.empty() && status == Platform::EConnectionType::CONNECTION_WIFI)
    RequestUserDetails();
}

void User::ResetAccessToken()
{
  std::lock_guard<std::mutex> lock(m_mutex);
  m_accessToken.clear();
  GetPlatform().GetSecureStorage().Remove(kMapsMeTokenKey);
  NotifySubscribersImpl();
}

void User::UpdateUserDetails()
{
  std::lock_guard<std::mutex> lock(m_mutex);
  if (m_authenticationInProgress || m_accessToken.empty())
    return;

  RequestUserDetails();
}

bool User::IsAuthenticated() const
{
  std::lock_guard<std::mutex> lock(m_mutex);
  return !m_accessToken.empty();
}

std::string User::GetAccessToken() const
{
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_accessToken;
}

User::Details User::GetDetails() const
{
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_details;
}

void User::SetAccessToken(std::string const & accessToken)
{
  std::lock_guard<std::mutex> lock(m_mutex);
  m_accessToken = accessToken;
  GetPlatform().GetSecureStorage().Save(kMapsMeTokenKey, m_accessToken);
  RequestUserDetails();
  NotifySubscribersImpl();
}

void User::Authenticate(std::string const & socialToken, SocialTokenType socialTokenType)
{
  std::string const url = AuthenticationUrl(socialToken, socialTokenType);
  if (url.empty())
  {
    LOG(LWARNING, ("Passport service is unavailable."));
    return;
  }

  if (!StartAuthentication())
    return;

  GetPlatform().RunTask(Platform::Thread::Network, [this, url]()
  {
    Request(url, nullptr, [this](std::string const & response)
    {
      SetAccessToken(ParseAccessToken(response));
      FinishAuthentication();
    }, [this](int)
    {
      FinishAuthentication();
    });
  });
}

bool User::StartAuthentication()
{
  std::lock_guard<std::mutex> lock(m_mutex);
  if (m_authenticationInProgress)
    return false;
  m_authenticationInProgress = true;
  return true;
}

void User::FinishAuthentication()
{
  std::lock_guard<std::mutex> lock(m_mutex);
  m_authenticationInProgress = false;

  NotifySubscribersImpl();
}

void User::AddSubscriber(std::unique_ptr<Subscriber> && subscriber)
{
  std::lock_guard<std::mutex> lock(m_mutex);

  if (subscriber->m_onChangeToken != nullptr)
  {
    subscriber->m_onChangeToken(m_accessToken);
    if (subscriber->m_postCallAction != Subscriber::Action::RemoveSubscriber)
      m_subscribers.push_back(std::move(subscriber));
  }
  else
  {
    m_subscribers.push_back(std::move(subscriber));
  }
}

void User::ClearSubscribers()
{
  std::lock_guard<std::mutex> lock(m_mutex);
  m_subscribers.clear();
}

void User::NotifySubscribersImpl()
{
  for (auto & s : m_subscribers)
  {
    if (s->m_onChangeToken != nullptr)
      s->m_onChangeToken(m_accessToken);
    if (s->m_onAuthenticate != nullptr)
      s->m_onAuthenticate(!m_accessToken.empty());
    if (s->m_postCallAction == Subscriber::Action::RemoveSubscriber)
      s.reset();
  }
  ClearSubscribersImpl();
}

void User::ClearSubscribersImpl()
{
  my::EraseIf(m_subscribers, [](auto const & s) { return s == nullptr; });
}

void User::RequestUserDetails()
{
  std::string const url = UserDetailsUrl();
  if (url.empty())
  {
    LOG(LWARNING, ("User details service is unavailable."));
    return;
  }

  if (m_accessToken.empty())
    return;

  GetPlatform().RunTask(Platform::Thread::Network, [this, url]()
  {
    Request(url, [this](platform::HttpClient & request)
    {
      request.SetRawHeader("Authorization", BuildAuthorizationToken(m_accessToken));
    },
    [this](std::string const & response)
    {
      auto const reviewIds = DeserializeReviewIds(response);
      if (!reviewIds.empty())
      {
        GetPlatform().GetSecureStorage().Save(kReviewIdsKey, response);
        std::lock_guard<std::mutex> lock(m_mutex);
        m_details.m_reviewIds = reviewIds;
        std::sort(m_details.m_reviewIds.begin(), m_details.m_reviewIds.end());
      }
    });
  });
}

void User::UploadUserReviews(std::string && dataStr, size_t numberOfUnsynchronized,
                             CompleteUploadingHandler const & onCompleteUploading)
{
  std::string const url = ReviewReceiverUrl();
  if (url.empty())
  {
    LOG(LWARNING, ("Reviews uploading is unavailable."));
    return;
  }

  if (m_accessToken.empty())
    return;

  GetPlatform().RunTask(Platform::Thread::Network,
                        [this, url, dataStr, numberOfUnsynchronized, onCompleteUploading]()
  {
    size_t const bytesCount = dataStr.size();
    Request(url, [this, dataStr](platform::HttpClient & request)
    {
      request.SetRawHeader("Authorization", BuildAuthorizationToken(m_accessToken));
      request.SetBodyData(dataStr, "application/json");
    },
    [this, bytesCount, onCompleteUploading](std::string const &)
    {
      alohalytics::Stats::Instance().LogEvent("UGC_DataUpload_finished",
                                              strings::to_string(bytesCount));
      LOG(LINFO, ("Reviews have been uploaded."));

      if (onCompleteUploading != nullptr)
        onCompleteUploading(true /* isSuccessful */);
    },
    [this, onCompleteUploading, numberOfUnsynchronized](int errorCode)
    {
      alohalytics::Stats::Instance().LogEvent("UGC_DataUpload_error",
                                              {{"error", strings::to_string(errorCode)},
                                               {"num", strings::to_string(numberOfUnsynchronized)}});
      LOG(LWARNING, ("Reviews have not been uploaded. Code =", errorCode));

      if (onCompleteUploading != nullptr)
        onCompleteUploading(false /* isSuccessful */);
    });
  });
}

void User::Request(std::string const & url, BuildRequestHandler const & onBuildRequest,
                   SuccessHandler const & onSuccess, ErrorHandler const & onError)
{
  uint32_t constexpr kWaitingInSeconds = 5;
  RequestImpl(url, onBuildRequest, onSuccess, onError, 0 /* attemptIndex */, kWaitingInSeconds);
}

void User::RequestImpl(std::string const & url, BuildRequestHandler const & onBuildRequest,
                       SuccessHandler const & onSuccess, ErrorHandler const & onError,
                       uint8_t attemptIndex, uint32_t waitingTimeInSeconds)
{
  ASSERT(onSuccess, ());

  uint8_t constexpr kMaxAttemptsCount = 3;
  uint32_t constexpr kDegradationFactor = 2;

  int resultCode = -1;
  bool isSuccessfulCode = false;

  platform::HttpClient request(url);
  request.SetRawHeader("Accept", "application/json");
  if (onBuildRequest)
    onBuildRequest(request);

  // TODO: Now passport service uses redirection. If it becomes false, uncomment check.
  if (request.RunHttpRequest())  // && !request.WasRedirected())
  {
    resultCode = request.ErrorCode();
    isSuccessfulCode = (resultCode == 200 || resultCode == 201);
    if (isSuccessfulCode)  // Ok.
    {
      onSuccess(request.ServerResponse());
      return;
    }

    if (resultCode == 403)  // Forbidden.
    {
      ResetAccessToken();
      LOG(LWARNING, ("Access denied for", url));
      if (onError)
        onError(resultCode);
      return;
    }
  }

  if (attemptIndex != 0)
  {
    ASSERT_LESS_OR_EQUAL(waitingTimeInSeconds,
                         std::numeric_limits<uint32_t>::max() / kDegradationFactor, ());
    waitingTimeInSeconds *= kDegradationFactor;
  }
  attemptIndex++;

  if (!isSuccessfulCode && attemptIndex == kMaxAttemptsCount && onError)
    onError(resultCode);

  if (attemptIndex < kMaxAttemptsCount)
  {
    GetPlatform().RunDelayedTask(Platform::Thread::Network,
                                 std::chrono::seconds(waitingTimeInSeconds),
                                 [this, url, onBuildRequest, onSuccess, onError,
                                  attemptIndex, waitingTimeInSeconds]()
    {
      RequestImpl(url, onBuildRequest, onSuccess, onError,
                  attemptIndex, waitingTimeInSeconds);
    });
  }
}
