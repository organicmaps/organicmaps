#pragma once

#include <string>

namespace platform
{
class HttpUserAgent
{
public:
  HttpUserAgent();
  std::string Key() const;
  std::string Value() const;

private:
  std::string ExtractAppVersion() const;

  std::string m_appVersion;
};
}  // platform
