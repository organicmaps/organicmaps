#include "online_cross_fetcher.hpp"

#include "platform/http_request.hpp"

#include "base/logging.hpp"
#include "base/string_utils.hpp"

#include "3party/jansson/myjansson.hpp"

#include "std/bind.hpp"

namespace routing
{
bool ParseResponse(const string & serverResponse, vector<m2::PointD> & outPoints)
{
  try
  {
    my::Json parser(serverResponse.c_str());

    json_t * countries = json_object_get(parser.get(), "used_mwms");
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
  catch (my::Json::Exception)
  {
    return false;
  }
  return false;
}

string GenerateOnlineRequest(string const & serverURL, m2::PointD const & startPoint,
                             m2::PointD const & finalPoint)
{
  return serverURL + "/mapsme?loc=" + strings::to_string(startPoint.y) + ',' +
         strings::to_string(startPoint.x) + "&loc=" + strings::to_string(finalPoint.y) + ',' +
         strings::to_string(finalPoint.x);
}

OnlineCrossFetcher::OnlineCrossFetcher(string const & serverURL, m2::PointD const & startPoint,
                                       m2::PointD const & finalPoint)
    : m_request(GenerateOnlineRequest(serverURL, startPoint, finalPoint))
{
  LOG(LINFO, ("Check mwms by URL: ", GenerateOnlineRequest(serverURL, startPoint, finalPoint)));
}

vector<m2::PointD> const & OnlineCrossFetcher::GetMwmPoints()
{
  m_mwmPoints.clear();
  if (!m_request.RunHTTPRequest())
    ParseResponse(m_request.server_response(), m_mwmPoints);
  return m_mwmPoints;
}
}
