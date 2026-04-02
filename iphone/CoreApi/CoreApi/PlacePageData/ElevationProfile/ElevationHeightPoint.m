#import "ElevationHeightPoint.h"

@implementation ElevationHeightPoint

- (instancetype)initWithDistance:(double)distance altitude:(double)altitude
{
  self = [super init];
  if (self)
  {
    _distance = distance;
    _altitude = altitude;
  }
  return self;
}

@end
