#include "map/user.hpp"

#include "platform/http_client.hpp"
#include "platform/platform.hpp"

#include "coding/serdes_json.hpp"
#include "coding/url_encode.hpp"
#include "coding/writer.hpp"

#include "platform/preferred_languages.hpp"

#include "base/logging.hpp"
#include "base/stl_helpers.hpp"
#include "base/string_utils.hpp"

#include "base/visitor.hpp"

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
std::string const kUserNameKey = "MapsMeUserName";
std::string const kUserIdKey = "MapsMeUserId";
std::string const kDefaultUserName = "Anonymous Traveller";
std::string const kPassportServerUrl = PASSPORT_URL;
std::string const kAppName = PASSPORT_APP_NAME;
std::string const kUGCServerUrl = UGC_URL;
std::string const kApplicationJson = "application/json";

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

  std::ostringstream ss;
  ss << kPassportServerUrl;
  switch (socialTokenType)
  {
  case User::SocialTokenType::Facebook:
  {
    ss << "/register-by-token/facebook/";
    return ss.str();
  }
  case User::SocialTokenType::Google:
  {
    ss << "/register-by-token/google-oauth2/";
    return ss.str();
  }
  case User::SocialTokenType::Phone:
  {
    ss << "/otp/token/";
    return ss.str();
  }
  }
  CHECK_SWITCH();
}

std::string UserDetailsUrl()
{
  if (kPassportServerUrl.empty())
    return {};
  return kPassportServerUrl + "/user_details";
}

std::string UserReviewsUrl()
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

struct PhoneAuthRequestData
{
  std::string m_cliendId;
  std::string m_code;

  explicit PhoneAuthRequestData(std::string const & code)
    : m_cliendId(kAppName)
    , m_code(code)
  {}

  DECLARE_VISITOR(visitor(m_cliendId, "client_id"),
                  visitor(m_code, "code"))
};
  
struct SocialNetworkAuthRequestData
{
  std::string m_accessToken;
  std::string m_clientId;
  std::string m_privacyLink;
  std::string m_termsLink;
  bool m_privacyAccepted = false;
  bool m_termsAccepted = false;
  bool m_promoAccepted = false;
  
  DECLARE_VISITOR(visitor(m_accessToken, "access_token"),
                  visitor(m_clientId, "client_id"),
                  visitor(m_privacyLink, "privacy_link"),
                  visitor(m_termsLink, "terms_link"),
                  visitor(m_privacyAccepted, "privacy_accepted"),
                  visitor(m_termsAccepted, "terms_accepted"),
                  visitor(m_promoAccepted, "promo_accepted"))
};

struct UserDetailsResponseData
{
  std::string m_firstName;
  std::string m_lastName;
  std::string m_userId;

  DECLARE_VISITOR(visitor(m_firstName, "first_name"),
                  visitor(m_lastName, "last_name"),
                  visitor(m_userId, "keyid"))
};

template<typename DataType>
std::string SerializeToJson(DataType const & data)
{
  std::string jsonStr;
  using Sink = MemWriter<std::string>;
  Sink sink(jsonStr);
  coding::SerializerJson<Sink> serializer(sink);
  serializer(data);
  return jsonStr;
}

template<typename DataType>
void DeserializeFromJson(std::string const & jsonStr, DataType & result)
{
  coding::DeserializerJson des(jsonStr);
  des(result);
}
}  // namespace

User::User()
{
  Init();
}

