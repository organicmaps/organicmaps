#import "TrackInfo+Core.h"
#import "AltitudeFormatter.h"
#import "DistanceFormatter.h"
#import "DurationFormatter.h"

@implementation TrackInfo

- (BOOL)hasElevationInfo {
  return _ascent != 0 || _descent != 0 || _maxElevation != 0 || _minElevation != 0;
}

+ (TrackInfo *)emptyInfo {
  return [[TrackInfo alloc] init];
}

@end

@implementation TrackInfo (Core)

- (instancetype)initWithTrackStatistics:(TrackStatistics const &)statistics {
  if (self = [super init]) {
    _distance = statistics.m_length;
    _duration = statistics.m_duration;
    _ascent = statistics.m_ascent;
    _descent = statistics.m_descent;
    _maxElevation = statistics.m_maxElevation;
    _minElevation = statistics.m_minElevation;
  }
  return self;
}

@end
