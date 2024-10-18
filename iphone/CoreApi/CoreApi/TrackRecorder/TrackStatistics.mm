#import "TrackStatistics+Core.h"

@implementation TrackStatistics

@end

@implementation TrackStatistics (Core)

- (instancetype)initWithTrackData:(Track const *)track {
  self = [super init];
  if (self) {
    _length = track->GetLengthMeters();
    _duration = track->GetDurationInSeconds();
    _ascend = 0;
    _descend = 0;
  }
  return self;
}

@end
