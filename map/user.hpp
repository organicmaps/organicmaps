#pragma once

#include "platform/safe_callback.hpp"

#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace platform
{
class HttpClient;
}

// This class is thread-safe.
class User
{
public:
  struct Details
  {
    using ReviewId = uint64_t;
    // m_reviewIds must be sorted.
    std::vector<ReviewId> m_reviewIds;
  };

  enum SocialTokenType
  {
    Facebook,
    Google,
    Phone
  };

  struct Subscriber
  {
    enum class Action
    {
      DoNothing,
      RemoveSubscriber
    };
    using AuthenticateHandler = std::function<void(bool success)>;
    using ChangeTokenHandler = std::function<void(std::string const & accessToken)>;

    Action m_postCallAction = Action::DoNothing;
    AuthenticateHandler m_onAuthenticate;
    ChangeTokenHandler m_onChangeToken;
  };

  using BuildRequestHandler = std::function<void(platform::HttpClient &)>;
  using SuccessHandler = std::function<void(std::string const &)>;
  using ErrorHandler = std::function<void(int, std::string const & response)>;
  using CompleteUploadingHandler = std::function<void(bool isSuccessful)>;
  using CompleteUserBindingHandler = platform::SafeCallback<void(bool isSuccessful)>;

  User();
  void Authenticate(std::string const & socialToken, SocialTokenType socialTokenType,
                    bool privacyAccepted, bool termsAccepted, bool promoAccepted);
  bool IsAuthenticated() const;
  void ResetAccessToken();
  void UpdateUserDetails();

  void AddSubscriber(std::unique_ptr<Subscriber> && subscriber);
  void ClearSubscribers();

  std::string GetAccessToken() const;
  std::string GetUserName() const;
  std::string GetUserId() const;
  Details GetDetails() const;

  void UploadUserReviews(std::string && dataStr, size_t numberOfUnsynchronized,
                         CompleteUploadingHandler const & onCompleteUploading);

  static std::string GetPhoneAuthUrl(std::string const & redirectUri);
  static std::string GetPrivacyPolicyLink();
  static std::string GetTermsOfUseLink();

  // Binds user with advertising id. It is necessary to prepare GDPR report.
  void BindUser(CompleteUserBindingHandler && completionHandler);

private:
  void Init();
  void SetAccessToken(std::string const & accessToken);
  void RequestBasicUserDetails(std::string const & accessToken,
                               SuccessHandler && onSuccess, ErrorHandler && onError);
  void RequestUserDetails();
  void Request(std::string const & url, BuildRequestHandler const & onBuildRequest,
               SuccessHandler const & onSuccess, ErrorHandler const & onError = nullptr);

  void RequestImpl(std::string const & url, BuildRequestHandler const & onBuildRequest,
                   SuccessHandler const & onSuccess, ErrorHandler const & onError,
                   uint8_t attemptIndex, uint32_t waitingTimeInSeconds);

  void NotifySubscribersImpl();
  void ClearSubscribersImpl();

  bool StartAuthentication();
  void FinishAuthentication();

  std::string m_accessToken;
  std::string m_userName;
  std::string m_userId;
  mutable std::mutex m_mutex;
  bool m_authenticationInProgress = false;
  Details m_details;
  std::vector<std::unique_ptr<Subscriber>> m_subscribers;
};

namespace lightweight
{
namespace impl
{
bool IsUserAuthenticated();
}  // namespace impl
}  //namespace lightweight
