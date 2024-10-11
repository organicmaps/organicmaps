#import "TrackRecordingInfo+Core.h"
#import "AltitudeFormatter.h"
#import "DistanceFormatter.h"
#import "DurationFormatter.h"

@implementation TrackRecordingInfo

@end

@implementation TrackRecordingInfo (Core)

- (instancetype)initWithGpsTrackInfo:(GpsTrackInfo const &)trackInfo {
  if (self = [super init]) {
    _distance = [DistanceFormatter distanceStringFromMeters:trackInfo.m_length];
    _duration = [DurationFormatter durationStringFromTimeInterval:trackInfo.m_duration];
    _ascent = [AltitudeFormatter altitudeStringFromMeters:trackInfo.m_ascent];
    _descent = [AltitudeFormatter altitudeStringFromMeters:trackInfo.m_descent];
    _maxElevation = [AltitudeFormatter altitudeStringFromMeters:trackInfo.m_maxElevation];
    _minElevation = [AltitudeFormatter altitudeStringFromMeters:trackInfo.m_minElevation];
  }
  return self;
}

@end
