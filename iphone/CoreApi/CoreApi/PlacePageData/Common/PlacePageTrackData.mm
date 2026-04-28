#import "ElevationProfileData+Core.h"
#import "PlacePageTrackData+Core.h"
#import "TrackInfo+Core.h"

@interface PlacePageTrackSelectionData ()

@property(nonatomic, readwrite) MWMTrackID trackId;
@property(nonatomic, readwrite) NSString * title;
@property(nonatomic, readwrite) UIColor * color;
@property(nonatomic, readwrite) BOOL isSelected;

- (instancetype)initWithTrackId:(MWMTrackID)trackId
                          title:(NSString *)title
                          color:(UIColor *)color
                     isSelected:(BOOL)isSelected;

@end

@implementation PlacePageTrackSelectionData

- (instancetype)initWithTrackId:(MWMTrackID)trackId
                          title:(NSString *)title
                          color:(UIColor *)color
                     isSelected:(BOOL)isSelected
{
  self = [super init];
  if (self)
  {
    _trackId = trackId;
    _title = title;
    _color = color;
    _isSelected = isSelected;
  }
  return self;
}

@end

@interface PlacePageTrackData ()

@property(nonatomic, readwrite) double activePointDistance;
@property(nonatomic, readwrite) NSArray<PlacePageTrackSelectionData *> * trackSelectionCandidates;

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
    _trackSelectionCandidates = @[];
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

    auto const & groupId = track.GetGroupId();
    if (groupId && bm.HasBmCategory(groupId))
    {
      _groupId = groupId;
      _trackCategory = [NSString stringWithCString:bm.GetCategoryName(groupId).c_str() encoding:NSUTF8StringEncoding];
    }

    auto const color = track.GetColor(0);
    _color = [UIColor colorWithRed:color.GetRedF() green:color.GetGreenF() blue:color.GetBlueF() alpha:1.f];

    std::string const description = track.GetDescription();
    _trackDescription = [NSString stringWithCString:description.c_str() encoding:NSUTF8StringEncoding];
    _isHtmlDescription = strings::IsHTML(description);
    _trackInfo = [[TrackInfo alloc] initWithTrackStatistics:track.GetStatistics()];
    _activePointDistance = bm.GetElevationActivePoint(_trackId);
    _myPositionDistance = bm.GetElevationMyPosition(_trackId);
    _onActivePointChangedHandler = onActivePointChangedHandler;
    NSMutableArray<PlacePageTrackSelectionData *> * trackSelectionCandidates = [NSMutableArray array];
    for (auto const & selectionInfo : rawData.GetTrackSelectionCandidates())
    {
      auto const * selectionTrack = bm.GetTrack(selectionInfo.m_trackId);
      if (selectionTrack == nullptr)
        continue;

      auto const selectionColor = selectionTrack->GetColor(0);
      auto * color = [UIColor colorWithRed:selectionColor.GetRedF()
                                     green:selectionColor.GetGreenF()
                                      blue:selectionColor.GetBlueF()
                                     alpha:1.f];
      auto * title = @(selectionTrack->GetName().c_str());
      auto * selectionData = [[PlacePageTrackSelectionData alloc] initWithTrackId:selectionInfo.m_trackId
                                                                            title:title
                                                                            color:color
                                                                       isSelected:selectionInfo.m_trackId == _trackId];
      [trackSelectionCandidates addObject:selectionData];
    }
    _trackSelectionCandidates = trackSelectionCandidates;

    auto const * elevationInfo = track.GetElevationInfo();
    if (elevationInfo)
      _elevationProfileData = [[ElevationProfileData alloc] initWithTrackId:_trackId elevationInfo:*elevationInfo];
  }
  return self;
}

@end
