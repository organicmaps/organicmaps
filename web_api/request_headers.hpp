#pragma once

#include "platform/http_client.hpp"

#include "geometry/point2d.hpp"

#include "base/geo_object_id.hpp"

#include <vector>

#include <boost/optional.hpp>

namespace web_api
{
class HeadersParams
{
public:
  boost::optional<m2::PointD> m_currentPosition;
  std::vector<base::GeoObjectId> m_countryGeoids;
  std::vector<base::GeoObjectId> m_cityGeoids;
};

platform::HttpClient::Headers GetDefaultCatalogHeaders();
platform::HttpClient::Headers GetCatalogHeaders(HeadersParams const & params);
}  // namespace web_api
