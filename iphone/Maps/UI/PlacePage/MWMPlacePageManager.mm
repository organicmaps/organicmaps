#import "MWMPlacePageManager.h"
#import "CLLocation+Mercator.h"
#import "MWMActivityViewController.h"
#import "MWMFacilitiesController.h"
#import "MWMFrameworkListener.h"
#import "MWMFrameworkStorageObserver.h"
#import "MWMLocationHelpers.h"
#import "MWMLocationObserver.h"
#import "MWMPlacePageData.h"
#import "MWMPlacePageLayout.h"
#import "MWMRoutePoint+CPP.h"
#import "MWMSearchManager+Filter.h"
#import "MWMStorage+UI.h"
#import "SwiftBridge.h"
#import "MWMMapViewControlsManager+AddPlace.h"
#import "location_util.h"

#import <CoreApi/CoreApi.h>

#include "map/utils.hpp"

#include "platform/downloader_defines.hpp"

#include "geometry/distance_on_sphere.hpp"

using namespace storage;

namespace
{
void logSponsoredEvent(PlacePageData *data, NSString *eventName)
{
  NSMutableDictionary *stat = [NSMutableDictionary dictionary];
  if (data.hotelBooking != nil)
  {
    stat[kStatProvider] = kStatBooking;
    stat[kStatHotel] = data.hotelBooking.hotelId;
    stat[kStatHotelLocation] = makeLocationEventValue(data.locationCoordinate.latitude,
                                                      data.locationCoordinate.longitude);
  }
  else if (data.isPartner)
  {
    stat[kStatProvider] = data.partnerName;
    stat[kStatObjectLat] = @(data.locationCoordinate.latitude);
    stat[kStatObjectLon] = @(data.locationCoordinate.longitude);
  }
  else
  {
    stat[kStatProvider] = kStatPlacePageHotelSearch;
    stat[kStatHotelLocation] = makeLocationEventValue(data.locationCoordinate.latitude,
                                                      data.locationCoordinate.longitude);
  }

  [Statistics logEvent:eventName withParameters:stat atLocation:[MWMLocationManager lastLocation]];
}

void logSponsoredEvent(MWMPlacePageData * data, NSString * eventName)
{
  auto const & latLon = data.latLon;

  NSMutableDictionary * stat = [@{} mutableCopy];
  if (data.isBooking)
  {
    stat[kStatProvider] = kStatBooking;
    stat[kStatHotel] = data.sponsoredId;
    stat[kStatHotelLocation] = makeLocationEventValue(latLon.m_lat, latLon.m_lon);
  }
  else if (data.isOpentable)
  {
    stat[kStatProvider] = kStatOpentable;
    stat[kStatRestaurant] = data.sponsoredId;
    stat[kStatRestaurantLocation] = makeLocationEventValue(latLon.m_lat, latLon.m_lon);
  }
  else if (data.isPartner)
  {
    stat[kStatProvider] = data.partnerName;
    stat[kStatCategory] = @(data.ratingRawValue);
    stat[kStatObjectLat] = @(latLon.m_lat);
    stat[kStatObjectLon] = @(latLon.m_lon);
  }
  else
  {
    stat[kStatProvider] = kStatPlacePageHotelSearch;
    stat[kStatHotelLocation] = makeLocationEventValue(latLon.m_lat, latLon.m_lon);
  }

  [Statistics logEvent:eventName withParameters:stat atLocation:[MWMLocationManager lastLocation]];
}

void RegisterEventIfPossible(eye::MapObject::Event::Type const type)
{
  auto const userPos = GetFramework().GetCurrentPosition();
  auto const & info = GetFramework().GetCurrentPlacePageInfo();
  utils::RegisterEyeEventIfPossible(type, userPos, info);
}
}  // namespace

@interface MWMPlacePageManager ()<MWMFrameworkStorageObserver, MWMPlacePageLayoutDelegate,
                                  MWMPlacePageLayoutDataSource, MWMLocationObserver,
                                  MWMBookmarksObserver, MWMGCReviewSaver>

@property(nonatomic) MWMPlacePageLayout * layout;
@property(nonatomic) MWMPlacePageData * data;

@property(nonatomic) storage::NodeStatus currentDownloaderStatus;

@property(nonatomic) BOOL isSponsoredOpenLogged;

@end

@implementation MWMPlacePageManager

- (void)showReview {
  [self show];
  [self showUGCAddReview:UgcSummaryRatingTypeNone
              fromSource:MWMUGCReviewSourceNotification];
}

- (void)show {
  self.isSponsoredOpenLogged = NO;
  self.currentDownloaderStatus = storage::NodeStatus::Undefined;
  [MWMFrameworkListener addObserver:self];

  self.data = [[MWMPlacePageData alloc] init];
  [self.data fillSections];

  if (!self.layout)
  {
    self.layout = [[MWMPlacePageLayout alloc] initWithOwnerView:self.ownerViewController.view
                                                       delegate:self
                                                     dataSource:self];
  }

  [MWMLocationManager addObserver:self];
  [[MWMBookmarksManager sharedManager] addObserver:self];

  [self setupSpeedAndDistance];

  [self.layout showWithData:self.data];
  
  // Call for the first time to produce changes
  [self processCountryEvent:@([self.data countryId].c_str())];
}

- (void)update {
  if (!self.isPPShown)
    return;

  self.data = [[MWMPlacePageData alloc] init];
  [self.data fillSections];
  [self setupSpeedAndDistance];
  [self.layout updateWithData:self.data];
}

- (BOOL)isPPShown {
  return self.data != nil;
}

- (void)dismiss
{
  [self.layout close];
  self.data = nil;
  [[MWMBookmarksManager sharedManager] removeObserver:self];
  [MWMLocationManager removeObserver:self];
  [MWMFrameworkListener removeObserver:self];
}

