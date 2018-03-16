#pragma once

#include <cstdint>
#include <functional>
#include <map>
#include <string>

namespace platform
{
class HttpUploader
{
public:
  struct Result
  {
    int32_t m_httpCode = 0;
    std::string m_description;
  };

  void SetMethod(std::string const & method) { m_method = method; }
  void SetUrl(std::string const & url) { m_url = url; }
  void SetParams(std::map<std::string, std::string> const & params) { m_params = params; }
  void SetParam(std::string const & key, std::string const & value) { m_params[key] = value; }
  void SetHeaders(std::map<std::string, std::string> const & headers) { m_headers = headers; }
  void SetFileKey(std::string const & fileKey) { m_fileKey = fileKey; }
  void SetFilePath(std::string const & filePath) { m_filePath = filePath; }

  Result Upload() const;

private:
  std::string m_method = "POST";
  std::string m_url;
  std::map<std::string, std::string> m_params;
  std::map<std::string, std::string> m_headers;
  std::string m_fileKey = "file";
  std::string m_filePath;
};
}  // namespace platform
