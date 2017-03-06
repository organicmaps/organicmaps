#include "routing/online_cross_fetcher.hpp"

#include "platform/http_request.hpp"

#include "base/logging.hpp"
#include "base/string_utils.hpp"

#include "3party/jansson/myjansson.hpp"

#include "geometry/mercator.hpp"

#include "std/bind.hpp"

namespace
{
inline string LatLonToURLArgs(ms::LatLon const & point)
{
  return strings::to_string(point.lat) + ','+ strings::to_string(point.lon);
}
}  // namespace

namespace routing
{
bool ParseResponse(const string & serverResponse, vector<m2::PointD> & outPoints)
{
  try
  {
    my::Json parser(serverResponse.c_str());

    json_t const * countries = json_object_get(parser.get(), "used_mwms");
    size_t pointsCount = json_array_size(countries);
    outPoints.reserve(pointsCount);
    for (size_t i = 0; i < pointsCount; ++i)
    {
      json_t * pointArray = json_array_get(countries, i);
      outPoints.push_back({json_number_value(json_array_get(pointArray, 0)),
                           json_number_value(json_array_get(pointArray, 1))});
    }
    return !outPoints.empty();
  }
  catch (my::Json::Exception&)
  {
    return false;
  }
  return false;
}

string GenerateOnlineRequest(string const & serverURL, ms::LatLon const & startPoint,
                             ms::LatLon const & finalPoint)
{
  return serverURL + "/mapsme?loc=" + LatLonToURLArgs(startPoint) + "&loc=" +
         LatLonToURLArgs(finalPoint);
}

OnlineCrossFetcher::OnlineCrossFetcher(string const & serverURL, ms::LatLon const & startPoint,
                                       ms::LatLon const & finalPoint)
    : m_request(GenerateOnlineRequest(serverURL, startPoint, finalPoint))
{
  LOG(LINFO, ("Check mwms by URL: ", GenerateOnlineRequest(serverURL, startPoint, finalPoint)));
}

void OnlineCrossFetcher::Do()
{
  m_mwmPoints.clear();
  if (m_request.RunHttpRequest() && m_request.ErrorCode() == 200 && !m_request.WasRedirected())
    ParseResponse(m_request.ServerResponse(), m_mwmPoints);
  else
    LOG(LWARNING, ("Can't get OSRM server response. Code: ", m_request.ErrorCode()));
}
}  // namespace routing
