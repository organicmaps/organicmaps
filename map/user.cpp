#include "map/user.hpp"

#include "platform/http_client.hpp"
#include "platform/platform.hpp"

#include "coding/url_encode.hpp"

#include "base/logging.hpp"

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
  std::ostringstream ss;
  ss << kUGCServerUrl << "/user/reviews";
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
    Request(url, {}, [this](std::string const & response)
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
    auto const headers = std::map<std::string, std::string>{{"Authorization", m_accessToken}};
    Request(url, headers, [this](std::string const & response)
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

void User::Request(std::string const & url,
                   std::map<std::string, std::string> const & headers,
                   std::function<void(std::string const &)> const & onSuccess)
{
  ASSERT(onSuccess != nullptr, ());

  uint8_t constexpr kAttemptsCount = 3;
  uint32_t constexpr kWaitingInSeconds = 5;
  uint32_t constexpr kDegradationScalar = 2;

  uint32_t waitingTime = kWaitingInSeconds;
  for (uint8_t i = 0; i < kAttemptsCount; ++i)
  {
    platform::HttpClient request(url);
    request.SetRawHeader("Accept", "application/json");
    for (auto const & header : headers)
      request.SetRawHeader(header.first, header.second);

    // TODO: Now passport service uses redirection. If it becomes false, uncomment checking.
    if (request.RunHttpRequest())// && !request.WasRedirected())
    {
      if (request.ErrorCode() == 200) // Ok.
      {
        onSuccess(request.ServerResponse());
        break;
      }

      if (request.ErrorCode() == 403) // Forbidden.
      {
        ResetAccessToken();
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
}
