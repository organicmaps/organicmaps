#import "ElevationHeightPoint.h"

@implementation ElevationHeightPoint

- (instancetype)initWithDistance:(double)distance andHeight:(double)height {
  self = [super init];
  if (self) {
    _distance = distance;
    _height = height;
  }
  return self;
}

@end
