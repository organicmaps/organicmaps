#import "PlacePageTrackData+Core.h"
#import "ElevationProfileData+Core.h"
#import "TrackInfo+Core.h"

@interface PlacePageTrackData ()

@property(nonatomic, readwrite) double activePointDistance;

@end

@implementation PlacePageTrackData

- (instancetype)initWithTrackInfo:(TrackInfo *)trackInfo
                            elevationInfo:(ElevationProfileData * _Nullable)elevationInfo
                     onActivePointChanged:(MWMVoidBlock)onActivePointChangedHandler {
  self = [super init];
  if (self) {
    _trackInfo = trackInfo;
    _elevationProfileData = elevationInfo;
    _onActivePointChangedHandler = onActivePointChangedHandler;
  }
  return self;
}

- (void)updateActivePointDistance:(double)distance {
  self.activePointDistance = distance;
  if (self.onActivePointChangedHandler)
    self.onActivePointChangedHandler();
}

@end

@implementation PlacePageTrackData (Core)

- (instancetype)initWithTrack:(Track const &)track
         onActivePointChanged:(MWMVoidBlock)onActivePointChangedHandler {
  self = [super init];
  if (self) {
    _trackId = track.GetData().m_id;
    _trackInfo = [[TrackInfo alloc] initWithTrackStatistics:track.GetStatistics()];

    auto const & bm = GetFramework().GetBookmarkManager();
    _activePointDistance = bm.GetElevationActivePoint(_trackId);
    _myPositionDistance = bm.GetElevationMyPosition(_trackId);
    _onActivePointChangedHandler = onActivePointChangedHandler;

    auto const & elevationInfo = track.GetElevationInfo();
    if (track.HasAltitudes() && elevationInfo.has_value()) {
      _elevationProfileData = [[ElevationProfileData alloc] initWithTrackId:_trackId
                                                              elevationInfo:elevationInfo.value()];
    }
  }
  return self;
}

@end
