#include "routing/routing_quality/mapbox/api.hpp"

#include "routing/vehicle_mask.hpp"

#include "coding/file_writer.hpp"
#include "coding/serdes_json.hpp"
#include "coding/url_encode.hpp"
#include "coding/writer.hpp"

#include "platform/http_client.hpp"

#include "geometry/latlon.hpp"

#include <sstream>

using namespace std;
using namespace string_literals;

namespace
{
string const kBaseURL = "https://api.mapbox.com/";
string const kDirectionsApiVersion = "v5";
string const kStylesApiVersion = "v1";

string VehicleTypeToMapboxType(routing::VehicleType type)
{
  using routing::VehicleType;

  switch (type)
  {
  case VehicleType::Car:
    return "mapbox/driving"s;
  case VehicleType::Pedestrian:
    return "mapbox/walking"s;
  case VehicleType::Bicycle:
    return "mapbox/cycling"s;
  case VehicleType::Transit:
  case VehicleType::Count:
    CHECK(false, ());
    return ""s;
  }

  UNREACHABLE();
}

string LatLonsToString(vector<ms::LatLon> const & coords)
{
  ostringstream oss;
  auto const size = coords.size();
  for (size_t i = 0; i < size; ++i)
  {
    auto const & ll = coords[i];
    oss << ll.lon << "," << ll.lat;
    if (i + 1 != size)
      oss << ";";
  }

  oss << ".json";
  return UrlEncode(oss.str());
}
}  // namespace

namespace routing_quality
{
namespace mapbox
{
DirectionsResponse Api::MakeDirectionsRequest(RouteParams const & params) const
{
  CHECK(!m_accessToken.empty(), ());
  platform::HttpClient request(GetDirectionsURL(params));
  DirectionsResponse ret;
  if (request.RunHttpRequest() && !request.WasRedirected() && request.ErrorCode() == 200)
  {
    auto const & response = request.ServerResponse();
    CHECK(!response.empty(), ());
    {
      coding::DeserializerJson des(response);
      des(ret);
    }
  }
  else
  {
    CHECK(false, (request.ErrorCode(), request.ServerResponse()));
  }

  return ret;
}

void Api::DrawRoutes(Geometry const & mapboxRoute, Geometry const & mapsmeRoute, string const & snapshotPath) const
{
  CHECK(!m_accessToken.empty(), ());
  CHECK(!snapshotPath.empty(), ());

  platform::HttpClient request(GetRoutesRepresentationURL(mapboxRoute, mapsmeRoute));
  if (request.RunHttpRequest() && !request.WasRedirected() && request.ErrorCode() == 200)
  {
    auto const & response = request.ServerResponse();
    FileWriter w(snapshotPath);
    w.Write(response.data(), response.size());
    return;
  }

  CHECK(false, ("Mapbox api error:", request.ErrorCode()));
}

// static
vector<string> Api::GenerateWaypointsBasedOn(DirectionsResponse response)
{
  auto & routes = response.m_routes;
  CHECK(!routes.empty(), ());
  vector<string> res;
  res.reserve(routes.size());
  for (auto & r : routes)
  {
    auto & coords = r.m_geometry.m_coordinates;
    // Leave at most 10 waypoints from mapbox route.
    Api::Sieve(coords, 10 /* maxRemainingNumber */);
    ostringstream oss;
    oss << "{";
    auto const size = coords.size();
    for (size_t i = 0; i < size; ++i)
    {
      auto const & lonLat = coords[i];
      CHECK_EQUAL(lonLat.size(), 2, ());
      oss << "{" << lonLat[1] << ", " << lonLat[0] << "}";
      if (i + 1 != size)
        oss << ",\n";
    }
    
    oss << "}";
    res.emplace_back(oss.str());
  }
  
  return res;
}

string Api::GetDirectionsURL(RouteParams const & params) const
{
  ostringstream oss;
  oss << kBaseURL << "directions/" << kDirectionsApiVersion << "/" << VehicleTypeToMapboxType(params.m_type) << "/";
  oss << LatLonsToString(params.m_waypoints) << "?";
  oss << "access_token=" << m_accessToken << "&";
  oss << "overview=simplified&" << "geometries=geojson";
  return oss.str();
}

string Api::GetRoutesRepresentationURL(Geometry const & mapboxRoute, Geometry const & mapsmeRoute) const
{
  using Sink = MemWriter<string>;

  string mapboxData;
  {
    Sink sink(mapboxData);
    coding::SerializerJson<Sink> ser(sink);
    ser(mapboxRoute);
  }

  mapboxData = UrlEncode(mapboxData);

  string mapsmeData;
  {
    Sink sink(mapsmeData);
    coding::SerializerJson<Sink> ser(sink);
    ser(mapsmeRoute);
  }

  mapsmeData = UrlEncode(mapsmeData);

  ostringstream oss;
  oss << kBaseURL << "styles/" << kStylesApiVersion << "/mapbox/streets-v10/static/";
  oss << "geojson(" << mapboxData << ")"
      << ",geojson(" << mapsmeData << ")"<< "/auto/1000x1000?" << "access_token=" << m_accessToken;
  return oss.str();
}
}  // namespace mapbox
}  // namespace routing_quality
