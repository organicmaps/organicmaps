#pragma once

#include <functional>
#include <map>
#include <string>

namespace platform
{
class HttpUploader
{
public:
  using ResultCallback = std::function<void(int32_t httpCode)>;

  void SetMethod(std::string const & method) { m_method = method; }
  void SetUrl(std::string const & url) { m_url = url; }
  void SetParams(std::map<std::string, std::string> const & params) { m_params = params; }
  void SetHeaders(std::map<std::string, std::string> const & headers) { m_headers = headers; }
  void SetFileKey(std::string const & fileKey) { m_fileKey = fileKey; }
  void SetFilename(std::string const & filePath) { m_filePath = filePath; }
  void SetCallback(ResultCallback const & callback) { m_callback = callback; }

  void Upload() const;

private:
  std::string m_method = "POST";
  std::string m_url;
  std::map<std::string, std::string> m_params;
  std::map<std::string, std::string> m_headers;
  std::string m_fileKey = "file";
  std::string m_filePath;
  ResultCallback m_callback;
};
}  // namespace platform