#pragma mark - MWMBookmarksObserver

- (void)onBookmarkDeleted:(MWMMarkID)bookmarkId
{
  auto data = self.data;
  NSAssert(data && self.layout, @"It must be openned place page!");
  if (!data.isBookmark || data.bookmarkId != bookmarkId)
    return;

  [self closePlacePage];
}

- (void)onBookmarksCategoryDeleted:(MWMMarkGroupID)groupId
{
  auto data = self.data;
  NSAssert(data && self.layout, @"It must be openned place page!");
  if (!data.isBookmark || data.bookmarkCategoryId != groupId)
    return;

  [self closePlacePage];
}

#pragma mark - MWMPlacePageLayoutDataSource

- (void)downloadSelectedArea
{
  auto data = self.data;
  if (!data)
    return;
  auto const & countryId = [data countryId];
  NodeAttrs nodeAttrs;
  GetFramework().GetStorage().GetNodeAttrs(countryId, nodeAttrs);
  switch (nodeAttrs.m_status)
  {
  case NodeStatus::NotDownloaded:
  case NodeStatus::Partly: [MWMStorage downloadNode:@(countryId.c_str())]; break;
  case NodeStatus::Undefined:
  case NodeStatus::Error: [MWMStorage retryDownloadNode:@(countryId.c_str())]; break;
  case NodeStatus::OnDiskOutOfDate: [MWMStorage updateNode:@(countryId.c_str())]; break;
  case NodeStatus::Downloading:
  case NodeStatus::Applying:
  case NodeStatus::InQueue: [MWMStorage cancelDownloadNode:@(countryId.c_str())]; break;
  case NodeStatus::OnDisk: break;
  }
}

- (NSString *)distanceToObject
{
  CLLocation * lastLocation = [MWMLocationManager lastLocation];
  auto data = self.data;
  if (!lastLocation || !data)
    return @"";
  std::string distance;
  CLLocationCoordinate2D const & coord = lastLocation.coordinate;
  ms::LatLon const & target = data.latLon;
  double meters = ms::DistanceOnEarth(coord.latitude, coord.longitude, target.m_lat, target.m_lon);
  if (meters < 0.)
    return nil;
  
  auto units = measurement_utils::Units::Metric;
  settings::TryGet(settings::kMeasurementUnits, units);
  
  std::string s;
  switch (units) {
    case measurement_utils::Units::Imperial:
      measurement_utils::FormatDistanceWithLocalization(meters,
                                                        distance,
                                                        [[@" " stringByAppendingString:L(@"mile")] UTF8String],
                                                        [[@" " stringByAppendingString:L(@"foot")] UTF8String]);
    case measurement_utils::Units::Metric:
      measurement_utils::FormatDistanceWithLocalization(meters,
                                                        distance,
                                                        [[@" " stringByAppendingString:L(@"kilometer")] UTF8String],
                                                        [[@" " stringByAppendingString:L(@"meter")] UTF8String]);
  }
  return @(distance.c_str());
}

#pragma mark - MWMFrameworkStorageObserver

- (void)processCountryEvent:(NSString *)countryId
{
  auto data = self.data;
  if (!data || [data countryId] != countryId.UTF8String)
    return;

  if ([data countryId] == kInvalidCountryId)
  {
    [self.layout processDownloaderEventWithStatus:storage::NodeStatus::Undefined progress:0];
    return;
  }

  NodeStatuses statuses;
  GetFramework().GetStorage().GetNodeStatuses([data countryId], statuses);

  auto const status = statuses.m_status;
  if (status == self.currentDownloaderStatus)
    return;

  self.currentDownloaderStatus = status;
  [self.layout processDownloaderEventWithStatus:status progress:0];
}

- (void)processCountry:(NSString *)countryId
       downloadedBytes:(uint64_t)downloadedBytes
            totalBytes:(uint64_t)totalBytes
{
  auto data = self.data;
  if (!data || countryId.UTF8String == kInvalidCountryId || [data countryId] != countryId.UTF8String)
    return;

  [self.layout
      processDownloaderEventWithStatus:storage::NodeStatus::Downloading
                              progress:static_cast<CGFloat>(downloadedBytes) / totalBytes];
}

#pragma mark - MWMPlacePageLayoutDelegate

- (void)onPlacePageTopBoundChanged:(CGFloat)bound
{
  [self.ownerViewController setPlacePageTopBound:bound];
  [self.layout checkCellsVisible];
}

- (void)destroyLayout { self.layout = nil; }
- (void)closePlacePage { GetFramework().DeactivateMapSelection(true); }
- (BOOL)isPreviewPlus { return self.data.isPreviewPlus; }
- (void)onExpanded
{
  if (self.isSponsoredOpenLogged)
    return;
  self.isSponsoredOpenLogged = YES;
  auto data = self.data;
  if (!data)
    return;
  NSMutableDictionary * parameters = [@{} mutableCopy];
  if (data.isBooking)
    parameters[kStatProvider] = kStatBooking;
  else if (data.isPartner)
    parameters[kStatProvider] = data.partnerName;
  else if (data.isHolidayObject)
    parameters[kStatProvider] = kStatHoliday;
  else if (data.isPromoCatalog)
    parameters[kStatProvider] = kStatMapsmeGuides;

  parameters[kStatConnection] = [Statistics connectionTypeString];
  parameters[kStatTags] = data.statisticsTags;
  [Statistics logEvent:kStatPlacepageSponsoredOpen withParameters:parameters];
}

