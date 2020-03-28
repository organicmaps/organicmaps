#import "ElevationProfileData+Core.h"

@implementation ElevationProfileData

@end

@implementation ElevationProfileData (Core)

- (instancetype)initWithElevationInfo:(ElevationInfo const &)elevationInfo
                          activePoint:(double)activePoint
                           myPosition:(double)myPosition {
  self = [super init];
  if (self) {
    _trackId = elevationInfo.GetId();
    _ascent = elevationInfo.GetAscent();
    _descent = elevationInfo.GetDescent();
    _maxAttitude = elevationInfo.GetMaxAltitude();
    _minAttitude = elevationInfo.GetMinAltitude();
    _difficulty = (ElevationDifficulty)elevationInfo.GetDifficulty();
    _trackTime = elevationInfo.GetDuration();
    NSMutableArray *pointsArray = [NSMutableArray array];
    for (auto const &p : elevationInfo.GetPoints()) {
      ElevationHeightPoint *point = [[ElevationHeightPoint alloc] initWithDistance:p.m_distance
                                                                       andAltitude:p.m_altitude];
      [pointsArray addObject:point];
    }
    _points = [pointsArray copy];
    _activePoint = activePoint;
    _myPosition = myPosition;
  }
  return self;
}


@end
