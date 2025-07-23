#import "PlacePageData.h"

#import "ElevationProfileData+Core.h"
#import "MWMMapNodeAttributes.h"
#import "PlacePageBookmarkData+Core.h"
#import "PlacePageButtonsData+Core.h"
#import "PlacePageInfoData+Core.h"
#import "PlacePagePreviewData+Core.h"
#import "PlacePageTrackData+Core.h"

#include <CoreApi/CoreApi.h>
#include "platform/network_policy.hpp"

static place_page::Info & rawData()
{
  return GetFramework().GetCurrentPlacePageInfo();
}

static PlacePageRoadType convertRoadType(RoadWarningMarkType roadType)
{
  switch (roadType)
  {
  case RoadWarningMarkType::Toll: return PlacePageRoadTypeToll;
  case RoadWarningMarkType::Ferry: return PlacePageRoadTypeFerry;
  case RoadWarningMarkType::Dirty: return PlacePageRoadTypeDirty;
  case RoadWarningMarkType::Count: return PlacePageRoadTypeNone;
  }
}

@interface PlacePageData () <MWMStorageObserver>
{
  FeatureID m_featureID;
  m2::PointD m_mercator;
  std::vector<std::string> m_rawTypes;
}

@property(nonatomic, readwrite) PlacePagePreviewData * previewData;
@property(nonatomic, readwrite) CLLocationCoordinate2D locationCoordinate;

- (PlacePageObjectType)objectTypeFromRawData;

@end

@implementation PlacePageData

- (instancetype)initWithLocalizationProvider:(id<IOpeningHoursLocalization>)localization
{
  self = [super init];
  if (self)
  {
    _buttonsData = [[PlacePageButtonsData alloc] initWithRawData:rawData()];
    _infoData = [[PlacePageInfoData alloc] initWithRawData:rawData() ohLocalization:localization];

    if (rawData().IsBookmark())
      _bookmarkData = [[PlacePageBookmarkData alloc] initWithRawData:rawData()];

    if (auto const & wikiDescription = rawData().GetWikiDescription(); !wikiDescription.empty())
      _wikiDescriptionHtml = @(("<html><body>" + wikiDescription + "</body></html>").c_str());

    _roadType = convertRoadType(rawData().GetRoadType());

    _isMyPosition = rawData().IsMyPosition();
    _isRoutePoint = rawData().IsRoutePoint();
    _isPreviewPlus = rawData().GetOpeningMode() == place_page::OpeningMode::PreviewPlus;
    auto latlon = rawData().GetLatLon();
    _locationCoordinate = CLLocationCoordinate2DMake(latlon.m_lat, latlon.m_lon);

    NSMutableArray * tagsArray = [NSMutableArray array];
    for (auto const & s : rawData().GetRawTypes())
      [tagsArray addObject:@(s.c_str())];

    if (rawData().IsTrack())
    {
      __weak auto weakSelf = self;
      _trackData =
          [[PlacePageTrackData alloc] initWithRawData:rawData()
                                 onActivePointChanged:^(void) { [weakSelf handleActiveTrackSelectionPointChanged]; }];
    }
    _previewData = [[PlacePagePreviewData alloc] initWithRawData:rawData()];

    auto const & countryId = rawData().GetCountryId();
    if (!countryId.empty())
    {
      _mapNodeAttributes = [[MWMStorage sharedStorage] attributesForCountry:@(rawData().GetCountryId().c_str())];
      [[MWMStorage sharedStorage] addObserver:self];
    }

    _objectType = [self objectTypeFromRawData];

    m_featureID = rawData().GetID();
    m_mercator = rawData().GetMercator();
    m_rawTypes = rawData().GetRawTypes();
  }
  return self;
}