- (void)logStateChangeEventWithValue:(NSNumber *)value {
  MWMPlacePageData * data = self.data;
  if (data == nil) return;
  
  NSString *types = data.statisticsTags;
  NSNumber *lat = [NSNumber numberWithFloat:data.latLon.m_lat];
  NSNumber *lon = [NSNumber numberWithFloat:data.latLon.m_lon];
  [Statistics logEvent:kStatPlacePageChangeState withParameters:@{kStatTypes: types,
                                                                  kStatLat: lat,
                                                                  kStatLon: lon,
                                                                  kStatValue: value}];
}

#pragma mark - MWMLocationObserver

- (void)onHeadingUpdate:(CLHeading *)heading
{
  auto lastLocation = [MWMLocationManager lastLocation];
  auto data = self.data;
  if (!lastLocation || !data)
    return;

  auto const locationMercator = lastLocation.mercator;
  auto const dataMercator = data.mercator;
  if (base::AlmostEqualAbs(locationMercator, dataMercator, 1e-10))
    return;

  auto const info = location_util::compassInfoFromHeading(heading);
  auto const angle = ang::AngleTo(locationMercator, dataMercator) + info.m_bearing;
  [self.layout rotateDirectionArrowToAngle:angle];
}

- (void)setupSpeedAndDistance
{
  [self.layout setDistanceToObject:self.distanceToObject];
  if (self.data.isMyPosition)
    [self.layout setSpeedAndAltitude:location_helpers::formattedSpeedAndAltitude(MWMLocationManager.lastLocation)];
}

- (void)onLocationUpdate:(CLLocation *)location
{
  [self setupSpeedAndDistance];
}

- (void)mwm_refreshUI { [self.layout mwm_refreshUI]; }

- (void)routeFrom:(PlacePageData *)data {
  [Statistics logEvent:kStatEventName(kStatPlacePage, kStatBuildRoute)
        withParameters:@{kStatValue : kStatSource}];

  MWMRoutePoint *point = [self routePoint:data withType:MWMRoutePointTypeStart intermediateIndex:0];
  [MWMRouter buildFromPoint:point bestRouter:YES];
  [self closePlacePage];
}

- (void)routeFrom
{
  [Statistics logEvent:kStatEventName(kStatPlacePage, kStatBuildRoute)
        withParameters:@{kStatValue : kStatSource}];

  MWMRoutePoint * point = [self routePointWithType:MWMRoutePointTypeStart intermediateIndex:0];
  [MWMRouter buildFromPoint:point bestRouter:YES];
  [self closePlacePage];
}

- (void)routeTo:(PlacePageData *)data {
  [Statistics logEvent:kStatEventName(kStatPlacePage, kStatBuildRoute)
        withParameters:@{kStatValue : kStatDestination}];

  if ([MWMRouter isOnRoute]) {
    [MWMRouter stopRouting];
  }

  if ([MWMTrafficManager transitEnabled]) {
    [MWMRouter setType:MWMRouterTypePublicTransport];
  }

  MWMRoutePoint *point = [self routePoint:data withType:MWMRoutePointTypeFinish intermediateIndex:0];
  [MWMRouter buildToPoint:point bestRouter:YES];
  [self closePlacePage];
}

- (void)routeTo
{
  [Statistics logEvent:kStatEventName(kStatPlacePage, kStatBuildRoute)
        withParameters:@{kStatValue : kStatDestination}];

  if ([MWMRouter isOnRoute])
    [MWMRouter stopRouting];

  if ([MWMTrafficManager transitEnabled])
    [MWMRouter setType:MWMRouterTypePublicTransport];

  MWMRoutePoint * point = [self routePointWithType:MWMRoutePointTypeFinish intermediateIndex:0];
  [MWMRouter buildToPoint:point bestRouter:YES];
  [self closePlacePage];
}

- (void)routeAddStop:(PlacePageData *)data {
  MWMRoutePoint *point = [self routePoint:data withType:MWMRoutePointTypeIntermediate intermediateIndex:0];
  [MWMRouter addPointAndRebuild:point];
  [self closePlacePage];
}

- (void)routeAddStop
{
  MWMRoutePoint * point =
      [self routePointWithType:MWMRoutePointTypeIntermediate intermediateIndex:0];
  [MWMRouter addPointAndRebuild:point];
  [self closePlacePage];
}

- (void)routeRemoveStop:(PlacePageData *)data {
  MWMRoutePoint *point = nil;
  auto const intermediateIndex = GetFramework().GetCurrentPlacePageInfo().GetIntermediateIndex();
  switch (GetFramework().GetCurrentPlacePageInfo().GetRouteMarkType()) {
  case RouteMarkType::Start:
    point = [self routePoint:data withType:MWMRoutePointTypeStart intermediateIndex:intermediateIndex];
    break;
  case RouteMarkType::Finish:
    point = [self routePoint:data withType:MWMRoutePointTypeFinish intermediateIndex:intermediateIndex];
    break;
  case RouteMarkType::Intermediate:
    point = [self routePoint:data withType:MWMRoutePointTypeIntermediate intermediateIndex:intermediateIndex];
    break;
  }
  [MWMRouter removePointAndRebuild:point];
  [self closePlacePage];
}

- (void)routeRemoveStop
{
  MWMRoutePoint * point = nil;
  auto const intermediateIndex = GetFramework().GetCurrentPlacePageInfo().GetIntermediateIndex();
  switch (GetFramework().GetCurrentPlacePageInfo().GetRouteMarkType())
  {
  case RouteMarkType::Start:
    point =
        [self routePointWithType:MWMRoutePointTypeStart intermediateIndex:intermediateIndex];
    break;
  case RouteMarkType::Finish:
    point =
        [self routePointWithType:MWMRoutePointTypeFinish intermediateIndex:intermediateIndex];
    break;
  case RouteMarkType::Intermediate:
    point = [self routePointWithType:MWMRoutePointTypeIntermediate
                   intermediateIndex:intermediateIndex];
    break;
  }
  [MWMRouter removePointAndRebuild:point];
  [self closePlacePage];
}

