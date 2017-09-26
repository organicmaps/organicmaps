#import "MWMPlacePageManager.h"
#import "CLLocation+Mercator.h"
#import "MapViewController.h"
#import "MWMAPIBar.h"
#import "MWMActivityViewController.h"
#import "MWMFrameworkListener.h"
#import "MWMFrameworkObservers.h"
#import "MWMLocationHelpers.h"
#import "MWMLocationManager.h"
#import "MWMLocationObserver.h"
#import "MWMPlacePageData.h"
#import "MWMPlacePageLayout.h"
#import "MWMRoutePoint+CPP.h"
#import "MWMRouter.h"
#import "MWMStorage.h"
#import "MWMUGCReviewController.h"
#import "MWMUGCReviewVM.h"
#import "Statistics.h"

#include "Framework.h"

#include "map/bookmark.hpp"

#include "geometry/distance_on_sphere.hpp"

extern NSString * const kBookmarkDeletedNotification;
extern NSString * const kBookmarkCategoryDeletedNotification;

namespace
{
void logSponsoredEvent(MWMPlacePageData * data, NSString * eventName)
{
  auto const & latLon = data.latLon;

  NSMutableDictionary * stat = [@{} mutableCopy];
  if (data.isBooking)
  {
    stat[kStatProvider] = kStatBooking;
    stat[kStatHotel] = data.sponsoredId;
    stat[kStatHotelLocation] = makeLocationEventValue(latLon.lat, latLon.lon);
  }
  else if (data.isOpentable)
  {
    stat[kStatProvider] = kStatOpentable;
    stat[kStatRestaurant] = data.sponsoredId;
    stat[kStatRestaurantLocation] = makeLocationEventValue(latLon.lat, latLon.lon);
  }
  else
  {
    stat[kStatProvider] = kStatPlacePageHotelSearch;
    stat[kStatHotelLocation] = makeLocationEventValue(latLon.lat, latLon.lon);
  }

  [Statistics logEvent:eventName withParameters:stat atLocation:[MWMLocationManager lastLocation]];
}
}  // namespace

@interface MWMPlacePageManager ()<MWMFrameworkStorageObserver, MWMPlacePageLayoutDelegate,
                                  MWMPlacePageLayoutDataSource, MWMLocationObserver>

@property(nonatomic) MWMPlacePageLayout * layout;
@property(nonatomic) MWMPlacePageData * data;

@property(nonatomic) storage::NodeStatus currentDownloaderStatus;

@property(nonatomic) BOOL isSponsoredOpenLogged;

@end

@implementation MWMPlacePageManager

- (void)show:(place_page::Info const &)info
{
  self.isSponsoredOpenLogged = NO;
  self.currentDownloaderStatus = storage::NodeStatus::Undefined;
  [MWMFrameworkListener addObserver:self];

  self.data = [[MWMPlacePageData alloc] initWithPlacePageInfo:info];
  [self.data fillSections];

  if (!self.layout)
  {
    self.layout = [[MWMPlacePageLayout alloc] initWithOwnerView:self.ownerViewController.view
                                                       delegate:self
                                                     dataSource:self];
  }

  [MWMLocationManager addObserver:self];
  [NSNotificationCenter.defaultCenter addObserver:self
                                         selector:@selector(handleBookmarkDeleting:)
                                             name:kBookmarkDeletedNotification
                                           object:nil];

  [NSNotificationCenter.defaultCenter addObserver:self
                                         selector:@selector(handleBookmarkCategoryDeleting:)
                                             name:kBookmarkCategoryDeletedNotification
                                           object:nil];
  [self setupSpeedAndDistance];

  [self.layout showWithData:self.data];
  
  // Call for the first time to produce changes
  [self processCountryEvent:[self.data countryId]];
}

- (void)dismiss
{
  [self.layout close];
  self.data = nil;
  [MWMLocationManager removeObserver:self];
  [MWMFrameworkListener removeObserver:self];
  [NSNotificationCenter.defaultCenter removeObserver:self];
}

