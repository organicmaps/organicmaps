#pragma once

#include <map>
#include <string>

namespace om::network::http
{
struct Payload
{
  std::string m_method = "POST";
  std::string m_url;
  std::map<std::string, std::string> m_params;
  std::map<std::string, std::string> m_headers;
  std::string m_fileKey = "file";
  std::string m_filePath;
  bool m_needClientAuth = false;
};
}  // namespace om::network::http
