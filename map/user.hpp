#pragma once

#include "base/worker_thread.hpp"

#include <condition_variable>
#include <functional>
#include <map>
#include <mutex>
#include <string>
#include <vector>

// This class is thread-safe.
class User
{
public:
  struct Details
  {
    using ReviewId = uint64_t;
    std::vector<ReviewId> m_reviewIds;
  };
  enum SocialTokenType
  {
    Facebook,
    Google
  };

  User();
  ~User();
  void Authenticate(std::string const & socialToken, SocialTokenType socialTokenType);
  bool IsAuthenticated() const;
  void ResetAccessToken();
  void UpdateUserDetails();

  std::string GetAccessToken() const;
  Details GetDetails() const;

private:
  void Init();
  void SetAccessToken(std::string const & accessToken);
  void RequestUserDetails();
  void Request(std::string const & url,
               std::map<std::string, std::string> const & headers,
               std::function<void(std::string const&)> const & onSuccess);

  std::string m_accessToken;
  mutable std::mutex m_mutex;
  std::condition_variable m_condition;
  bool m_needTerminate = false;
  bool m_authenticationInProgress = false;
  Details m_details;
  base::WorkerThread m_workerThread;
};
