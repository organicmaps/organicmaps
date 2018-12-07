#import "MWMPlacePageManager.h"
#import "CLLocation+Mercator.h"
#import "MWMAPIBar.h"
#import "MWMActivityViewController.h"
#import "MWMBookmarksManager.h"
#import "MWMFrameworkListener.h"
#import "MWMFrameworkObservers.h"
#import "MWMLocationHelpers.h"
#import "MWMLocationManager.h"
#import "MWMLocationObserver.h"
#import "MWMPlacePageData.h"
#import "MWMPlacePageLayout.h"
#import "MWMRoutePoint+CPP.h"
#import "MWMRouter.h"
#import "MWMSearch.h"
#import "MWMSearchHotelsFilterViewController.h"
#import "MWMStorage.h"
#import "MWMUGCViewModel.h"
#import "MapViewController.h"
#import "Statistics.h"
#import "SwiftBridge.h"

#include "Framework.h"

#include "map/bookmark.hpp"
#include "map/utils.hpp"

#include "geometry/distance_on_sphere.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"
#include "base/stl_helpers.hpp"

#include <utility>

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
  else if (data.isPartner)
  {
    stat[kStatProvider] = data.partnerName;
    stat[kStatCategory] = @(data.ratingRawValue);
    stat[kStatObjectLat] = @(latLon.lat);
    stat[kStatObjectLon] = @(latLon.lon);
  }
  else
  {
    stat[kStatProvider] = kStatPlacePageHotelSearch;
    stat[kStatHotelLocation] = makeLocationEventValue(latLon.lat, latLon.lon);
  }

  [Statistics logEvent:eventName withParameters:stat atLocation:[MWMLocationManager lastLocation]];
}

void RegisterEventIfPossible(eye::MapObject::Event::Type const type, place_page::Info const & info)
{
  auto const userPos = GetFramework().GetCurrentPosition();
  if (userPos)
  {
    auto const mapObject = utils::MakeEyeMapObject(info);
    if (!mapObject.IsEmpty())
      eye::Eye::Event::MapObjectEvent(mapObject, type, userPos.get());
  }
}
}  // namespace

@interface MWMPlacePageManager ()<MWMFrameworkStorageObserver, MWMPlacePageLayoutDelegate,
                                  MWMPlacePageLayoutDataSource, MWMLocationObserver,
                                  MWMBookmarksObserver>

@property(nonatomic) MWMPlacePageLayout * layout;
@property(nonatomic) MWMPlacePageData * data;

@property(nonatomic) storage::NodeStatus currentDownloaderStatus;

@property(nonatomic) BOOL isSponsoredOpenLogged;

@end

@implementation MWMPlacePageManager

- (void)showReview:(place_page::Info const &)info
{
  [self show:info];
  [self showUGCAddReview:MWMRatingSummaryViewValueTypeNoValue
              fromSource:MWMUGCReviewSourceNotification];
}

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
  [[MWMBookmarksManager sharedManager] addObserver:self];

  [self setupSpeedAndDistance];

  [self.layout showWithData:self.data];
  
  // Call for the first time to produce changes
  [self processCountryEvent:[self.data countryId]];
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
  case NodeStatus::Partly: [MWMStorage downloadNode:countryId onSuccess:nil]; break;
  case NodeStatus::Undefined:
  case NodeStatus::Error: [MWMStorage retryDownloadNode:countryId]; break;
  case NodeStatus::OnDiskOutOfDate: [MWMStorage updateNode:countryId]; break;
  case NodeStatus::Downloading:
  case NodeStatus::Applying:
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
- (BOOL)isExpandedOnShow { return self.data.isPreviewExtended; }
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
  else if (data.isPartner)
    parameters[kStatProvider] = data.partnerName;
  else if (data.isHolidayObject)
    parameters[kStatProvider] = kStatHoliday;

  parameters[kStatConnection] = [Statistics connectionTypeString];
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
  if (base::AlmostEqualAbs(locationMercator, dataMercator, 1e-10))
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

  if ([MWMRouter isOnRoute])
    [MWMRouter stopRouting];
  
  if ([MWMTrafficManager transitEnabled])
    [MWMRouter setType:MWMRouterTypePublicTransport];
  
  MWMRoutePoint * point = [self routePointWithType:MWMRoutePointTypeFinish intermediateIndex:0];
  [MWMRouter buildToPoint:point bestRouter:YES];
  [self closePlacePage];
}

- (void)routeAddStop
{
  MWMRoutePoint * point =
      [self routePointWithType:MWMRoutePointTypeIntermediate intermediateIndex:0];
  [MWMRouter addPointAndRebuild:point];
  [self closePlacePage];
}

