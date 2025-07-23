#import "CLLocation+Mercator.h"
#import "MWMLocationHelpers.h"

@implementation CLLocation (Mercator)

- (m2::PointD)mercator
{
  return location_helpers::ToMercator(self.coordinate);
}

@end
