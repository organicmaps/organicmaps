#import "ElevationProfileData+Core.h"
#import "PlacePageTrackData+Core.h"
#import "TrackInfo+Core.h"

@implementation PlacePageTrackCategory

- (instancetype)initWithGroupId:(MWMMarkGroupID)groupId name:(NSString *)name
{
  self = [super init];
  if (self)
  {
    _groupId = groupId;
    _name = [name copy];
  }
  return self;
}

@end

@interface PlacePageTrackData ()

@property(nonatomic, readwrite) double activePointDistance;

@end

@implementation PlacePageTrackData

- (instancetype)initWithTrackInfo:(TrackInfo *)trackInfo
                    elevationInfo:(ElevationProfileData * _Nullable)elevationInfo
             onActivePointChanged:(MWMVoidBlock)onActivePointChangedHandler
{
  self = [super init];
  if (self)
  {
    _trackInfo = trackInfo;
    _elevationProfileData = elevationInfo;
    _onActivePointChangedHandler = onActivePointChangedHandler;
  }
  return self;
}

- (void)updateActivePointDistance:(double)distance
{
  self.activePointDistance = distance;
  if (self.onActivePointChangedHandler)
    self.onActivePointChangedHandler();
}

@end

@implementation PlacePageTrackData (Core)

- (instancetype)initWithRawData:(place_page::Info const &)rawData
           onActivePointChanged:(MWMVoidBlock)onActivePointChangedHandler
{
  self = [super init];
  if (self)
  {
    auto const trackPtr = GetFramework().GetBookmarkManager().GetTrack(rawData.GetTrackId());
    auto const & track = *trackPtr;
    auto const & bm = GetFramework().GetBookmarkManager();

    _trackId = track.GetData().m_id;
    _isTempRelationTrack = (_trackId == kml::kTempRelationTrackId);

    auto const groupId = track.GetGroupId();
    if (bm.HasBmCategory(groupId))
      _category = [[PlacePageTrackCategory alloc] initWithGroupId:groupId name:@(bm.GetCategoryName(groupId).c_str())];

    auto const color = track.GetColor(0);
    _color = [UIColor colorWithRed:color.GetRedF() green:color.GetGreenF() blue:color.GetBlueF() alpha:1.f];

    _trackDescription = @(track.GetDescription().c_str());
    _trackInfo = [[TrackInfo alloc] initWithTrackStatistics:track.GetStatistics()];
    _activePointDistance = bm.GetElevationActivePoint(_trackId);
    _myPositionDistance = bm.GetElevationMyPosition(_trackId);
    _onActivePointChangedHandler = onActivePointChangedHandler;

    auto const * elevationInfo = track.GetElevationInfo();
    if (elevationInfo)
      _elevationProfileData = [[ElevationProfileData alloc] initWithTrackId:_trackId elevationInfo:*elevationInfo];
  }
  return self;
}

@end
