#pragma once

#include "base/worker_thread.hpp"

#include <condition_variable>
#include <mutex>
#include <string>

// This class is thread-safe.
class User
{
public:
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

  std::string const & GetAccessToken() const;

private:
  void Init();
  void SetAccessToken(std::string const & accessToken);

  std::string m_accessToken;
  mutable std::mutex m_mutex;
  std::condition_variable m_condition;
  bool m_needTerminate = false;
  bool m_authenticationInProgress = false;
  base::WorkerThread m_workerThread;
};