- (void)routeRemoveStop
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
  case MWMPlacePageTaxiProviderTaxi: providerString = kStatUnknown; break;
  case MWMPlacePageTaxiProviderUber: providerString = kStatUber; break;
  case MWMPlacePageTaxiProviderYandex: providerString = kStatYandex; break;
  case MWMPlacePageTaxiProviderMaxim: providerString = kStatMaxim; break;
  case MWMPlacePageTaxiProviderRutaxi: providerString = kStatRutaxi; break;
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
  RegisterEventIfPossible(eye::MapObject::Event::Type::AddToBookmark, data.getRawData);
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

- (void)showPlaceDescription:(NSString *)htmlString
{
  [self.ownerViewController openFullPlaceDescriptionWithHtml:htmlString];
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
  
  if (!isDescription && data.isPartnerAppInstalled)
  {
    [UIApplication.sharedApplication openURL:data.deepLink];
    return;
  }
  
  NSURL * url = isDescription ? data.sponsoredDescriptionURL : data.sponsoredURL;
  NSAssert(url, @"Sponsored url can't be nil!");
  
  [UIApplication.sharedApplication openURL:url];
}

- (void)searchBookingHotels
{
  auto data = self.data;
  if (!data)
    return;
  logSponsoredEvent(data, kStatPlacePageHotelSearch);
  NSURL * url = data.bookingSearchURL;
  NSAssert(url, @"Search url can't be nil!");
  [UIApplication.sharedApplication openURL:url];
}

- (void)openPartner
{
  auto data = self.data;
  if (!data)
    return;
  logSponsoredEvent(data, kStatPlacePageSponsoredActionButtonClick);
  NSURL * url = data.sponsoredURL;
  NSAssert(url, @"Partner url can't be nil!");
  [UIApplication.sharedApplication openURL:url];
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
  [UIApplication.sharedApplication openURL:data.URLToAllReviews];
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

- (void)showUGCAddReview:(MWMRatingSummaryViewValueType)value fromSource:(MWMUGCReviewSource)source
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

  RegisterEventIfPossible(eye::MapObject::Event::Type::UgcEditorOpened, data.getRawData);
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
          kStatIsAuthenticated: @([MWMAuthorizationViewModel isAuthenticated]),
          kStatIsOnline:
              @(GetPlatform().ConnectionStatus() != Platform::EConnectionType::CONNECTION_NONE),
          kStatMode: kStatAdd,
          kStatFrom: sourceString
        }];
  auto ugcReviewModel =
      [[MWMUGCReviewModel alloc] initWithReviewValue:value ratings:ratings title:title text:@""];
  auto ugcVC = [MWMUGCAddReviewController instanceWithModel:ugcReviewModel
                                                     onSave:^(MWMUGCReviewModel * model) {
                                                       auto data = self.data;
                                                       if (!data)
                                                       {
                                                         NSAssert(false, @"");
                                                         return;
                                                       }
                                                       RegisterEventIfPossible(
                                                           eye::MapObject::Event::Type::UgcSaved,
                                                           data.getRawData);
                                                       [data setUGCUpdateFrom:model];
                                                     }];
  [[MapViewController sharedController].navigationController pushViewController:ugcVC animated:YES];
}

- (void)searchSimilar
{
  [Statistics logEvent:@"Placepage_Hotel_search_similar"
        withParameters:@{kStatProvider : self.data.isBooking ? kStatBooking : kStatOSM}];

  auto data = self.data;
  if (!data)
    return;

  search_filter::HotelParams params;
  params.m_type = *data.hotelType;
  CHECK(data.hotelType, ("Incorrect hotel type at coordinate:", data.latLon.lat, data.latLon.lon));

  if (data.isBooking)
  {
    if (auto const price = data.hotelRawApproximatePricing)
    {
      CHECK_LESS_OR_EQUAL(*price, base::Key(search_filter::Price::Three), ());
      params.m_price = static_cast<search_filter::Price>(*price);
    }

    params.m_rating = place_page::rating::GetFilterRating(data.ratingRawValue);
  }

  [MWMSearch showHotelFilterWithParams:std::move(params)
                      onFinishCallback:^{
    [MWMMapViewControlsManager.manager searchTextOnMap:[L(@"booking_hotel") stringByAppendingString:@" "]
                                        forInputLocale:[NSLocale currentLocale].localeIdentifier];
  }];
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

- (void)openReviews:(id<MWMReviewsViewModelProtocol> _Nonnull)reviewsViewModel
{
  auto reviewsVC = [[MWMReviewsViewController alloc] initWithViewModel:reviewsViewModel];
  [[MapViewController sharedController].navigationController pushViewController:reviewsVC animated:YES];
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

- (MapViewController *)ownerViewController { return [MapViewController sharedController]; }

@end
