#pragma once

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
    Google
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
  using ErrorHandler = std::function<void(int)>;
  using CompleteUploadingHandler = std::function<void(bool)>;

  User();
  void Authenticate(std::string const & socialToken, SocialTokenType socialTokenType);
  bool IsAuthenticated() const;
  void ResetAccessToken();
  void UpdateUserDetails();

  void AddSubscriber(std::unique_ptr<Subscriber> && subscriber);
  void ClearSubscribers();

  std::string GetAccessToken() const;
  Details GetDetails() const;

  void UploadUserReviews(std::string && dataStr, size_t numberOfUnsynchronized,
                         CompleteUploadingHandler const & onCompleteUploading);

private:
  void Init();
  void SetAccessToken(std::string const & accessToken);
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
  mutable std::mutex m_mutex;
  bool m_authenticationInProgress = false;
  Details m_details;
  std::vector<std::unique_ptr<Subscriber>> m_subscribers;
};
