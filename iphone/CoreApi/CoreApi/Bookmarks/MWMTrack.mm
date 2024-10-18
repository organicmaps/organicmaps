#import "MWMTrack+Core.h"
#import "TrackStatistics+Core.h"

@implementation MWMTrack

@end

@implementation MWMTrack (Core)

- (instancetype)initWithTrackId:(MWMTrackID)trackId trackData:(Track const *)track {
  self = [super init];
  if (self) {
    _trackId = trackId;
    _trackName = @(track->GetName().c_str());
    _trackLengthMeters = track->GetLengthMeters();
    auto const color = track->GetColor(0);
    _trackColor = [UIColor colorWithRed:color.GetRedF() green:color.GetGreenF() blue:color.GetBlueF() alpha:1.f];
    _statistics = [[TrackStatistics alloc] initWithTrackData:track];
  }
  return self;
}

@end
