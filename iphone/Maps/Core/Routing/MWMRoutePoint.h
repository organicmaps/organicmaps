#import "SwiftBridge.h"

#include "geometry/latlon.hpp"
#include "geometry/mercator.hpp"
#include "geometry/point2d.hpp"

static inline MWMRoutePoint * makeMWMRoutePoint(m2::PointD const & point, NSString * name)
{
  return [[MWMRoutePoint alloc] initWithX:point.x y:point.y name:name isMyPosition:false];
}

static inline MWMRoutePoint * makeMWMRoutePoint(m2::PointD const & point)
{
  return [[MWMRoutePoint alloc] initWithX:point.x y:point.y];
}

static inline MWMRoutePoint * makeMWMRoutePointZero() { return [[MWMRoutePoint alloc] init]; }
static inline m2::PointD mercatorMWMRoutePoint(MWMRoutePoint * point)
{
  return m2::PointD(point.x, point.y);
}

static inline ms::LatLon latlonMWMRoutePoint(MWMRoutePoint * point)
{
  return MercatorBounds::ToLatLon(mercatorMWMRoutePoint(point));
}
