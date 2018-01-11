#pragma once

#include <functional>
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

  using BuildRequestHandler = std::function<void(platform::HttpClient &)>;
  using SuccessHandler = std::function<void(std::string const &)>;
  using ErrorHandler = std::function<void(int)>;
  using CompleteUploadingHandler = std::function<void(bool)>;

  User();
  void Authenticate(std::string const & socialToken, SocialTokenType socialTokenType);
  bool IsAuthenticated() const;
  void ResetAccessToken();
  void UpdateUserDetails();

  std::string GetAccessToken() const;
  Details GetDetails() const;

  void UploadUserReviews(std::string && dataStr,
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

  std::string m_accessToken;
  mutable std::mutex m_mutex;
  bool m_authenticationInProgress = false;
  Details m_details;
};
