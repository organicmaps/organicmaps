#pragma once

#include "routing/checkpoints.hpp"
#include "routing/router.hpp"

#include "platform/http_client.hpp"

#include "geometry/point2d.hpp"
#include "geometry/latlon.hpp"

#include "base/thread.hpp"

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
string GenerateOnlineRequest(string const & serverURL, ms::LatLon const & startPoint,
                             ms::LatLon const & finalPoint);

/// \brief ParseResponse MAPS.ME OSRM server response parser.
/// \param serverResponse Server response data.
/// \param outPoints Mwm map points.
/// \return true if there are some maps in a server's response.
bool ParseResponse(string const & serverResponse, vector<m2::PointD> & outPoints);

class OnlineCrossFetcher : public threads::IRoutine
{
public:
  /// \brief OnlineCrossFetcher helper class to make request to online OSRM server
  ///        and get mwm names list
  OnlineCrossFetcher(TCountryFileFn const & countryFileFn, string const & serverURL,
                     Checkpoints const & checkpoints);

  /// Overrides threads::IRoutine processing procedure. Calls online OSRM server and parses response.
  void Do() override;

  /// \brief GetMwmPoints returns mwm representation points list.
  /// \return Mwm points to build route from startPt to finishPt. Empty list if there were errors.
  vector<m2::PointD> const & GetMwmPoints() { return m_mwmPoints; }

private:
  TCountryFileFn const m_countryFileFn;
  string const m_serverURL;
  Checkpoints const m_checkpoints;
  vector<m2::PointD> m_mwmPoints;
};
}