void User::Init()
{
  std::lock_guard<std::mutex> lock(m_mutex);

  auto & secureStorage = GetPlatform().GetSecureStorage();

  std::string token;
  if (secureStorage.Load(kMapsMeTokenKey, token))
    m_accessToken = token;

  std::string userName;
  if (secureStorage.Load(kUserNameKey, userName))
    m_userName = userName;

  std::string userId;
  if (secureStorage.Load(kUserIdKey, userId))
    m_userId = userId;

  NotifySubscribersImpl();

  std::string reviewIds;
  if (secureStorage.Load(kReviewIdsKey, reviewIds))
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

std::string User::GetUserName() const
{
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_userName;
}

std::string User::GetUserId() const
{
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_userId;
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

void User::Authenticate(std::string const & socialToken, SocialTokenType socialTokenType,
                        bool privacyAccepted, bool termsAccepted, bool promoAccepted)
{
  std::string const url = AuthenticationUrl(socialToken, socialTokenType);
  if (url.empty())
  {
    LOG(LWARNING, ("Passport service is unavailable."));
    return;
  }

  if (!StartAuthentication())
    return;

  BuildRequestHandler authParams;
  if (socialTokenType == SocialTokenType::Phone)
  {
    authParams = [socialToken](platform::HttpClient & request)
    {
      auto jsonData = SerializeToJson(PhoneAuthRequestData(socialToken));
      request.SetBodyData(jsonData, kApplicationJson);
    };
  }
  else
  {
    SocialNetworkAuthRequestData authData;
    authData.m_accessToken = socialToken;
    authData.m_clientId = kAppName;
    authData.m_termsLink = GetTermsOfUseLink();
    authData.m_privacyLink = GetPrivacyPolicyLink();
    authData.m_termsAccepted = termsAccepted;
    authData.m_privacyAccepted = privacyAccepted;
    authData.m_promoAccepted = promoAccepted;
    authParams = [authData = std::move(authData)](platform::HttpClient & request)
    {
      auto jsonData = SerializeToJson(authData);
      request.SetBodyData(jsonData, kApplicationJson);
    };
  }

  GetPlatform().RunTask(Platform::Thread::Network,
                        [this, url, authParams = std::move(authParams)]()
  {
    Request(url, authParams, [this](std::string const & response)
    {
      SetAccessToken(ParseAccessToken(response));
      FinishAuthentication();
    }, [this](int code, std::string const & response)
    {
      LOG(LWARNING, ("Authentication failed. Code =", code, "Response =", response));
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
  std::string const detailsUrl = UserDetailsUrl();
  if (detailsUrl.empty())
  {
    LOG(LWARNING, ("User details service is unavailable."));
    return;
  }

  std::string const reviewsUrl = UserReviewsUrl();
  if (reviewsUrl.empty())
  {
    LOG(LWARNING, ("User reviews service is unavailable."));
    return;
  }

  if (m_accessToken.empty())
    return;

  GetPlatform().RunTask(Platform::Thread::Network, [this, detailsUrl, reviewsUrl]()
  {
    // Request user details.
    Request(detailsUrl, [this](platform::HttpClient & request)
    {
      request.SetRawHeader("Authorization", BuildAuthorizationToken(m_accessToken));
    },
    [this](std::string const & response)
    {
      UserDetailsResponseData userDetails;
      DeserializeFromJson(response, userDetails);

      auto userName = kDefaultUserName;
      if (!userDetails.m_firstName.empty() && !userDetails.m_lastName.empty())
        userName = userDetails.m_firstName + " " + userDetails.m_lastName;
      else if (!userDetails.m_firstName.empty())
        userName = userDetails.m_firstName;
      else if (!userDetails.m_lastName.empty())
        userName = userDetails.m_lastName;

      GetPlatform().GetSecureStorage().Save(kUserNameKey, userName);
      GetPlatform().GetSecureStorage().Save(kUserIdKey, userDetails.m_userId);

      std::lock_guard<std::mutex> lock(m_mutex);
      m_userName = userName;
      m_userId = userDetails.m_userId;
    });

    // Request user's reviews.
    Request(reviewsUrl, [this](platform::HttpClient & request)
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
      request.SetBodyData(dataStr, kApplicationJson);
    },
    [bytesCount, onCompleteUploading](std::string const &)
    {
      alohalytics::Stats::Instance().LogEvent("UGC_DataUpload_finished",
                                              strings::to_string(bytesCount));
      LOG(LINFO, ("Reviews have been uploaded."));

      if (onCompleteUploading != nullptr)
        onCompleteUploading(true /* isSuccessful */);
    },
    [onCompleteUploading, numberOfUnsynchronized](int errorCode, std::string const & response)
    {
      alohalytics::Stats::Instance().LogEvent("UGC_DataUpload_error",
                                              {{"error", strings::to_string(errorCode)},
                                               {"num", strings::to_string(numberOfUnsynchronized)}});
      LOG(LWARNING, ("Reviews have not been uploaded. Code =", errorCode, "Response =", response));

      if (onCompleteUploading != nullptr)
        onCompleteUploading(false /* isSuccessful */);
    });
  });
}

// static
std::string User::GetPhoneAuthUrl(std::string const & redirectUri)
{
  std::ostringstream os;
  os << kPassportServerUrl << "/oauth/authorize/?mode=phone_device&response_type=code"
     << "&locale=" << languages::GetCurrentOrig() << "&redirect_uri=" << UrlEncode(redirectUri)
     << "&client_id=" << kAppName;

  return os.str();
}

// static
std::string User::GetPrivacyPolicyLink()
{
  return "https://legal.my.com/us/maps/privacy/";
}

// static
std::string User::GetTermsOfUseLink()
{
  return "https://legal.my.com/us/maps/tou/";
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
  request.SetRawHeader("Accept", kApplicationJson);
  request.SetRawHeader("User-Agent", GetPlatform().GetAppUserAgent());
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
        onError(resultCode, request.ServerResponse());
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
    onError(resultCode, request.ServerResponse());

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

namespace lightweight
{
bool IsUserAuthenticated()
{
  std::string token;
  if (GetPlatform().GetSecureStorage().Load(kMapsMeTokenKey, token))
    return !token.empty();

  return false;
}
}  // namespace lightweight
