#import "ElevationHeightPoint.h"

@implementation ElevationHeightPoint

- (instancetype)initWithDistance:(double)distance andAltitude:(double)altitude {
  self = [super init];
  if (self) {
    _distance = distance;
    _altitude = altitude;
  }
  return self;
}

@end
