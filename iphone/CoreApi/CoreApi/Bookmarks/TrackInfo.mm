#import "StringUtils+Core.h"
#import "TrackInfo+Core.h"

@implementation TrackInfo

+ (TrackInfo *)emptyInfo
{
  return [[TrackInfo alloc] initWithTrackStatistics:TrackStatistics() hasElevationInfo:NO];
}

@end

@implementation TrackInfo (Core)

- (instancetype)initWithTrackStatistics:(TrackStatistics const &)statistics hasElevationInfo:(BOOL)hasElevationInfo
{
  if (self = [super init])
  {
    _distance = ToNSString(statistics.GetFormattedLength());
    _duration = ToNSString(statistics.GetFormattedDuration());
    _hasElevationInfo = hasElevationInfo;
    if (hasElevationInfo)
    {
      _ascent = ToNSString(statistics.GetFormattedAscent());
      _descent = ToNSString(statistics.GetFormattedDescent());
      _maxElevation = ToNSString(statistics.GetFormattedMaxElevation());
      _minElevation = ToNSString(statistics.GetFormattedMinElevation());
    }
    else
      _ascent = _descent = _maxElevation = _minElevation = @"";
  }
  return self;
}

@end
