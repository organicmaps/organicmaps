#import "StringUtils+Core.h"
#import "TrackInfo+Core.h"

@implementation TrackInfo
{
  TrackStatistics _statistics;
}

+ (TrackInfo *)emptyInfo
{
  return [[TrackInfo alloc] initWithTrackStatistics:TrackStatistics()];
}

- (NSString *)distance
{
  return ToNSString(_statistics.GetFormattedLength());
}

- (NSString *)duration
{
  return ToNSString(_statistics.GetFormattedDuration());
}

- (NSString *)ascent
{
  return ToNSString(_statistics.GetFormattedAscent());
}

- (NSString *)descent
{
  return ToNSString(_statistics.GetFormattedDescent());
}

- (NSString *)maxElevation
{
  return ToNSString(_statistics.GetFormattedMaxElevation());
}

- (NSString *)minElevation
{
  return ToNSString(_statistics.GetFormattedMinElevation());
}

@end

@implementation TrackInfo (Core)

- (instancetype)initWithTrackStatistics:(TrackStatistics const &)statistics
{
  if (self = [super init])
  {
    _statistics = statistics;
    _hasElevationInfo = statistics.m_ascent != 0 || statistics.m_descent != 0 || statistics.m_maxElevation != 0 ||
                        statistics.m_minElevation != 0;
  }
  return self;
}

@end
