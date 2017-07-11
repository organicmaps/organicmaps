#pragma once

#include "platform/http_client.hpp"

#include <string>

namespace partners_api_utils
{
inline bool RunSimpleHttpRequest(std::string const & url, std::string & result)
{
  platform::HttpClient request(url);
  if (request.RunHttpRequest() && !request.WasRedirected() && request.ErrorCode() == 200)
  {
    result = request.ServerResponse();
    return true;
  }
  return false;
}
}  // namespace partners_api_utils
