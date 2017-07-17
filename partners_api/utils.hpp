#pragma once

#include "platform/http_client.hpp"

#include <string>

namespace partners_api
{
namespace http
{
struct Result
{
  Result(bool result, int errorCode, std::string const & data)
    : m_result(result), m_errorCode(errorCode), m_data(data)
  {
  }

  operator bool() const { return m_result && m_errorCode == 200; }

  bool m_result = false;
  int m_errorCode = platform::HttpClient::kNoError;
  std::string m_data;
};

inline Result RunSimpleRequest(std::string const & url)
{
  platform::HttpClient request(url);
  bool result = false;

  if (request.RunHttpRequest() && !request.WasRedirected() && request.ErrorCode() == 200)
    result = true;

  return {result, request.ErrorCode(), request.ServerResponse()};
}
}  // namespace http
}  // namespace partners_api
