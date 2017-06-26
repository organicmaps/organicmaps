#import "MWMPlacePageManager.h"
#import <Pushwoosh/PushNotificationManager.h>
#import "CLLocation+Mercator.h"
#import "MWMAPIBar.h"
#import "MWMActivityViewController.h"
#import "MWMCircularProgress.h"
#import "MWMEditBookmarkController.h"
#import "MWMFrameworkListener.h"
#import "MWMFrameworkObservers.h"
#import "MWMLocationHelpers.h"
#import "MWMLocationManager.h"
#import "MWMLocationObserver.h"
#import "MWMPlacePageData.h"
#import "MWMPlacePageLayout.h"
#import "MWMRoutePoint+CPP.h"
#import "MWMRouter.h"
#import "MWMSideButtons.h"
#import "MWMStorage.h"
#import "MWMViewController.h"
#import "MapViewController.h"
#import "Statistics.h"
#import "SwiftBridge.h"

#include "geometry/distance_on_sphere.hpp"
#include "geometry/point2d.hpp"

#include "platform/measurement_utils.hpp"

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

@end

@implementation MWMPlacePageManager

- (void)show:(place_page::Info const &)info
{
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
  [[NSNotificationCenter defaultCenter] addObserver:self
                                           selector:@selector(handleBookmarkDeleting:)
                                               name:kBookmarkDeletedNotification
                                             object:nil];

  [[NSNotificationCenter defaultCenter] addObserver:self
                                           selector:@selector(handleBookmarkCategoryDeleting:)
                                               name:kBookmarkCategoryDeletedNotification
                                             object:nil];
  [self setupSpeedAndDistance];

  [self.layout showWithData:self.data];
  
  // Call for the first time to produce changes
  [self processCountryEvent:self.data.countryId];
}

- (void)close
{
  [self.layout close];
  self.data = nil;
  [MWMLocationManager removeObserver:self];
  [MWMFrameworkListener removeObserver:self];
  [[NSNotificationCenter defaultCenter] removeObserver:self];
}

- (void)handleBookmarkDeleting:(NSNotification *)notification
{
  NSAssert(self.data && self.layout, @"It must be openned place page!");
  if (!self.data.isBookmark)
    return;

  auto value = static_cast<NSValue *>(notification.object);
  auto deletedBac = BookmarkAndCategory();
  [value getValue:&deletedBac];
  NSAssert(deletedBac.IsValid(), @"Place page must have valid bookmark and category.");
  auto bac = self.data.bac;
  if (bac.m_bookmarkIndex != deletedBac.m_bookmarkIndex || bac.m_categoryIndex != deletedBac.m_categoryIndex)
    return;

  [self shouldClose];
}

- (void)handleBookmarkCategoryDeleting:(NSNotification *)notification
{
  NSAssert(self.data && self.layout, @"It must be openned place page!");
  if (!self.data.isBookmark)
    return;

  auto deletedIndex = static_cast<NSNumber *>(notification.object).integerValue;
  auto index = self.data.bac.m_categoryIndex;
  if (index != deletedIndex)
    return;

  [self shouldClose];
}

#pragma mark - MWMPlacePageLayoutDataSource

- (void)downloadSelectedArea
{
  auto data = self.data;
  if (!data)
    return;
  auto const & countryId = data.countryId;
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
  if (countryId == kInvalidCountryId)
  {
    [self.layout processDownloaderEventWithStatus:storage::NodeStatus::Undefined progress:0];
    return;
  }

  auto data = self.data;
  if (!data || data.countryId != countryId)
    return;

  NodeStatuses statuses;
  GetFramework().GetStorage().GetNodeStatuses(countryId, statuses);

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
  if (!data || countryId == kInvalidCountryId || data.countryId != countryId)
    return;

  [self.layout
      processDownloaderEventWithStatus:storage::NodeStatus::Downloading
                              progress:static_cast<CGFloat>(progress.first) / progress.second];
}

#pragma mark - MWMPlacePageLayout

- (void)onPlacePageTopBoundChanged:(CGFloat)bound
{
  self.ownerViewController.visibleAreaBottomOffset = bound;
  [[MWMSideButtons buttons] setBottomBound:self.ownerViewController.view.height - bound];
}

- (void)shouldDestroyLayout { self.layout = nil; }
- (void)shouldClose { GetFramework().DeactivateMapSelection(true); }
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
  [MWMRouter buildFromPoint:[self routePointWithType:MWMRoutePointTypeStart] bestRouter:YES];
  [self close];
}

- (void)routeTo
{
  [Statistics logEvent:kStatEventName(kStatPlacePage, kStatBuildRoute)
        withParameters:@{kStatValue : kStatDestination}];
  [MWMRouter buildToPoint:[self routePointWithType:MWMRoutePointTypeFinish] bestRouter:YES];
  [self close];
}