- (void)orderTaxi:(PlacePageData *)data
{
  NSString * providerString = nil;
  switch (data.taxiProvider)
  {
  case PlacePageTaxiProviderNone: providerString = kStatUnknown; break;
  case PlacePageTaxiProviderUber: providerString = kStatUber; break;
  case PlacePageTaxiProviderYandex: providerString = kStatYandex; break;
  case PlacePageTaxiProviderMaxim: providerString = kStatMaxim; break;
  case PlacePageTaxiProviderRutaxi: providerString = kStatVezet; break;
  }

  [Statistics logEvent:kStatPlacePageTaxiClick
        withParameters:@{kStatProvider : providerString, kStatTags : data.statisticsTags}];
  [MWMRouter setType:MWMRouterTypeTaxi];
  MWMRoutePoint * point = [self routePointWithData:data pointType:MWMRoutePointTypeFinish intermediateIndex:0];
  [MWMRouter buildToPoint:point bestRouter:NO];
  [self closePlacePage];
}

- (MWMRoutePoint *)routePointWithData:(PlacePageData *)data
                                 pointType:(MWMRoutePointType)type
                    intermediateIndex:(size_t)intermediateIndex
{
  if (data.isMyPosition) {
    return [[MWMRoutePoint alloc] initWithLastLocationAndType:type intermediateIndex:intermediateIndex];
  }

  NSString *title = nil;
  if (data.previewData.title.length > 0) {
    title = data.previewData.title;
  } else if (data.previewData.address.length > 0) {
    title = data.previewData.address;
  } else if (data.previewData.subtitle.length > 0) {
    title = data.previewData.subtitle;
  } else if (data.bookmarkData != nil) {
    title = data.bookmarkData.externalTitle;
  } else {
    title = L(@"core_placepage_unknown_place");
  }

  NSString * subtitle = nil;
  if (data.previewData.subtitle.length > 0 && ![title isEqualToString:data.previewData.subtitle]) {
    subtitle = data.previewData.subtitle;
  }

  return [[MWMRoutePoint alloc] initWithPoint:location_helpers::ToMercator(data.locationCoordinate)
                                        title:title
                                     subtitle:subtitle
                                         type:type
                            intermediateIndex:intermediateIndex];
}

- (MWMRoutePoint *)routePoint:(PlacePageData *)data
                     withType:(MWMRoutePointType)type
                    intermediateIndex:(size_t)intermediateIndex
{
  if (data.isMyPosition) {
    return [[MWMRoutePoint alloc] initWithLastLocationAndType:type
                                            intermediateIndex:intermediateIndex];
  }

  NSString *title = nil;
  if (data.previewData.title.length > 0) {
    title = data.previewData.title;
  } else if (data.previewData.address.length > 0) {
    title = data.previewData.address;
  } else if (data.previewData.subtitle.length > 0) {
    title = data.previewData.subtitle;
  } else if (data.bookmarkData != nil) {
    title = data.bookmarkData.externalTitle;
  } else {
    title = L(@"core_placepage_unknown_place");
  }

  NSString * subtitle = nil;
  if (data.previewData.subtitle.length > 0 && ![title isEqualToString:data.previewData.subtitle]) {
    subtitle = data.previewData.subtitle;
  }

  return [[MWMRoutePoint alloc] initWithPoint:location_helpers::ToMercator(data.locationCoordinate)
                                        title:title
                                     subtitle:subtitle
                                         type:type
                            intermediateIndex:intermediateIndex];
}

- (MWMRoutePoint *)routePointWithType:(MWMRoutePointType)type
                    intermediateIndex:(size_t)intermediateIndex
{
  auto data = self.data;
  if (!data)
    return nil;

  if (data.isMyPosition)
    return [[MWMRoutePoint alloc] initWithLastLocationAndType:type
                                            intermediateIndex:intermediateIndex];

  NSString * title = nil;
  if (data.title.length > 0)
    title = data.title;
  else if (data.address.length > 0)
    title = data.address;
  else if (data.subtitle.length > 0)
    title = data.subtitle;
  else if (data.isBookmark)
    title = data.externalTitle;
  else
    title = L(@"core_placepage_unknown_place");

  NSString * subtitle = nil;
  if (data.subtitle.length > 0 && ![title isEqualToString:data.subtitle])
    subtitle = data.subtitle;

  return [[MWMRoutePoint alloc] initWithPoint:data.mercator
                                        title:title
                                     subtitle:subtitle
                                         type:type
                            intermediateIndex:intermediateIndex];
}


- (void)share:(PlacePageData *)data {
  [Statistics logEvent:kStatEventName(kStatPlacePage, kStatShare)];
  MWMActivityViewController * shareVC = [MWMActivityViewController shareControllerForPlacePage:data];
  [shareVC presentInParentViewController:self.ownerViewController anchorView:self.layout.shareAnchor];
}

- (void)share
{
  auto data = self.data;
  if (!data)
    return;
  [Statistics logEvent:kStatEventName(kStatPlacePage, kStatShare)];
  MWMActivityViewController * shareVC = [MWMActivityViewController
      shareControllerForPlacePageObject:static_cast<id<MWMPlacePageObject>>(data)];
  [shareVC presentInParentViewController:self.ownerViewController
                              anchorView:self.layout.shareAnchor];
}

- (void)editPlace
{
  [Statistics logEvent:kStatEventName(kStatPlacePage, kStatEdit)];
  GetPlatform().GetMarketingService().SendPushWooshTag(marketing::kEditorEditDiscovered);
  [self.ownerViewController openEditor];
}

