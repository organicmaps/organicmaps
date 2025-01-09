#import "TrackInfo+Core.h"
#import "StringUtils.h"

@implementation TrackInfo

+ (TrackInfo *)emptyInfo {
  return [[TrackInfo alloc] initWithTrackStatistics:TrackStatistics()];
}

@end

@implementation TrackInfo (Core)

- (instancetype)initWithTrackStatistics:(TrackStatistics const &)statistics {
  if (self = [super init]) {
    _distance = ToNSString(statistics.GetFormattedLength());
    _duration = ToNSString(statistics.GetFormattedDuration());
    _ascent = ToNSString(statistics.GetFormattedAscent());
    _descent = ToNSString(statistics.GetFormattedDescent());
    _maxElevation = ToNSString(statistics.GetFormattedMaxElevation());
    _minElevation = ToNSString(statistics.GetFormattedMinElevation());
    _hasElevationInfo = statistics.m_ascent != 0 ||
                        statistics.m_descent != 0 ||
                        statistics.m_maxElevation != 0 ||
                        statistics.m_minElevation != 0;
  }
  return self;
}

@end
