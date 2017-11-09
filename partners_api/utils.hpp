#pragma once

#include "platform/http_client.hpp"

#include <chrono>
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

Result RunSimpleRequest(std::string const & url);
}  // namespace http

std::string FormatTime(std::chrono::system_clock::time_point p, std::string const & format);
}  // namespace partners_api
