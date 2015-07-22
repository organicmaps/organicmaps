#include "routing/osrm_online_router.hpp"
#include "routing/route.hpp"

#include "indexer/mercator.hpp"

#include "platform/http_request.hpp"

#include "base/timer.hpp"
#include "base/string_utils.hpp"
#include "base/macros.hpp"
#include "base/logging.hpp"

#include "std/bind.hpp"

#include "3party/jansson/myjansson.hpp"

namespace routing
{

char const * OSRM_CAR_ROUTING_URL = "http://router.project-osrm.org/viaroute?output=json&compression=false&";

string OsrmOnlineRouter::GetName() const
{
  return "osrm-online";
}

void OsrmOnlineRouter::CalculateRoute(m2::PointD const & startPoint, m2::PointD const & /* direction */,
                                      m2::PointD const & /* finalPoint */, TReadyCallback const & callback,
                                      TProgressCallback const & /* progressCallback */)
{
  // Construct OSRM url request to get the route
  string url = OSRM_CAR_ROUTING_URL;
  using strings::to_string;
  url += "loc=" + to_string(MercatorBounds::YToLat(startPoint.y)) + "," + to_string(MercatorBounds::XToLon(startPoint.x))
      + "&loc=" + to_string(MercatorBounds::YToLat(m_finalPt.y)) + "," + to_string(MercatorBounds::XToLon(m_finalPt.x));

  // Request will be deleted in the callback
  downloader::HttpRequest::Get(url, bind(&OsrmOnlineRouter::OnRouteReceived, this, _1, callback));
}

void OsrmOnlineRouter::OnRouteReceived(downloader::HttpRequest & request, TReadyCallback callback)
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
            double lat, lon;
            bool validCoord = false;
            if (json_is_array(jcoords))
            {
              if (json_array_size(jcoords) >= 2)
              {
                lat = json_real_value(json_array_get(jcoords, 0));
                lon = json_real_value(json_array_get(jcoords, 1));
                validCoord = true;
              }
            }
            else if (json_is_string(jcoords))
            {
              string const coords = json_string_value(jcoords);
              string const strLat(coords, 0, coords.find(','));
              string const strLon(coords, coords.find(',') + 1);
              if (strings::to_double(strLat, lat) && strings::to_double(strLon, lon))
                validCoord = true;
            }

            if (validCoord)
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
        callback(result, IRouter::NoError);
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
