#include "routing/online_cross_fetcher.hpp"

#include "platform/http_request.hpp"
#include "platform/platform.hpp"

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
bool ParseResponse(string const & serverResponse, vector<m2::PointD> & outPoints)
{
  try
  {
    my::Json parser(serverResponse.c_str());

    json_t const * countries = json_object_get(parser.get(), "used_mwms");
    size_t const pointsCount = json_array_size(countries);
    for (size_t i = 0; i < pointsCount; ++i)
    {
      json_t * pointArray = json_array_get(countries, i);
      outPoints.push_back({json_number_value(json_array_get(pointArray, 0)),
                           json_number_value(json_array_get(pointArray, 1))});
    }
    return pointsCount > 0;
  }
  catch (my::Json::Exception const & exception)
  {
    LOG(LWARNING, ("Can't parse server response:", exception.what()));
    LOG(LWARNING, ("Response body:", serverResponse));
    return false;
  }
}

string GenerateOnlineRequest(string const & serverURL, ms::LatLon const & startPoint,
                             ms::LatLon const & finalPoint)
{
  return serverURL + "/mapsme?loc=" + LatLonToURLArgs(startPoint) + "&loc=" +
    LatLonToURLArgs(finalPoint);
}

OnlineCrossFetcher::OnlineCrossFetcher(TCountryFileFn const & countryFileFn,
                                       string const & serverURL, Checkpoints const & checkpoints)
  : m_countryFileFn(countryFileFn), m_serverURL(serverURL), m_checkpoints(checkpoints)
{
  CHECK(m_countryFileFn, ());
}

void OnlineCrossFetcher::Do()
{
  m_mwmPoints.clear();

  for (size_t i = 0; i < m_checkpoints.GetNumSubroutes(); ++i)
  {
    m2::PointD const & pointFrom = m_checkpoints.GetPoint(i);
    m2::PointD const & pointTo = m_checkpoints.GetPoint(i + 1);

    string const mwmFrom = m_countryFileFn(pointFrom);
    string const mwmTo = m_countryFileFn(pointTo);
    if (mwmFrom == mwmTo)
      continue;

    string const url = GenerateOnlineRequest(m_serverURL, MercatorBounds::ToLatLon(pointFrom),
                                             MercatorBounds::ToLatLon(pointTo));
    platform::HttpClient request(url);
    request.SetRawHeader("User-Agent", GetPlatform().GetAppUserAgent());
    LOG(LINFO, ("Check mwms by URL: ", url));

    if (request.RunHttpRequest() && request.ErrorCode() == 200 && !request.WasRedirected())
      ParseResponse(request.ServerResponse(), m_mwmPoints);
    else
      LOG(LWARNING, ("Can't get OSRM server response. Code: ", request.ErrorCode()));
  }
}
}  // namespace routing
