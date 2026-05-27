#import "RouteElevationPreviewData.h"

@implementation RouteElevationPreviewData

- (instancetype)initWithTrackInfo:(TrackInfo *)trackInfo
                    elevationInfo:(ElevationProfileData * _Nullable)elevationProfileData
{
  self = [super init];
  if (self)
  {
    _trackInfo = trackInfo;
    _elevationProfileData = elevationProfileData;
  }
  return self;
}

@end