- (void)handleBookmarkDeleting:(NSNotification *)notification
{
  auto data = self.data;
  NSAssert(data && self.layout, @"It must be openned place page!");
  if (!data.isBookmark)
    return;

  auto value = static_cast<NSValue *>(notification.object);
  auto deletedBookmarkAndCategory = BookmarkAndCategory();
  [value getValue:&deletedBookmarkAndCategory];
  NSAssert(deletedBookmarkAndCategory.IsValid(),
           @"Place page must have valid bookmark and category.");
  auto bookmarkAndCategory = data.bookmarkAndCategory;
  if (bookmarkAndCategory.m_bookmarkIndex != deletedBookmarkAndCategory.m_bookmarkIndex ||
      bookmarkAndCategory.m_categoryIndex != deletedBookmarkAndCategory.m_categoryIndex)
    return;

  [self closePlacePage];
}

- (void)handleBookmarkCategoryDeleting:(NSNotification *)notification
{
  auto data = self.data;
  NSAssert(data && self.layout, @"It must be openned place page!");
  if (!data.isBookmark)
    return;

  auto deletedIndex = static_cast<NSNumber *>(notification.object).integerValue;
  auto index = data.bookmarkAndCategory.m_categoryIndex;
  if (index != deletedIndex)
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
  case NodeStatus::Partly: [MWMStorage downloadNode:countryId onSuccess:nil]; break;
  case NodeStatus::Undefined:
  case NodeStatus::Error: [MWMStorage retryDownloadNode:countryId]; break;
  case NodeStatus::OnDiskOutOfDate: [MWMStorage updateNode:countryId]; break;
  case NodeStatus::Downloading:
  case NodeStatus::InQueue: [MWMStorage cancelDownloadNode:countryId]; break;
  case NodeStatus::OnDisk: break;
  }
}

- (NSString *)distanceToObject
{
  CLLocation * lastLocation = [MWMLocationManager lastLocation];
  auto data = self.data;
  if (!lastLocation || !data)
    return @"";
  string distance;
  CLLocationCoordinate2D const & coord = lastLocation.coordinate;
  ms::LatLon const & target = data.latLon;
  measurement_utils::FormatDistance(
      ms::DistanceOnEarth(coord.latitude, coord.longitude, target.lat, target.lon), distance);
  return @(distance.c_str());
}

#pragma mark - MWMFrameworkStorageObserver

