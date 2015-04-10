#pragma once

#include "routing/router.hpp"

#include "geometry/point2d.hpp"

namespace downloader { class HttpRequest; }

namespace routing
{

class OsrmOnlineRouter : public IRouter
{
  m2::PointD m_finalPt;

  /// Http callback from the server
  void OnRouteReceived(downloader::HttpRequest & request, ReadyCallback callback);

public:
  virtual string GetName() const;
  virtual void SetFinalPoint(m2::PointD const & finalPt);
  virtual void CalculateRoute(m2::PointD const & startingPt, ReadyCallback const & callback, m2::PointD const & direction = m2::PointD::Zero());
};

} // namespace routing
