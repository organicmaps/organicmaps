#include "platform/http_user_agent.hpp"

#include <sstream>

namespace platform
{
HttpUserAgent::HttpUserAgent()
{
  m_appVersion = ExtractAppVersion();
}

std::string HttpUserAgent::Key() const
{
  return "User-Agent";
}

std::string HttpUserAgent::Value() const
{
  std::stringstream ss;
  ss << "MAPS.ME/";
#ifdef OMIM_OS_IPHONE
  ss << "iOS/";
#elif OMIM_OS_ANDROID
  ss << "Android/";
#endif
  ss << m_appVersion;
  return ss.str();
}
}  // platform
