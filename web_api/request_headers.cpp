#include "web_api/request_headers.hpp"

#include "web_api/utils.hpp"

#include "platform/platform.hpp"

#include "geometry/mercator.hpp"

#include "base/string_utils.hpp"

namespace web_api
{
namespace
{
char const * const kDeviceIdHeader = "X-Mapsme-Device-Id";
char const * const kUserAgentHeader = "User-Agent";
char const * const kCitiesHeader = "X-Mapsme-City-Ids";
char const * const kCountriesHeader = "X-Mapsme-Country-Ids";
char const * const kLatLonHeader = "X-Mapsme-Lat-Lon";
}  // namespace

platform::HttpClient::Headers GetDefaultCatalogHeaders()
{
  return {{kUserAgentHeader, GetPlatform().GetAppUserAgent()},
          {kDeviceIdHeader, DeviceId()}};
}

platform::HttpClient::Headers GetCatalogHeaders(HeadersParams const & params)
{
  platform::HttpClient::Headers result = GetDefaultCatalogHeaders();

  if (params.m_currentPosition)
  {
    std::ostringstream latLonStream;
    auto const latLon = mercator::ToLatLon(params.m_currentPosition.get());
    latLonStream << std::fixed << std::setprecision(3) << latLon.m_lat << "," << latLon.m_lon;
    result.emplace(kLatLonHeader, latLonStream.str());
  }

  if (!params.m_cityGeoIds.empty())
    result.emplace(kCitiesHeader, strings::JoinAny(params.m_cityGeoIds));

  if (!params.m_countryGeoIds.empty())
    result.emplace(kCountriesHeader, strings::JoinAny(params.m_countryGeoIds));

  return result;
}
}  // namespace web_api
