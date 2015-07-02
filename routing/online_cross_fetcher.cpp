#include "online_cross_fetcher.hpp"

#include "platform/http_request.hpp"

#include "base/logging.hpp"
#include "base/string_utils.hpp"

#include "3party/jansson/myjansson.hpp"

#include "indexer/mercator.hpp"

#include "std/bind.hpp"

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
  return serverURL + "/mapsme?loc=" + strings::to_string(startPoint.lat) + ',' +
         strings::to_string(startPoint.lon) + "&loc=" + strings::to_string(finalPoint.lat) + ',' +
         strings::to_string(finalPoint.lon);
}

OnlineCrossFetcher::OnlineCrossFetcher(string const & serverURL, m2::PointD const & startPoint,
                                       m2::PointD const & finalPoint)
    : m_request(GenerateOnlineRequest(serverURL, startPoint, finalPoint))
{
  LOG(LINFO, ("Check mwms by URL: ", GenerateOnlineRequest(serverURL, startPoint, finalPoint)));
}

void OnlineCrossFetcher::Do()
{
  m_mwmPoints.clear();
  if (m_request.RunHTTPRequest() && m_request.error_code() == 200 && !m_request.was_redirected())
    ParseResponse(m_request.server_response(), m_mwmPoints);
  else
    LOG(LWARNING, ("Can't get OSRM server response. Code: ", m_request.error_code()));
}
}  // namespace routing