- (void)addStop
{
  [MWMRouter addIntermediatePointAndRebuild:[self routePointWithType:MWMRoutePointTypeIntermediate]
                          intermediateIndex:0];
  [self shouldClose];
}

- (void)removeStop
{
  switch (self.data.routeMarkType)
  {
  case RouteMarkType::Start:
    [MWMRouter removeStartPointAndRebuild:self.data.intermediateIndex];
    break;
  case RouteMarkType::Finish:
    [MWMRouter removeFinishPointAndRebuild:self.data.intermediateIndex];
    break;
  case RouteMarkType::Intermediate:
    [MWMRouter removeIntermediatePointAndRebuild:self.data.intermediateIndex];
    break;
  }
  [self shouldClose];
}

- (void)taxiTo
{
  auto data = self.data;
  if (!data)
    return;
  [Statistics logEvent:kStatPlacePageTaxiClick
        withParameters:@{kStatProvider : kStatUber, kStatTags : data.statisticsTags}];
  [MWMRouter setType:MWMRouterTypeTaxi];
  [MWMRouter buildToPoint:[self routePointWithType:MWMRoutePointTypeFinish] bestRouter:NO];
  [self close];
}

- (MWMRoutePoint *)routePointWithType:(MWMRoutePointType)type
{
  auto data = self.data;
  if (!data)
    return nil;

  if (data.isMyPosition)
    return [[MWMRoutePoint alloc] initWithLastLocationAndType:type];

  NSString * name = nil;
  if (data.title.length > 0)
    name = data.title;
  else if (data.address.length > 0)
    name = data.address;
  else if (data.subtitle.length > 0)
    name = data.subtitle;
  else if (data.isBookmark)
    name = data.externalTitle;
  else
    name = L(@"placepage_unknown_place");

  return [[MWMRoutePoint alloc] initWithPoint:data.mercator type:type];
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
  [[UIApplication sharedApplication] openURL:url];
}

- (void)apiBack
{
  auto data = self.data;
  if (!data)
    return;
  [Statistics logEvent:kStatEventName(kStatPlacePage, kStatAPI)];
  NSURL * url = [NSURL URLWithString:data.apiURL];
  [[UIApplication sharedApplication] openURL:url];
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
  auto galleryModel = [[MWMGalleryModel alloc] initWithTitle:self.hotelName items:self.data.photos];
  auto initialPhoto = galleryModel.items[index];
  auto photoVC = [[MWMPhotosViewController alloc] initWithPhotos:galleryModel
                                                    initialPhoto:initialPhoto
                                                   referenceView:referenceView];
  photoVC.referenceViewForPhotoWhenDismissingHandler = ^UIView *(MWMGalleryItemModel * photo) {
    return referenceViewWhenDismissingHandler([galleryModel.items indexOfObject:photo]);
  };

  [[MapViewController controller] presentViewController:photoVC animated:YES completion:nil];
}

- (void)showGalery
{
  auto data = self.data;
  if (!data)
    return;
  logSponsoredEvent(self.data, kStatPlacePageHotelGallery);
  auto galleryModel = [[MWMGalleryModel alloc] initWithTitle:self.hotelName items:self.data.photos];
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
  [self.ownerViewController openUrl:url];
  [data logLocalAdsEvent:local_ads::EventType::OpenInfo];
}

- (void)openViatorURL:(nullable NSURL *)url
{
  if (auto u = url ?: self.data.sponsoredURL)
    [self.ownerViewController openUrl:u];
}

- (void)viewWillTransitionToSize:(CGSize)size
       withTransitionCoordinator:(id<UIViewControllerTransitionCoordinator>)coordinator
{
  [coordinator
      animateAlongsideTransition:^(id<UIViewControllerTransitionCoordinatorContext> context) {
        [self.layout layoutWithSize:size];
      }
                      completion:^(id<UIViewControllerTransitionCoordinatorContext> context){
                      }];
}

#pragma mark - MWMFeatureHolder

- (FeatureID const &)featureId { return self.data.featureId; }

#pragma mark - MWMBookingInfoHolder

- (std::vector<booking::HotelFacility> const &)hotelFacilities { return self.data.facilities; }
- (NSString *)hotelName { return self.data.title; }

#pragma mark - Ownerfacilities

- (MapViewController *)ownerViewController { return [MapViewController controller]; }

#pragma mark - Deprecated

@synthesize leftBound = _leftBound;
@synthesize topBound = _topBound;

- (void)setTopBound:(CGFloat)topBound
{
  if (_topBound == topBound)
    return;

  _topBound = topBound;
  [self.layout updateTopBound];
}

- (void)setLeftBound:(CGFloat)leftBound
{
  if (_leftBound == leftBound)
    return;

  _leftBound = leftBound;
  [self.layout updateLeftBound];
}
@end
