#include "map/user.hpp"

#include "platform/http_client.hpp"
#include "platform/platform.hpp"

#include "coding/url_encode.hpp"

#include "base/logging.hpp"
#include "base/string_utils.hpp"

#include "3party/Alohalytics/src/alohalytics.h"
#include "3party/jansson/myjansson.hpp"

#include <chrono>
#include <sstream>

// #define STAGE_PASSPORT_SERVER
#define STAGE_UGC_SERVER
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
    socialTokenStr = "google";
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

  return kUGCServerUrl + "/user/reviews/";
}

std::string ReviewReceiverUrl()
{
  if (kUGCServerUrl.empty())
    return {};

  std::ostringstream ss;
  ss << kUGCServerUrl << "/receive/"
     << static_cast<int>(ReviewReceiverProtocol::LatestVersion) << "/";
  return ss.str();
}

std::string ParseAccessToken(std::string const & src)
{
  my::Json root(src.c_str());
  std::string tokenStr;
  FromJSONObject(root.get(), "access_token", tokenStr);
  return tokenStr;
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

User::~User()
{
  std::lock_guard<std::mutex> lock(m_mutex);
  m_needTerminate = true;
  m_condition.notify_one();
}

void User::Init()
{
  std::lock_guard<std::mutex> lock(m_mutex);

  std::string token;
  if (GetPlatform().GetSecureStorage().Load(kMapsMeTokenKey, token))
    m_accessToken = token;

  std::string reviewIds;
  if (GetPlatform().GetSecureStorage().Load(kReviewIdsKey, reviewIds))
    m_details.m_reviewIds = DeserializeReviewIds(reviewIds);

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
}

void User::Authenticate(std::string const & socialToken, SocialTokenType socialTokenType)
{
  std::string const url = AuthenticationUrl(socialToken, socialTokenType);
  if (url.empty())
  {
    LOG(LWARNING, ("Passport service is unavailable."));
    return;
  }

  {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_authenticationInProgress)
      return;
    m_authenticationInProgress = true;
  }

  //TODO: refactor this after adding support of delayed tasks in WorkerThread.
  // Also we need to strictly control destructors order to eliminate the case when
  // a delayed task calls destructed object.
  m_workerThread.Push([this, url]()
  {
    Request(url, nullptr, [this](std::string const & response)
    {
      SetAccessToken(ParseAccessToken(response));
    });
    std::lock_guard<std::mutex> lock(m_mutex);
    m_authenticationInProgress = false;
  });
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

  m_workerThread.Push([this, url]()
  {
    Request(url, [this](platform::HttpClient & request)
    {
      request.SetRawHeader("Authorization", m_accessToken);
    },
    [this](std::string const & response)
    {
      auto const reviewIds = DeserializeReviewIds(response);
      if (!reviewIds.empty())
      {
        GetPlatform().GetSecureStorage().Save(kReviewIdsKey, response);
        std::lock_guard<std::mutex> lock(m_mutex);
        m_details.m_reviewIds = reviewIds;
      }
    });
  });
}

void User::UploadUserReviews(std::string && dataStr,
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

  m_workerThread.Push([this, url, dataStr, onCompleteUploading]()
  {
    size_t const bytesCount = dataStr.size();
    Request(url, [this, dataStr](platform::HttpClient & request)
    {
      request.SetRawHeader("Authorization", m_accessToken);
      request.SetBodyData(dataStr, "application/json");
    },
    [this, bytesCount, onCompleteUploading](std::string const &)
    {
      alohalytics::Stats::Instance().LogEvent("UGC_DataUpload_finished",
                                              strings::to_string(bytesCount));
      LOG(LWARNING, ("Reviews have been uploaded."));

      if (onCompleteUploading != nullptr)
        onCompleteUploading();
    },
    [this, onCompleteUploading](int errorCode)
    {
      alohalytics::Stats::Instance().LogEvent("UGC_DataUpload_error",
                                              strings::to_string(errorCode));
      LOG(LWARNING, ("Reviews have not been uploaded."));

      if (onCompleteUploading != nullptr)
        onCompleteUploading();
    });
  });
}

void User::Request(std::string const & url, BuildRequestHandler const & onBuildRequest,
                   SuccessHandler const & onSuccess, ErrorHandler const & onError)
{
  ASSERT(onSuccess != nullptr, ());

  uint8_t constexpr kAttemptsCount = 3;
  uint32_t constexpr kWaitingInSeconds = 5;
  uint32_t constexpr kDegradationScalar = 2;

  uint32_t waitingTime = kWaitingInSeconds;
  int resultCode = -1;
  bool isSuccessfulCode = false;
  for (uint8_t i = 0; i < kAttemptsCount; ++i)
  {
    platform::HttpClient request(url);
    request.SetRawHeader("Accept", "application/json");
    if (onBuildRequest != nullptr)
      onBuildRequest(request);

    // TODO: Now passport service uses redirection. If it becomes false, uncomment checking.
    if (request.RunHttpRequest())// && !request.WasRedirected())
    {
      resultCode = request.ErrorCode();
      isSuccessfulCode = (resultCode == 200 || resultCode == 201);
      if (isSuccessfulCode) // Ok.
      {
        onSuccess(request.ServerResponse());
        break;
      }

      if (resultCode == 403) // Forbidden.
      {
        ResetAccessToken();
        LOG(LWARNING, ("Access denied for", url));
        break;
      }
    }

    // Wait for some time and retry.
    std::unique_lock<std::mutex> lock(m_mutex);
    m_condition.wait_for(lock, std::chrono::seconds(waitingTime),
                         [this]{return m_needTerminate;});
    if (m_needTerminate)
      break;
    waitingTime *= kDegradationScalar;
  }

  if (!isSuccessfulCode && onError != nullptr)
    onError(resultCode);
}