- (void)addBusiness
{
  [Statistics logEvent:kStatEditorAddClick withParameters:@{kStatValue : kStatPlacePage}];
  [[MWMMapViewControlsManager manager] addPlace:YES hasPoint:NO point:m2::PointD()];
}

- (void)addPlace
{
  auto data = self.data;
  if (!data)
    return;
  [Statistics logEvent:kStatEditorAddClick
        withParameters:@{kStatValue : kStatPlacePageNonBuilding}];
  [[MWMMapViewControlsManager manager] addPlace:NO hasPoint:YES point:data.mercator];
}

- (void)addPlace:(CLLocationCoordinate2D)coordinate
{
  [Statistics logEvent:kStatEditorAddClick
        withParameters:@{kStatValue : kStatPlacePageNonBuilding}];
  [[MWMMapViewControlsManager manager] addPlace:NO hasPoint:YES point:location_helpers::ToMercator(coordinate)];
}

- (void)addBookmark:(PlacePageData *)data {
  RegisterEventIfPossible(eye::MapObject::Event::Type::AddToBookmark);
  [Statistics logEvent:kStatBookmarkCreated];

  auto &f = GetFramework();
  auto &bmManager = f.GetBookmarkManager();
  auto &info = f.GetCurrentPlacePageInfo();
  auto const categoryId = f.LastEditedBMCategory();
  kml::BookmarkData bmData;
  bmData.m_name = info.FormatNewBookmarkName();
  bmData.m_color.m_predefinedColor = f.LastEditedBMColor();
  bmData.m_point = info.GetMercator();
  if (info.IsFeature()) {
    SaveFeatureTypes(info.GetTypes(), bmData);
  }
  auto editSession = bmManager.GetEditSession();
  auto const *bookmark = editSession.CreateBookmark(std::move(bmData), categoryId);

  auto buildInfo = info.GetBuildInfo();
  buildInfo.m_match = place_page::BuildInfo::Match::Everything;
  buildInfo.m_userMarkId = bookmark->GetId();
  f.UpdatePlacePageInfoForCurrentSelection(buildInfo);
}

- (void)addBookmark
{
  auto data = self.data;
  if (!data)
    return;
  RegisterEventIfPossible(eye::MapObject::Event::Type::AddToBookmark);
  [Statistics logEvent:kStatBookmarkCreated];
  [data updateBookmarkStatus:YES];
  [self.layout reloadBookmarkSection:YES];
}

- (void)removeBookmark:(PlacePageData *)data
{
  [Statistics logEvent:kStatEventName(kStatPlacePage, kStatBookmarks)
        withParameters:@{kStatValue : kStatRemove}];

  auto &f = GetFramework();
  f.GetBookmarkManager().GetEditSession().DeleteBookmark(data.bookmarkData.bookmarkId);

  auto buildInfo = f.GetCurrentPlacePageInfo().GetBuildInfo();
  buildInfo.m_match = place_page::BuildInfo::Match::FeatureOnly;
  buildInfo.m_userMarkId = kml::kInvalidMarkId;
  f.UpdatePlacePageInfoForCurrentSelection(buildInfo);
}

- (void)removeBookmark
{
  auto data = self.data;
  if (!data)
    return;
  [Statistics logEvent:kStatEventName(kStatPlacePage, kStatBookmarks)
        withParameters:@{kStatValue : kStatRemove}];
  [data updateBookmarkStatus:NO];
  [self.layout reloadBookmarkSection:NO];
}

- (void)call:(PlacePageData *)data {
  if (data.infoData.phoneUrl && [UIApplication.sharedApplication canOpenURL:data.infoData.phoneUrl]) {
    [UIApplication.sharedApplication openURL:data.infoData.phoneUrl options:@{} completionHandler:nil];
  }
}

- (void)call
{
  MWMPlacePageData *data = self.data;
  if (!data) return;
  NSString *filteredDigits = [[data.phoneNumber componentsSeparatedByCharactersInSet:
                               [[NSCharacterSet decimalDigitCharacterSet] invertedSet]]
                              componentsJoinedByString:@""];
  NSString *resultNumber = [data.phoneNumber hasPrefix:@"+"] ? [NSString stringWithFormat:@"+%@", filteredDigits] : filteredDigits;
  NSURL *phoneURL = [NSURL URLWithString:[NSString stringWithFormat:@"tel://%@", resultNumber]];
  if ([UIApplication.sharedApplication canOpenURL:phoneURL]) {
    [UIApplication.sharedApplication openURL:phoneURL options:@{} completionHandler:nil];
  }
}

- (void)editBookmark {
  [self.ownerViewController openBookmarkEditor];
}

- (void)showPlaceDescription:(NSString *)htmlString
{
  [self.ownerViewController openFullPlaceDescriptionWithHtml:htmlString];
}

- (void)openPartner:(PlacePageData *)data withStatisticLog:(NSString *)eventName proposedUrl:(NSURL *)proposedUrl
{
  logSponsoredEvent(data, eventName);

  NSURL *deeplink = [NSURL URLWithString:data.sponsoredDeeplink];
  if (deeplink != nil && [UIApplication.sharedApplication canOpenURL:deeplink]) {
    [UIApplication.sharedApplication openURL:deeplink options:@{} completionHandler:nil];
  } else if (proposedUrl != nil) {
    [UIApplication.sharedApplication openURL:proposedUrl options:@{} completionHandler:nil];
  } else {
    NSAssert(proposedUrl, @"Sponsored url can't be nil!");
    return;
  }

  if (data.previewData.isBookingPlace) {
    auto mercator = location_helpers::ToMercator(data.locationCoordinate);
    [MWMEye transitionToBookingWithPos:CGPointMake(mercator.x, mercator.y)];
  }
}

