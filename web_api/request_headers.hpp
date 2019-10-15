#pragma once

#include "platform/http_client.hpp"

namespace web_api
{
extern char const * const kDeviceIdHeader;
extern char const * const kUserAgentHeader;

platform::HttpClient::Headers GetDefaultCatalogHeaders();
}  // namespace web_api
