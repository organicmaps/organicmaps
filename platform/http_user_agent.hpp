#pragma once

#include <string>

namespace platform
{
class HttpUserAgent
{
public:
  HttpUserAgent();
  std::string Get() const;

  operator std::string() const { return Get(); }

private:
  std::string ExtractAppVersion() const;

  std::string m_appVersion;
};
}  // platform
