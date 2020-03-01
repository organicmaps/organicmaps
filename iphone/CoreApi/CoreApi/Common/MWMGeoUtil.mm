#import "MWMGeoUtil.h"

#include "geometry/mercator.hpp"
#include "geometry/angles.hpp"

@implementation MWMGeoUtil

+ (float)angleAtPoint:(CLLocationCoordinate2D)p1 toPoint:(CLLocationCoordinate2D)p2 {
  auto mp1 = mercator::FromLatLon(p1.latitude, p1.longitude);
  auto mp2 = mercator::FromLatLon(p2.latitude, p2.longitude);
  return ang::AngleTo(mp1, mp2);
}

@end
