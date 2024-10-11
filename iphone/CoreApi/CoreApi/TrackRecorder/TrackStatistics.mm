#import "TrackStatistics+Core.h"

@implementation TrackStatistics

@end

@implementation TrackStatistics (Core)

- (instancetype)initWithTrackData:(Track const *)track {
  self = [super init];
  if (self) {
    _length = track->GetLengthMeters();
    // TODO: add methods to call
    _duration = 0;
    _elevationGain = 0;
  }
  return self;
}

- (instancetype)initWithGpsTrackInfo:(GpsTrackCollection::GpsTrackInfo const &)trackInfo {
  if (self = [super init]) {
    _length = trackInfo.length;
    _duration = trackInfo.duration;
    _elevationGain = trackInfo.elevationGain;
  }
  return self;
}

@end
