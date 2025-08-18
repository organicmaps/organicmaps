#include "routing/routing_quality/api/mapbox/mapbox_api.hpp"

#include "routing/vehicle_mask.hpp"

#include "coding/file_writer.hpp"
#include "coding/serdes_json.hpp"
#include "coding/url.hpp"
#include "coding/writer.hpp"

#include "platform/http_client.hpp"

#include "geometry/latlon.hpp"
#include "geometry/mercator.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"

#include <sstream>

namespace
{
std::string const kBaseURL = "https://api.mapbox.com/";
std::string const kDirectionsApiVersion = "v5";
std::string const kCarType = "mapbox/driving";

std::string VehicleTypeToMapboxType(routing_quality::api::VehicleType type)
{
  switch (type)
  {
  case routing_quality::api::VehicleType::Car: return kCarType;
  }

  UNREACHABLE();
}

std::string LatLonsToString(std::vector<ms::LatLon> const & coords)
{
  std::ostringstream oss;
  auto const size = coords.size();
  for (size_t i = 0; i < size; ++i)
  {
    auto const & ll = coords[i];
    oss << ll.m_lon << "," << ll.m_lat;
    if (i + 1 != size)
      oss << ";";
  }

  oss << ".json";
  return url::UrlEncode(oss.str());
}
}  // namespace

namespace routing_quality::api::mapbox
{
// static
std::string const MapboxApi::kApiName = "mapbox";

MapboxApi::MapboxApi(std::string const & token) : RoutingApi(kApiName, token, kMaxRPS) {}

Response MapboxApi::CalculateRoute(Params const & params, int32_t /* startTimeZoneUTC */) const
{
  Response response;
  MapboxResponse mapboxResponse = MakeRequest(params);

  response.m_params = params;
  response.m_code = mapboxResponse.m_code;
  for (auto const & route : mapboxResponse.m_routes)
  {
    api::Route apiRoute;
    apiRoute.m_eta = route.m_duration;
    apiRoute.m_distance = route.m_distance;
    for (auto const & lonlat : route.m_geometry.m_coordinates)
    {
      CHECK_EQUAL(lonlat.size(), 2, ());

      auto const lon = lonlat.front();
      auto const lat = lonlat.back();
      apiRoute.m_waypoints.emplace_back(lat, lon);
    }

    response.m_routes.emplace_back(std::move(apiRoute));
  }

  return response;
}

MapboxResponse MapboxApi::MakeRequest(Params const & params) const
{
  MapboxResponse mapboxResponse;
  platform::HttpClient request(GetDirectionsURL(params));

  if (request.RunHttpRequest() && !request.WasRedirected() && request.ErrorCode() == 200)
  {
    auto const & response = request.ServerResponse();
    CHECK(!response.empty(), ());
    {
      mapboxResponse.m_code = ResultCode::ResponseOK;
      coding::DeserializerJson des(response);
      des(mapboxResponse);
    }
  }
  else
  {
    LOG(LWARNING, (request.ErrorCode(), request.ServerResponse()));
    mapboxResponse.m_code = ResultCode::Error;
  }

  return mapboxResponse;
}

std::string MapboxApi::GetDirectionsURL(Params const & params) const
{
  CHECK(!GetAccessToken().empty(), ());

  std::vector<ms::LatLon> coords;
  coords.reserve(params.m_waypoints.GetPoints().size());
  for (auto const & point : params.m_waypoints.GetPoints())
    coords.emplace_back(mercator::ToLatLon(point));

  std::ostringstream oss;
  oss << kBaseURL << "directions/" << kDirectionsApiVersion << "/" << VehicleTypeToMapboxType(params.m_type) << "/";
  oss << LatLonsToString(coords) << "?";
  oss << "access_token=" << GetAccessToken() << "&";
  oss << "overview=simplified&"
      << "geometries=geojson";
  oss << "&alternatives=true";

  return oss.str();
}
}  // namespace routing_quality::api::mapbox