- (instancetype)initWithTrackInfo:(TrackInfo * _Nonnull)trackInfo
                    elevationInfo:(ElevationProfileData * _Nullable)elevationInfo
{
  self = [super init];
  if (self)
  {
    _objectType = PlacePageObjectTypeTrackRecording;
    _roadType = PlacePageRoadTypeNone;
    _previewData = [[PlacePagePreviewData alloc] initWithTrackInfo:trackInfo];
    __weak auto weakSelf = self;
    _trackData =
        [[PlacePageTrackData alloc] initWithTrackInfo:trackInfo
                                        elevationInfo:elevationInfo
                                 onActivePointChanged:^(void) { [weakSelf handleActiveTrackSelectionPointChanged]; }];
  }
  return self;
}

- (void)updateWithTrackInfo:(TrackInfo * _Nonnull)trackInfo
              elevationInfo:(ElevationProfileData * _Nullable)elevationInfo
{
  _previewData = [[PlacePagePreviewData alloc] initWithTrackInfo:trackInfo];
  _trackData.trackInfo = trackInfo;
  _trackData.elevationProfileData = elevationInfo;
  if (self.onTrackRecordingProgressUpdate != nil)
    self.onTrackRecordingProgressUpdate();
}

- (void)handleActiveTrackSelectionPointChanged
{
  if (!self || !rawData().IsTrack())
    return;
  auto const & trackInfo = GetFramework().GetBookmarkManager().GetTrackSelectionInfo(rawData().GetTrackId());
  auto const latlon = mercator::ToLatLon(trackInfo.m_trackPoint);
  _locationCoordinate = CLLocationCoordinate2DMake(latlon.m_lat, latlon.m_lon);
  self.previewData = [[PlacePagePreviewData alloc] initWithRawData:rawData()];
}

- (void)dealloc
{
  if (self.mapNodeAttributes != nil)
    [[MWMStorage sharedStorage] removeObserver:self];
}

+ (BOOL)hasData
{
  return GetFramework().HasPlacePageInfo();
}

#pragma mark - Private

- (void)updateBookmarkStatus
{
  if (!GetFramework().HasPlacePageInfo())
    return;
  if (rawData().IsBookmark())
  {
    _bookmarkData = [[PlacePageBookmarkData alloc] initWithRawData:rawData()];
  }
  else if (rawData().IsTrack())
  {
    __weak auto weakSelf = self;
    _trackData =
        [[PlacePageTrackData alloc] initWithRawData:rawData()
                               onActivePointChanged:^(void) { [weakSelf handleActiveTrackSelectionPointChanged]; }];
  }
  else
  {
    _bookmarkData = nil;
  }
  _previewData = [[PlacePagePreviewData alloc] initWithRawData:rawData()];
  _objectType = [self objectTypeFromRawData];
  if (self.onBookmarkStatusUpdate != nil)
    self.onBookmarkStatusUpdate();
}

- (PlacePageObjectType)objectTypeFromRawData
{
  if (rawData().IsBookmark())
    return PlacePageObjectTypeBookmark;
  else if (rawData().IsTrack())
    return PlacePageObjectTypeTrack;
  else if (self.trackData)
    return PlacePageObjectTypeTrackRecording;
  else
    return PlacePageObjectTypePOI;
}

#pragma mark - MWMStorageObserver

- (void)processCountryEvent:(NSString *)countryId
{
  if ([countryId isEqualToString:self.mapNodeAttributes.countryId])
  {
    _mapNodeAttributes = [[MWMStorage sharedStorage] attributesForCountry:countryId];
    if (self.onMapNodeStatusUpdate != nil)
      self.onMapNodeStatusUpdate();
  }
}

- (void)processCountry:(NSString *)countryId downloadedBytes:(uint64_t)downloadedBytes totalBytes:(uint64_t)totalBytes
{
  if ([countryId isEqualToString:self.mapNodeAttributes.countryId] && self.onMapNodeProgressUpdate != nil)
    self.onMapNodeProgressUpdate(downloadedBytes, totalBytes);
}

@end