- (void)openPartnerWithStatisticLog:(NSString *)eventName proposedUrl:(NSURL *)proposedUrl
{
  auto data = self.data;
  if (!data)
    return;
  
  logSponsoredEvent(data, eventName);
  
  NSURL * url = data.isPartnerAppInstalled ? data.deepLink : proposedUrl;
  NSAssert(url, @"Sponsored url can't be nil!");
  
  [UIApplication.sharedApplication openURL:url options:@{} completionHandler:nil];

  if (data.isBooking)
    [MWMEye transitionToBookingWithPos:CGPointMake(data.mercator.x, data.mercator.y)];
}

- (void)book:(PlacePageData *)data {
  NSURL *url = [NSURL URLWithString:data.sponsoredURL];
  [self openPartner:data withStatisticLog:kStatPlacePageHotelBook proposedUrl:url];
}

- (void)book
{
  auto data = self.data;
  if (!data)
    return;
  NSString * eventName = nil;
  if (data.isBooking)
  {
    eventName = kStatPlacePageHotelBook;
  }
  else if (data.isOpentable)
  {
    eventName = kStatPlacePageRestaurantBook;
  }
  else
  {
    NSAssert(false, @"Invalid book case!");
    return;
  }
  
  [self openPartnerWithStatisticLog:eventName proposedUrl:data.sponsoredURL];
}

- (void)openDescriptionUrl:(PlacePageData *)data {
  NSURL *url = [NSURL URLWithString:data.sponsoredDescriptionURL];
  [self openPartner:data withStatisticLog:kStatPlacePageHotelDetails proposedUrl:url];
}

- (void)openDescriptionUrl
{
  auto data = self.data;
  if (!data)
    return;

 [self openPartnerWithStatisticLog:kStatPlacePageHotelDetails
                       proposedUrl:data.sponsoredDescriptionURL];
}

- (void)openMoreUrl:(PlacePageData *)data {
  NSURL *url = [NSURL URLWithString:data.sponsoredMoreURL];
  if (!url) { return; }
  logSponsoredEvent(data, kStatPlacePageHotelMore);
  [UIApplication.sharedApplication openURL:url options:@{} completionHandler:nil];
  auto mercator = location_helpers::ToMercator(data.locationCoordinate);
  [MWMEye transitionToBookingWithPos:CGPointMake(mercator.x, mercator.y)];
}

- (void)openMoreUrl
{
  auto data = self.data;
  if (!data)
    return;

  logSponsoredEvent(data, kStatPlacePageHotelMore);
  [UIApplication.sharedApplication openURL:data.sponsoredMoreURL
                                   options:@{} completionHandler:nil];
  [MWMEye transitionToBookingWithPos:CGPointMake(data.mercator.x, data.mercator.y)];
}

- (void)openReviewUrl:(PlacePageData *)data {
  NSURL *url = [NSURL URLWithString:data.sponsoredReviewURL];
  [self openPartner:data withStatisticLog:kStatPlacePageHotelReviews proposedUrl:url];
}

- (void)openReviewUrl
{
  auto data = self.data;
  if (!data)
    return;

  [self openPartnerWithStatisticLog:kStatPlacePageHotelReviews
                        proposedUrl:data.sponsoredReviewURL];
}

- (void)searchBookingHotels
{
  auto data = self.data;
  if (!data)
    return;
  
  logSponsoredEvent(data, kStatPlacePageHotelBook);
  NSURL * url = data.bookingSearchURL;
  NSAssert(url, @"Search url can't be nil!");
  [UIApplication.sharedApplication openURL:url options:@{} completionHandler:nil];
}

- (void)openPartner:(PlacePageData *)data
{
  logSponsoredEvent(data, kStatPlacePageSponsoredActionButtonClick);
  NSURL *url = [NSURL URLWithString:data.sponsoredURL];
  NSAssert(url, @"Partner url can't be nil!");
  [UIApplication.sharedApplication openURL:url options:@{} completionHandler:nil];
}

- (void)openPartner
{
  auto data = self.data;
  if (!data)
    return;
  logSponsoredEvent(data, kStatPlacePageSponsoredActionButtonClick);
  NSURL * url = data.sponsoredURL;
  NSAssert(url, @"Partner url can't be nil!");
  [UIApplication.sharedApplication openURL:url options:@{} completionHandler:nil];
}

- (void)avoidDirty {
  [Statistics logEvent:kStatPlacepageDrivingOptionsAction
        withParameters:@{kStatType : [kStatUnpaved capitalizedString]}];
  [MWMRouter avoidRoadTypeAndRebuild:MWMRoadTypeDirty];
  [self closePlacePage];
}


- (void)avoidFerry {
  [Statistics logEvent:kStatPlacepageDrivingOptionsAction
        withParameters:@{kStatType : [kStatFerry capitalizedString]}];
  [MWMRouter avoidRoadTypeAndRebuild:MWMRoadTypeFerry];
  [self closePlacePage];
}


- (void)avoidToll {
  [Statistics logEvent:kStatPlacepageDrivingOptionsAction
        withParameters:@{kStatType : [kStatToll capitalizedString]}];
  [MWMRouter avoidRoadTypeAndRebuild:MWMRoadTypeToll];
  [self closePlacePage];
}


- (void)showPhotoAtIndex:(NSInteger)index
                         referenceView:(UIView *)referenceView
    referenceViewWhenDismissingHandler:
        (MWMPlacePageButtonsDismissBlock)referenceViewWhenDismissingHandler
{
  auto data = self.data;
  if (!data)
    return;
  logSponsoredEvent(self.data, kStatPlacePageHotelGallery);
  auto galleryModel = [[MWMGalleryModel alloc] initWithTitle:self.hotelName items:data.photos];
  auto initialPhoto = galleryModel.items[index];
  auto photoVC = [[MWMPhotosViewController alloc] initWithPhotos:galleryModel
                                                    initialPhoto:initialPhoto
                                                   referenceView:referenceView];
  photoVC.referenceViewForPhotoWhenDismissingHandler = ^UIView *(MWMGalleryItemModel * photo) {
    return referenceViewWhenDismissingHandler([galleryModel.items indexOfObject:photo]);
  };

  [[MapViewController sharedController] presentViewController:photoVC animated:YES completion:nil];
}

