#include "osrm_router.hpp"
#include "route.hpp"

#include "../indexer/mercator.hpp"

#include "../platform/http_request.hpp"

#include "../base/timer.hpp"
#include "../base/string_utils.hpp"
#include "../base/macros.hpp"
#include "../base/logging.hpp"

#include "../std/bind.hpp"

#include "../3party/jansson/myjansson.hpp"

namespace routing
{

char const * OSRM_CAR_ROUTING_URL = "http://router.project-osrm.org/viaroute?output=json&compression=false&";

string OsrmRouter::GetName() const
{
  return "osrm";
}

void OsrmRouter::SetFinalPoint(m2::PointD const & finalPt)
{
  m_finalPt = finalPt;
}

void OsrmRouter::CalculateRoute(m2::PointD const & startingPt, ReadyCallback const & callback)
{
  // Construct OSRM url request to get the route
  string url = OSRM_CAR_ROUTING_URL;
  url += "loc=" + strings::to_string(startingPt.x) + "," + strings::to_string(startingPt.y)
      + "&loc=" + strings::to_string(m_finalPt.x)   + "," + strings::to_string(m_finalPt.y);

  // Request will be deleted in the callback
  downloader::HttpRequest::Get(url, bind(&OsrmRouter::OnRouteReceived, this, _1));
}

void OsrmRouter::OnRouteReceived(downloader::HttpRequest & request)
{
  if (request.Status() == downloader::HttpRequest::ECompleted)
  {
    LOG(LINFO, ("Route HTTP request response:", request.Data()));

    // Parse received json request
    try
    {
      vector<m2::PointD> route;
      string routeName;

      my::Json json(request.Data().c_str());
      json_t const * jgeometry = json_object_get(json.get(), "route_geometry");
      if (jgeometry)
      {
        for (size_t i = 0; i < json_array_size(jgeometry); ++i)
        {
          json_t const * jcoords = json_array_get(jgeometry, i);
          if (jcoords)
          {
            string const coords = json_string_value(jcoords);
            string const strLon(coords, 0, coords.find(','));
            string const strLat(coords, coords.find(',') + 1);
            double lat, lon;
            if (strings::to_double(strLat, lat) && strings::to_double(strLon, lon))
              route.push_back(MercatorBounds::FromLatLon(lat, lon));
          }
        }
        json_t const * jsummary = json_object_get(json.get(), "route_summary");
        json_t const * jdistance = json_object_get(jsummary, "total_distance");
        json_t const * jtime = json_object_get(jsummary, "total_time");
        if (jdistance && jtime)
        {
          uint64_t const metres = static_cast<uint64_t>(json_number_value(jdistance));
          uint64_t const minutes = static_cast<uint64_t>(json_number_value(jtime) / 60.);
          routeName = strings::to_string(metres) + " m  " +  strings::to_string(minutes) + " min";
        }
      }

      if (!route.empty())
      {
        LOG(LINFO, ("Received route with", route.size(), "points"));
        Route result(GetName(), route, routeName + " " + my::FormatCurrentTime());
        m_callback(result);
      }
    }
    catch (my::Json::Exception const & ex)
    {
      LOG(LERROR, ("Can't parse route json response", ex.Msg()));
    }
  }
  else
  {
    LOG(LWARNING, ("Route HTTP request has failed", request.Data()));
  }

  // Should delete the request to avoid memory leak
  delete &request;
}

}
