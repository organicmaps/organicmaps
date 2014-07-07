#pragma once

#include "router.hpp"

#include "../geometry/point2d.hpp"

namespace downloader { class HttpRequest; }

namespace routing
{

class OsrmRouter : public IRouter
{
  m2::PointD m_finalPt;
  ReadyCallback m_callback;

  /// Http callback from the server
  void OnRouteReceived(downloader::HttpRequest & request);

protected:
  virtual string RoutingUrl() const = 0;

public:
  virtual string GetName() const;
  virtual void SetFinalPoint(m2::PointD const & finalPt);
  virtual void CalculateRoute(m2::PointD const & startingPt, ReadyCallback const & callback);
};

} // namespace routing
