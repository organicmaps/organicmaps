#pragma once

#include <map>
#include <string>

namespace platform
{
struct HttpPayload
{
  HttpPayload();

  std::string m_method;
  std::string m_url;
  std::map<std::string, std::string> m_params;
  std::map<std::string, std::string> m_headers;
  std::string m_fileKey;
  std::string m_filePath;
  bool m_needClientAuth;
};
}  // namespace platform
