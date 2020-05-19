#include "web_api/request_headers.hpp"

#include "web_api/utils.hpp"

#include "platform/platform.hpp"

#include "geometry/mercator.hpp"

#include "base/string_utils.hpp"

#include <utility>

namespace web_api
{
namespace
{
char const * const kDeviceIdHeader = "X-Mapsme-Device-Id";
char const * const kAdvertisingIdHeader = "X-Mapsme-Advertising-Id";
char const * const kUserAgentHeader = "User-Agent";
char const * const kCitiesHeader = "X-Mapsme-City-Ids";
char const * const kCountriesHeader = "X-Mapsme-Country-Ids";
char const * const kLatLonHeader = "X-Mapsme-Lat-Lon";
char const * const kGuidesHeader = "X-Mapsme-Downloaded-Guides";
}  // namespace

platform::HttpClient::Headers GetDefaultCatalogHeaders()
{
  return {{kUserAgentHeader, GetPlatform().GetAppUserAgent()},
          {kDeviceIdHeader, DeviceId()}};
}

platform::HttpClient::Headers GetDefaultAuthHeaders()
{
  platform::HttpClient::Headers result = {{kUserAgentHeader, GetPlatform().GetAppUserAgent()},
                                          {kDeviceIdHeader, DeviceId()}};

  auto adId = GetPlatform().AdvertisingId();
  if (!adId.empty())
    result.emplace(kAdvertisingIdHeader, std::move(adId));
  return result;
}

platform::HttpClient::Header GetPositionHeader(m2::PointD const & pos)
{
  std::ostringstream latLonStream;
  auto const latLon = mercator::ToLatLon(pos);
  latLonStream << std::fixed << std::setprecision(3) << latLon.m_lat << "," << latLon.m_lon;

  return {kLatLonHeader, latLonStream.str()};
}

platform::HttpClient::Headers GetCatalogHeaders(HeadersParams const & params)
{
  platform::HttpClient::Headers result = GetDefaultCatalogHeaders();

  if (params.m_currentPosition)
  {
    auto const latLon = GetPositionHeader(*params.m_currentPosition);
    result.emplace(latLon.m_name, latLon.m_value);
  }

  if (!params.m_cityGeoIds.empty())
    result.emplace(kCitiesHeader, strings::JoinAny(params.m_cityGeoIds));

  if (!params.m_countryGeoIds.empty())
    result.emplace(kCountriesHeader, strings::JoinAny(params.m_countryGeoIds));

  if (!params.m_downloadedGuidesIds.empty())
    result.emplace(kGuidesHeader, strings::JoinAny(params.m_downloadedGuidesIds));

  return result;
}
}  // namespace web_api
