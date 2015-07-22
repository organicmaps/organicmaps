#pragma once

#include "routing/async_router.hpp"
#include "routing/router.hpp"

#include "geometry/point2d.hpp"

namespace downloader { class HttpRequest; }

namespace routing
{
class OsrmOnlineRouter : public AsyncRouter
{
  m2::PointD m_finalPt;

  /// Http callback from the server
  void OnRouteReceived(downloader::HttpRequest & request, TReadyCallback callback);

public:
  virtual string GetName() const;
  // AsyncRouter overrides:
  void CalculateRoute(m2::PointD const & startPoint, m2::PointD const & /* direction */,
                      m2::PointD const & /* finalPoint */, TReadyCallback const & callback,
                      TProgressCallback const & /* progressCallback */) override;
};

} // namespace routing