- (void)showGallery
{
  auto data = self.data;
  if (!data)
    return;
  logSponsoredEvent(self.data, kStatPlacePageHotelGallery);
  auto galleryModel = [[MWMGalleryModel alloc] initWithTitle:self.hotelName items:data.photos];
  auto galleryVc = [MWMGalleryViewController instanceWithModel:galleryModel];
  [[MapViewController sharedController].navigationController pushViewController:galleryVc animated:YES];
}

- (void)showUGCAddReview:(PlacePageData *)data rating:(UgcSummaryRatingType)value fromSource:(MWMUGCReviewSource)source
{
  NSMutableArray *ratings = [NSMutableArray array];
  for (NSString *cat in data.ratingCategories) {
    [ratings addObject:[[MWMUGCRatingStars alloc] initWithTitle:cat
                                                          value:value
                                                       maxValue:5.0f]];
  }

  RegisterEventIfPossible(eye::MapObject::Event::Type::UgcEditorOpened);
  NSString * sourceString;
  switch (source) {
    case MWMUGCReviewSourcePlacePage:
      sourceString = kStatPlacePage;
      break;
    case MWMUGCReviewSourcePlacePagePreview:
      sourceString = kStatPlacePagePreview;
      break;
    case MWMUGCReviewSourceNotification:
      sourceString = kStatNotification;
      break;
  }
  [Statistics logEvent:kStatUGCReviewStart
        withParameters:@{
          kStatIsAuthenticated: @([MWMUser isAuthenticated]),
          kStatIsOnline: @(GetPlatform().ConnectionStatus() != Platform::EConnectionType::CONNECTION_NONE),
          kStatMode: kStatAdd,
          kStatFrom: sourceString
        }];
  auto ugcReviewModel =
      [[MWMUGCReviewModel alloc] initWithReviewValue:value ratings:ratings title:data.previewData.title text:@""];
  auto ugcVC = [MWMUGCAddReviewController instanceWithModel:ugcReviewModel saver:self];
  [[MapViewController sharedController].navigationController pushViewController:ugcVC animated:YES];
}

- (void)showUGCAddReview:(UgcSummaryRatingType)value fromSource:(MWMUGCReviewSource)source
{
  auto data = self.data;
  if (!data)
    return;

  NSMutableArray<MWMUGCRatingStars *> * ratings = [@[] mutableCopy];
  for (auto & cat : data.ugcRatingCategories)
    [ratings addObject:[[MWMUGCRatingStars alloc] initWithTitle:@(cat.c_str())
                                                          value:value
                                                       maxValue:5.0f]];
  auto title = data.title;

  RegisterEventIfPossible(eye::MapObject::Event::Type::UgcEditorOpened);
  NSString * sourceString;
  switch (source) {
    case MWMUGCReviewSourcePlacePage:
      sourceString = kStatPlacePage;
      break;
    case MWMUGCReviewSourcePlacePagePreview:
      sourceString = kStatPlacePagePreview;
      break;
    case MWMUGCReviewSourceNotification:
      sourceString = kStatNotification;
      break;
  }
  [Statistics logEvent:kStatUGCReviewStart
        withParameters:@{
          kStatIsAuthenticated: @([MWMUser isAuthenticated]),
          kStatIsOnline:
              @(GetPlatform().ConnectionStatus() != Platform::EConnectionType::CONNECTION_NONE),
          kStatMode: kStatAdd,
          kStatFrom: sourceString
        }];
  auto ugcReviewModel =
      [[MWMUGCReviewModel alloc] initWithReviewValue:value ratings:ratings title:title text:@""];
  auto ugcVC = [MWMUGCAddReviewController instanceWithModel:ugcReviewModel saver: self];
  [[MapViewController sharedController].navigationController pushViewController:ugcVC animated:YES];
}

- (void)searchSimilar
{
  [Statistics logEvent:@"Placepage_Hotel_search_similar"
        withParameters:@{kStatProvider : self.data.isBooking ? kStatBooking : kStatOSM}];

  MWMHotelParams * params = [[MWMHotelParams alloc] init];
  [[MWMSearchManager manager] showHotelFilterWithParams:params
                      onFinishCallback:^{
    [MWMMapViewControlsManager.manager searchTextOnMap:[L(@"booking_hotel") stringByAppendingString:@" "]
                                        forInputLocale:[NSLocale currentLocale].localeIdentifier];
  }];
}

- (void)showAllFacilities:(PlacePageData *)data {
  logSponsoredEvent(data, kStatPlacePageHotelFacilities);
  MWMFacilitiesController *vc = [[MWMFacilitiesController alloc] init];
  vc.name = data.previewData.title;
  vc.facilities = data.hotelBooking.facilities;
  [[MapViewController sharedController].navigationController pushViewController:vc animated:YES];
}

- (void)showAllFacilities
{
  NSAssert(false, @"deprecated");
}

- (void)openLocalAdsURL:(PlacePageData *)data
{
  NSURL *url = [NSURL URLWithString:data.infoData.localAdsUrl];
  if (!url)
    return;

  auto const & feature = GetFramework().GetCurrentPlacePageInfo().GetID();
  [Statistics logEvent:kStatPlacePageOwnershipButtonClick
        withParameters:@{
                         @"mwm_name" : @(feature.GetMwmName().c_str()),
                         @"mwm_version" : @(feature.GetMwmVersion()),
                         @"feature_id" : @(feature.m_index)
                         }
            atLocation:[MWMLocationManager lastLocation]];

  [self.ownerViewController openUrl:url];
}

