#include "web_api/request_headers.hpp"

#include "web_api/utils.hpp"

#include "platform/platform.hpp"

namespace web_api
{
char const * const kDeviceIdHeader = "X-Mapsme-Device-Id";
char const * const kUserAgentHeader = "User-Agent";

platform::HttpClient::Headers GetDefaultCatalogHeaders()
{
  return {{kUserAgentHeader, GetPlatform().GetAppUserAgent()},
          {kDeviceIdHeader, DeviceId()}};
}
}  // namespace web_api
