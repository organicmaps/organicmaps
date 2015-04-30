#pragma once

#include "3party/Alohalytics/src/http_client.h"

#include "geometry/point2d.hpp"

#include "std/string.hpp"
#include "std/vector.hpp"

namespace routing
{
/// URL string generator for MAPS.ME OSRM server request.
/// \param serverURL http server url with protocol, name and port if needed. For example:
/// http://mail.ru:12345
/// \param startPoint Coordinates of a start point.
/// \param finalPoint Coordinates of a finish point.
/// \return URL for OSRM MAPS.ME server request.
/// \see MapsMePlugin.hpp for REST protocol.
string GenerateOnlineRequest(string const & serverURL, m2::PointD const & startPoint,
                             m2::PointD const & finalPoint);

/// \brief ParseResponse MAPS.ME OSRM server response parser.
/// \param serverResponse Server response data.
/// \param outPoints Mwm map points.
/// \return true if there are some maps in a server's response.
bool ParseResponse(string const & serverResponse, vector<m2::PointD> & outPoints);

class OnlineCrossFetcher
{
public:
  /// \brief OnlineCrossFetcher helper class to make request to online OSRM server
  ///        and get mwm name list
  /// \param serverURL Server URL
  /// \param startPoint Start point coordinate
  /// \param finalPoint Finish point coordinate
  OnlineCrossFetcher(string const & serverURL, m2::PointD const & startPoint,
                     m2::PointD const & finalPoint);

  /// \brief getMwmPoints Waits for a server response, and returns mwm representation points list.
  /// \return Mwm names to build route from startPt to finishPt. Empty list if there were errors.
  /// \warning Can take a long time while waits a server response.
  vector<m2::PointD> const & GetMwmPoints();

private:
  alohalytics::HTTPClientPlatformWrapper m_request;
  vector<m2::PointD> m_mwmPoints;
};
}
