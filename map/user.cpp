#include "map/user.hpp"

#include "platform/http_client.hpp"
#include "platform/platform.hpp"

#include "coding/url_encode.hpp"

#include "base/logging.hpp"

#include "3party/jansson/myjansson.hpp"

#include <chrono>
#include <sstream>
#include <thread>

#define STAGE_PASSPORT_SERVER
#include "private.h"

namespace
{
std::string const kMapsMeTokenKey = "MapsMeToken";
std::string const kServerUrl = PASSPORT_URL;

std::string AuthenticationUrl(std::string const & socialToken,
                              User::SocialTokenType socialTokenType)
{
  if (kServerUrl.empty())
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
  ss << kServerUrl << "/register-by-token/" << socialTokenStr
     << "/?access_token=" << UrlEncode(socialToken);
  return ss.str();
}

std::string ParseAccessToken(std::string const & src)
{
  my::Json root(src.c_str());
  std::string tokenStr;
  FromJSONObject(root.get(), "access_token", tokenStr);
  return tokenStr;
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
  std::string token;
  if (GetPlatform().GetSecureStorage().Load(kMapsMeTokenKey, token))
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_accessToken = token;
  }
}

void User::ResetAccessToken()
{
  std::lock_guard<std::mutex> lock(m_mutex);
  m_accessToken.clear();
  GetPlatform().GetSecureStorage().Remove(kMapsMeTokenKey);
}

bool User::IsAuthenticated() const
{
  std::lock_guard<std::mutex> lock(m_mutex);
  return !m_accessToken.empty();
}

std::string const & User::GetAccessToken() const
{
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_accessToken;
}

void User::SetAccessToken(std::string const & accessToken)
{
  std::lock_guard<std::mutex> lock(m_mutex);
  m_accessToken = accessToken;
  GetPlatform().GetSecureStorage().Save(kMapsMeTokenKey, m_accessToken);
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
  m_workerThread.Push([this, url]()
  {
    uint8_t constexpr kAttemptsCount = 3;
    uint32_t constexpr kWaitingInSeconds = 5;
    uint32_t constexpr kDegradationScalar = 2;

    uint32_t waitingTime = kWaitingInSeconds;
    for (uint8_t i = 0; i < kAttemptsCount; ++i)
    {
      platform::HttpClient request(url);
      request.SetRawHeader("Accept", "application/json");
      // TODO: Now passport service uses redirection. If it becomes false, uncomment checking.
      if (request.RunHttpRequest())// && !request.WasRedirected())
      {
        if (request.ErrorCode() == 200) // Ok.
        {
          SetAccessToken(ParseAccessToken(request.ServerResponse()));
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

    {
      std::lock_guard<std::mutex> lock(m_mutex);
      m_authenticationInProgress = false;
    }
  });
}
