#include "platform/http_user_agent.hpp"

#include "std/target_os.hpp"

#include <sstream>

namespace platform
{
HttpUserAgent::HttpUserAgent()
{
  m_appVersion = ExtractAppVersion();
}

std::string HttpUserAgent::Get() const
{
  std::stringstream ss;
  ss << "MAPS.ME/";
#if defined(OMIM_OS_IPHONE)
  ss << "iOS/";
#elif defined(OMIM_OS_ANDROID)
  ss << "Android/";
#else
  ss << "Unknown/";
#endif
  ss << m_appVersion;
  return ss.str();
}
}  // namespace platform