- (void)openLocalAdsURL
{
  auto data = self.data;
  if (!data)
    return;
  auto url = [NSURL URLWithString:data.localAdsURL];
  if (!url)
    return;

  auto const & feature = [data featureId];
  [Statistics logEvent:kStatPlacePageOwnershipButtonClick
        withParameters:@{
                         @"mwm_name" : @(feature.GetMwmName().c_str()),
                         @"mwm_version" : @(feature.GetMwmVersion()),
                         @"feature_id" : @(feature.m_index)
                         }
            atLocation:[MWMLocationManager lastLocation]];

  [self.ownerViewController openUrl:url];
}

//- (void)openSponsoredURL:(nullable NSURL *)url
//{
//  if (auto u = url ?: self.data.sponsoredURL)
//    [self.ownerViewController openUrl:u];
//}

- (void)openReviews:(id<MWMReviewsViewModelProtocol> _Nonnull)reviewsViewModel
{
  auto reviewsVC = [[MWMReviewsViewController alloc] initWithViewModel:reviewsViewModel];
  [[MapViewController sharedController].navigationController pushViewController:reviewsVC animated:YES];
}

- (void)openWebsite:(PlacePageData *)data {
  NSURL *url = [NSURL URLWithString:data.infoData.website];
  if (url) {
    [self.ownerViewController openUrl:url];
    //TODO: add local ads events
  }
}

- (void)openCatalogSingleItem:(PlacePageData *)data atIndex:(NSInteger)index {
  [Statistics logEvent:kStatPlacepageSponsoredItemSelected
        withParameters:@{
                         kStatProvider: kStatMapsmeGuides,
                         kStatPlacement: data.isLargeToponim ? kStatPlacePageToponims : kStatPlacePageSightSeeing,
                         kStatItem: @(index),
                         kStatDestination: kStatCatalogue
                         }];
  NSURL *url = [NSURL URLWithString:data.catalogPromo.promoItems[index].catalogUrl];
  NSURL *patchedUrl = [[MWMBookmarksManager sharedManager] injectCatalogUTMContent:url content:MWMUTMContentView];
  [self openCatalogForURL:patchedUrl];
}

- (void)openCatalogMoreItems:(PlacePageData *)data {
  [self openCatalogForURL:data.catalogPromo.moreUrl];
  [Statistics logEvent:kStatPlacepageSponsoredMoreSelected
        withParameters:@{
                         kStatProvider: kStatMapsmeGuides,
                         kStatPlacement: data.isLargeToponim ? kStatPlacePageToponims : kStatPlacePageSightSeeing,
                         }];
}

- (void)showRemoveAds {
  [[MapViewController sharedController] showRemoveAds];
}

#pragma mark - AvailableArea / PlacePageArea

- (void)updateAvailableArea:(CGRect)frame
{
  auto data = self.data;
  if (data)
    [self.layout updateAvailableArea:frame];
}

#pragma mark - MWMFeatureHolder

- (FeatureID const &)featureId { return GetFramework().GetCurrentPlacePageInfo().GetID(); }
#pragma mark - MWMBookingInfoHolder

- (std::vector<booking::HotelFacility> const &)hotelFacilities { return self.data.facilities; }
- (NSString *)hotelName { return self.data.title; }

#pragma mark - Ownerfacilities

- (MapViewController *)ownerViewController { return [MapViewController sharedController]; }

- (void)saveUgcWithModel:(MWMUGCReviewModel *)model
                language:(NSString *)language
           resultHandler:(void (^)(BOOL))resultHandler {
  using namespace ugc;
  auto appInfo = AppInfo.sharedInfo;
  auto const locale =
      static_cast<uint8_t>(StringUtf8Multilang::GetLangIndex(appInfo.twoLetterLanguageId.UTF8String));
  std::vector<uint8_t> keyboardLanguages;
  // TODO: Set the list of used keyboard languages (not only the recent one).
  auto twoLetterInputLanguage = languages::Normalize(language.UTF8String);
  keyboardLanguages.emplace_back(StringUtf8Multilang::GetLangIndex(twoLetterInputLanguage));

  KeyboardText t{model.text.UTF8String, locale, keyboardLanguages};
  Ratings r;
  for (MWMUGCRatingStars * star in model.ratings)
    r.emplace_back(star.title.UTF8String, star.value);

  UGCUpdate update{r, t, std::chrono::system_clock::now()};

  place_page::Info const & info = GetFramework().GetCurrentPlacePageInfo();
  GetFramework().GetUGCApi()->SetUGCUpdate(info.GetID(), update,
  [resultHandler, info](ugc::Storage::SettingResult const result)
  {
    if (result != ugc::Storage::SettingResult::Success)
    {
      resultHandler(NO);
      return;
    }

    resultHandler(YES);
    GetFramework().UpdatePlacePageInfoForCurrentSelection();

    utils::RegisterEyeEventIfPossible(eye::MapObject::Event::Type::UgcSaved,
                                      GetFramework().GetCurrentPosition(), info);
  });
}

#pragma mark - MWMPlacePagePromoProtocol

- (void)openCatalogForURL:(NSURL *)url {
  // NOTE: UTM is already into URL, core part does it for Placepage Gallery.
  MWMCatalogWebViewController *catalog = [MWMCatalogWebViewController catalogFromAbsoluteUrl:url utm:MWMUTMNone];
  NSMutableArray<UIViewController *> * controllers = [self.ownerViewController.navigationController.viewControllers mutableCopy];
  [controllers addObjectsFromArray:@[catalog]];
  [self.ownerViewController.navigationController setViewControllers:controllers animated:YES];
}

@end
