#import "RouteElevationPreviewData.h"

@implementation RouteElevationPreviewData

- (instancetype)initWithTrackInfo:(TrackInfo *)trackInfo
                    elevationInfo:(ElevationProfileData * _Nullable)elevationProfileData
{
  return [self initWithTrackInfo:trackInfo elevationInfo:elevationProfileData entryPointIndex:0 finishPointIndex:-1];
}

- (instancetype)initWithTrackInfo:(TrackInfo *)trackInfo
                    elevationInfo:(ElevationProfileData * _Nullable)elevationProfileData
                  entryPointIndex:(double)entryPointIndex
                 finishPointIndex:(NSInteger)finishPointIndex
{
  self = [super init];
  if (self)
  {
    _entryPointIndex = entryPointIndex;
    _finishPointIndex = finishPointIndex;
    _trackInfo = trackInfo;
    _elevationProfileData = elevationProfileData;
  }
  return self;
}

@end