- (void)processCountryEvent:(TCountryId const &)countryId
{
  auto data = self.data;
  if (!data || [data countryId] != countryId)
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

- (void)processCountry:(TCountryId const &)countryId
              progress:(MapFilesDownloader::TProgress const &)progress
{
  auto data = self.data;
  if (!data || countryId == kInvalidCountryId || [data countryId] != countryId)
    return;

  [self.layout
      processDownloaderEventWithStatus:storage::NodeStatus::Downloading
                              progress:static_cast<CGFloat>(progress.first) / progress.second];
}

#pragma mark - MWMPlacePageLayoutDelegate

- (void)onPlacePageTopBoundChanged:(CGFloat)bound
{
  [self.ownerViewController setPlacePageTopBound:bound];
  [self.layout checkCellsVisible];
}

- (void)destroyLayout { self.layout = nil; }
- (void)closePlacePage { GetFramework().DeactivateMapSelection(true); }
- (BOOL)isExpandedOnShow
{
  auto data = self.data;
  return data.isViator || data.isCian;
}

- (void)onExpanded
{
  if (self.isSponsoredOpenLogged)
    return;
  self.isSponsoredOpenLogged = YES;
  auto data = self.data;
  if (!data)
    return;
  NSMutableDictionary * parameters = [@{} mutableCopy];
  if (data.isViator)
    parameters[kStatProvider] = kStatViator;
  else if (data.isBooking)
    parameters[kStatProvider] = kStatBooking;
  else if (data.isCian)
    parameters[kStatProvider] = kStatCian;
  switch (Platform::ConnectionStatus())
  {
  case Platform::EConnectionType::CONNECTION_NONE:
    parameters[kStatConnection] = kStatOffline;
    break;
  case Platform::EConnectionType::CONNECTION_WIFI: parameters[kStatConnection] = kStatWifi; break;
  case Platform::EConnectionType::CONNECTION_WWAN: parameters[kStatConnection] = kStatMobile; break;
  }
  parameters[kStatTags] = data.statisticsTags;
  [Statistics logEvent:kStatPlacepageSponsoredOpen withParameters:parameters];
}

#pragma mark - MWMLocationObserver

- (void)onHeadingUpdate:(location::CompassInfo const &)info
{
  auto lastLocation = [MWMLocationManager lastLocation];
  auto data = self.data;
  if (!lastLocation || !data)
    return;

  auto const locationMercator = lastLocation.mercator;
  auto const dataMercator = data.mercator;
  if (my::AlmostEqualAbs(locationMercator, dataMercator, 1e-10))
    return;

  auto const angle = ang::AngleTo(locationMercator, dataMercator) + info.m_bearing;
  [self.layout rotateDirectionArrowToAngle:angle];
}

- (void)setupSpeedAndDistance
{
  [self.layout setDistanceToObject:self.distanceToObject];
  if (self.data.isMyPosition)
    [self.layout setSpeedAndAltitude:location_helpers::formattedSpeedAndAltitude(MWMLocationManager.lastLocation)];
}

- (void)onLocationUpdate:(location::GpsInfo const &)locationInfo
{
  [self setupSpeedAndDistance];
}

- (void)mwm_refreshUI { [self.layout mwm_refreshUI]; }
- (void)routeFrom
{
  [Statistics logEvent:kStatEventName(kStatPlacePage, kStatBuildRoute)
        withParameters:@{kStatValue : kStatSource}];

  MWMRoutePoint * point = [self routePointWithType:MWMRoutePointTypeStart intermediateIndex:0];
  [MWMRouter buildFromPoint:point bestRouter:YES];
  [self closePlacePage];
}

- (void)routeTo
{
  [Statistics logEvent:kStatEventName(kStatPlacePage, kStatBuildRoute)
        withParameters:@{kStatValue : kStatDestination}];

  MWMRoutePoint * point = [self routePointWithType:MWMRoutePointTypeFinish intermediateIndex:0];
  [MWMRouter buildToPoint:point bestRouter:YES];
  [self closePlacePage];
}

- (void)addStop
{
  MWMRoutePoint * point =
      [self routePointWithType:MWMRoutePointTypeIntermediate intermediateIndex:0];
  [MWMRouter addPointAndRebuild:point];
  [self closePlacePage];
}

- (void)removeStop
{
  auto data = self.data;
  MWMRoutePoint * point = nil;
  switch (data.routeMarkType)
  {
  case RouteMarkType::Start:
    point =
        [self routePointWithType:MWMRoutePointTypeStart intermediateIndex:data.intermediateIndex];
    break;
  case RouteMarkType::Finish:
    point =
        [self routePointWithType:MWMRoutePointTypeFinish intermediateIndex:data.intermediateIndex];
    break;
  case RouteMarkType::Intermediate:
    point = [self routePointWithType:MWMRoutePointTypeIntermediate
                   intermediateIndex:data.intermediateIndex];
    break;
  }
  [MWMRouter removePointAndRebuild:point];
  [self closePlacePage];
}

- (void)orderTaxi:(MWMPlacePageTaxiProvider)provider
{
  auto data = self.data;
  if (!data)
    return;
  NSString * providerString = nil;
  switch (provider)
  {
  case MWMPlacePageTaxiProviderTaxi: providerString = kStatUnknown;
  case MWMPlacePageTaxiProviderUber: providerString = kStatUber;
  case MWMPlacePageTaxiProviderYandex: providerString = kStatYandex;
  }
  [Statistics logEvent:kStatPlacePageTaxiClick
        withParameters:@{kStatProvider : providerString, kStatTags : data.statisticsTags}];
  [MWMRouter setType:MWMRouterTypeTaxi];
  MWMRoutePoint * point = [self routePointWithType:MWMRoutePointTypeFinish intermediateIndex:0];
  [MWMRouter buildToPoint:point bestRouter:NO];
  [self closePlacePage];
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
    title = L(@"placepage_unknown_place");

  NSString * subtitle = nil;
  if (data.subtitle.length > 0 && ![title isEqualToString:data.subtitle])
    subtitle = data.subtitle;

  return [[MWMRoutePoint alloc] initWithPoint:data.mercator
                                        title:title
                                     subtitle:subtitle
                                         type:type
                            intermediateIndex:intermediateIndex];
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

- (void)addBookmark
{
  auto data = self.data;
  if (!data)
    return;
  [Statistics logEvent:kStatBookmarkCreated];
  [data updateBookmarkStatus:YES];
  [self.layout reloadBookmarkSection:YES];
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

- (void)editBookmark
{
  auto data = self.data;
  if (data)
    [self.ownerViewController openBookmarkEditorWithData:data];
}

- (void)book:(BOOL)isDescription
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
  logSponsoredEvent(data, eventName);
  NSURL * url = isDescription ? data.sponsoredDescriptionURL : data.sponsoredURL;
  NSAssert(url, @"Sponsored url can't be nil!");
  [self.ownerViewController openUrl:url];
}

- (void)searchBookingHotels
{
  auto data = self.data;
  if (!data)
    return;
  logSponsoredEvent(data, kStatPlacePageHotelSearch);
  NSURL * url = data.bookingSearchURL;
  NSAssert(url, @"Search url can't be nil!");
  [self.ownerViewController openUrl:url];
}

- (void)call
{
  auto data = self.data;
  if (!data)
    return;
  NSAssert(data.phoneNumber, @"Phone number can't be nil!");
  NSString * phoneNumber = [[@"telprompt:" stringByAppendingString:data.phoneNumber]
      stringByAddingPercentEscapesUsingEncoding:NSUTF8StringEncoding];
  NSURL * url = [NSURL URLWithString:phoneNumber];
  [UIApplication.sharedApplication openURL:url];
}

- (void)apiBack
{
  auto data = self.data;
  if (!data)
    return;
  [Statistics logEvent:kStatEventName(kStatPlacePage, kStatAPI)];
  NSURL * url = [NSURL URLWithString:data.apiURL];
  [UIApplication.sharedApplication openURL:url];
  [self.ownerViewController.apiBar back];
}

- (void)showAllReviews
{
  auto data = self.data;
  if (!data)
    return;
  logSponsoredEvent(data, kStatPlacePageHotelReviews);
  [self.ownerViewController openUrl:data.URLToAllReviews];
}

- (void)showPhotoAtIndex:(NSInteger)index
           referenceView:(UIView *)referenceView
           referenceViewWhenDismissingHandler:(MWMPlacePageButtonsDismissBlock)referenceViewWhenDismissingHandler
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

  [[MapViewController controller] presentViewController:photoVC animated:YES completion:nil];
}

- (void)showGallery
{
  auto data = self.data;
  if (!data)
    return;
  logSponsoredEvent(self.data, kStatPlacePageHotelGallery);
  auto galleryModel = [[MWMGalleryModel alloc] initWithTitle:self.hotelName items:data.photos];
  auto galleryVc = [MWMGalleryViewController instanceWithModel:galleryModel];
  [[MapViewController controller].navigationController pushViewController:galleryVc animated:YES];
}

- (void)showAllFacilities
{
  auto data = self.data;
  if (!data)
    return;
  logSponsoredEvent(data, kStatPlacePageHotelFacilities);
  [self.ownerViewController openHotelFacilities];
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

- (void)openSponsoredURL:(nullable NSURL *)url
{
  if (auto u = url ?: self.data.sponsoredURL)
    [self.ownerViewController openUrl:u];
}

- (void)reviewOn:(NSInteger)starNumber
{
  // TODO: Prepare ugc update.
}

#pragma mark - AvailableArea / PlacePageArea

- (void)updateAvailableArea:(CGRect)frame
{
  auto data = self.data;
  if (data)
    [self.layout updateAvailableArea:frame];
}

#pragma mark - MWMFeatureHolder

- (FeatureID const &)featureId { return [self.data featureId]; }
#pragma mark - MWMBookingInfoHolder

- (std::vector<booking::HotelFacility> const &)hotelFacilities { return self.data.facilities; }
- (NSString *)hotelName { return self.data.title; }

#pragma mark - Ownerfacilities

- (MapViewController *)ownerViewController { return [MapViewController controller]; }

@end
