#import "MWMPlacePageManager.h"
#import "CLLocation+Mercator.h"
#import "MWMActivityViewController.h"
#import "MWMLocationHelpers.h"
#import "MWMLocationObserver.h"
#import "MWMRoutePoint+CPP.h"
#import "MWMSearchManager+Filter.h"
#import "MWMStorage+UI.h"
#import "SwiftBridge.h"
#import "MWMMapViewControlsManager+AddPlace.h"
#import "location_util.h"

#import <CoreApi/CoreApi.h>

#include "map/utils.hpp"

#include "platform/downloader_defines.hpp"

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

void RegisterEventIfPossible(eye::MapObject::Event::Type const type)
{
  auto const userPos = GetFramework().GetCurrentPosition();
  auto const & info = GetFramework().GetCurrentPlacePageInfo();
  utils::RegisterEyeEventIfPossible(type, userPos, info);
}
}  // namespace

@interface MWMPlacePageManager ()<MWMGCReviewSaver>

@property(nonatomic) storage::NodeStatus currentDownloaderStatus;

@property(nonatomic) BOOL isSponsoredOpenLogged;

@end

@implementation MWMPlacePageManager

- (void)showReview {
//  [self show];
//  [self showUGCAddReview:UgcSummaryRatingTypeNone
//              fromSource:MWMUGCReviewSourceNotification];
}

- (BOOL)isPPShown {
  return GetFramework().HasPlacePageInfo();
}

- (void)closePlacePage { GetFramework().DeactivateMapSelection(true); }

- (void)routeFrom:(PlacePageData *)data {
  [Statistics logEvent:kStatEventName(kStatPlacePage, kStatBuildRoute)
        withParameters:@{kStatValue : kStatSource}];

  MWMRoutePoint *point = [self routePoint:data withType:MWMRoutePointTypeStart intermediateIndex:0];
  [MWMRouter buildFromPoint:point bestRouter:YES];
  [self closePlacePage];
}

- (void)routeTo:(PlacePageData *)data {
  [Statistics logEvent:kStatEventName(kStatPlacePage, kStatBuildRoute)
        withParameters:@{kStatValue : kStatDestination}];

  if ([MWMRouter isOnRoute]) {
    [MWMRouter stopRouting];
  }

  if ([MWMMapOverlayManager transitEnabled]) {
    [MWMRouter setType:MWMRouterTypePublicTransport];
  }

  MWMRoutePoint *point = [self routePoint:data withType:MWMRoutePointTypeFinish intermediateIndex:0];
  [MWMRouter buildToPoint:point bestRouter:YES];
  [self closePlacePage];
}

- (void)routeAddStop:(PlacePageData *)data {
  MWMRoutePoint *point = [self routePoint:data withType:MWMRoutePointTypeIntermediate intermediateIndex:0];
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

+ (NSString *)taxiProviderStatisticsString:(PlacePageTaxiProvider)provider
{
  switch (provider)
  {
  case PlacePageTaxiProviderNone: return kStatUnknown;
  case PlacePageTaxiProviderUber: return kStatUber;
  case PlacePageTaxiProviderYandex: return kStatYandex;
  case PlacePageTaxiProviderMaxim: return kStatMaxim;
  case PlacePageTaxiProviderRutaxi: return kStatVezet;
  case PlacePageTaxiProviderFreenow: return kStatFreenow;
  case PlacePageTaxiProviderYango: return kStatYango;
  case PlacePageTaxiProviderCitymobil: return kStatCitymobil;
  }
}

- (void)orderTaxi:(PlacePageData *)data
{
  NSString * providerString = [MWMPlacePageManager taxiProviderStatisticsString:data.taxiProvider];

  [Statistics logEvent:kStatPlacePageTaxiClick
        withParameters:@{kStatProvider : providerString, kStatTags : data.statisticsTags}];
  [MWMRouter setType:MWMRouterTypeTaxi];
  MWMRoutePoint * point = [self routePointWithData:data pointType:MWMRoutePointTypeFinish intermediateIndex:0];
  [MWMRouter buildToPoint:point bestRouter:NO];
  [self closePlacePage];
}

- (void)logTaxiShown:(PlacePageData *)data
{
  NSString * providerString = [MWMPlacePageManager taxiProviderStatisticsString:data.taxiProvider];
  [Statistics logEvent:kStatPlacepageTaxiShow withParameters:@{kStatProvider : providerString}];
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

- (void)share:(PlacePageData *)data {
  [Statistics logEvent:kStatEventName(kStatPlacePage, kStatShare)];
  MWMActivityViewController * shareVC = [MWMActivityViewController shareControllerForPlacePage:data];
  [shareVC presentInParentViewController:self.ownerViewController anchorView:nil]; // TODO: add anchor for iPad
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
  [data updateBookmarkStatus];
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
  buildInfo.m_source = place_page::BuildInfo::Source::Other;
  f.UpdatePlacePageInfoForCurrentSelection(buildInfo);
  [data updateBookmarkStatus];
}

- (void)call:(PlacePageData *)data {
  if (data.infoData.phoneUrl && [UIApplication.sharedApplication canOpenURL:data.infoData.phoneUrl]) {
    [UIApplication.sharedApplication openURL:data.infoData.phoneUrl options:@{} completionHandler:nil];
  }
}

- (void)editBookmark:(PlacePageData *)data {
  MWMEditBookmarkController *editBookmarkController = [[UIStoryboard instance:MWMStoryboardMain]
                                                       instantiateViewControllerWithIdentifier:@"MWMEditBookmarkController"];
  editBookmarkController.placePageData = data;
  [[MapViewController sharedController].navigationController pushViewController:editBookmarkController animated:YES];
}

- (void)showPlaceDescription:(NSString *)htmlString
{
  [self.ownerViewController openFullPlaceDescriptionWithHtml:htmlString];
}

- (void)searchBookingHotels:(PlacePageData *)data {
  logSponsoredEvent(data, kStatPlacePageHotelBook);
  NSURL *url = [NSURL URLWithString:data.bookingSearchUrl];
  NSAssert(url, @"Search url can't be nil!");
  [UIApplication.sharedApplication openURL:url options:@{} completionHandler:nil];
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

- (void)book:(PlacePageData *)data {
  NSURL *url = [NSURL URLWithString:data.sponsoredURL];
  [self openPartner:data withStatisticLog:kStatPlacePageHotelBook proposedUrl:url];
}

- (void)openDescriptionUrl:(PlacePageData *)data {
  NSURL *url = [NSURL URLWithString:data.sponsoredDescriptionURL];
  [self openPartner:data withStatisticLog:kStatPlacePageHotelDetails proposedUrl:url];
}

- (void)openMoreUrl:(PlacePageData *)data {
  NSURL *url = [NSURL URLWithString:data.sponsoredMoreURL];
  if (!url) { return; }
  logSponsoredEvent(data, kStatPlacePageHotelMore);
  [UIApplication.sharedApplication openURL:url options:@{} completionHandler:nil];
  auto mercator = location_helpers::ToMercator(data.locationCoordinate);
  [MWMEye transitionToBookingWithPos:CGPointMake(mercator.x, mercator.y)];
}

- (void)openReviewUrl:(PlacePageData *)data {
  NSURL *url = [NSURL URLWithString:data.sponsoredReviewURL];
  [self openPartner:data withStatisticLog:kStatPlacePageHotelReviews proposedUrl:url];
}

- (void)openPartner:(PlacePageData *)data
{
  logSponsoredEvent(data, kStatPlacePageSponsoredActionButtonClick);
  NSURL *url = [NSURL URLWithString:data.sponsoredURL];
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

- (void)showUGCAddReview:(PlacePageData *)data
                  rating:(UgcSummaryRatingType)value
              fromSource:(MWMUGCReviewSource)source {
  RegisterEventIfPossible(eye::MapObject::Event::Type::UgcEditorOpened);
  NSString *sourceString;
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

  MWMUGCAddReviewController *ugcVC = [[MWMUGCAddReviewController alloc] initWithPlacePageData:data
                                                                                       rating:value
                                                                                        saver:self];
  [[MapViewController sharedController].navigationController pushViewController:ugcVC animated:YES];
}

- (void)searchSimilar:(PlacePageData *)data
{
  [Statistics logEvent:@"Placepage_Hotel_search_similar"
        withParameters:@{kStatProvider : data.sponsoredType == PlacePageSponsoredTypeBooking ? kStatBooking : kStatOSM}];

  MWMHotelParams * params = [[MWMHotelParams alloc] initWithPlacePageData:data];
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

- (void)openLocalAdsURL:(PlacePageData *)data {
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

- (void)openElevationDifficultPopup:(PlacePageData *)data {
  auto difficultyPopup = [ElevationDetailsBuilder buildWithData:data];
  [[MapViewController sharedController] presentViewController:difficultyPopup animated:YES completion:nil];
}

#pragma mark - AvailableArea / PlacePageArea

- (void)updateAvailableArea:(CGRect)frame
{
//  auto data = self.data;
//  if (data)
//    [self.layout updateAvailableArea:frame];
}

#pragma mark - MWMFeatureHolder

- (FeatureID const &)featureId { return GetFramework().GetCurrentPlacePageInfo().GetID(); }

#pragma mark - Ownerfacilities

- (MapViewController *)ownerViewController { return [MapViewController sharedController]; }

- (void)saveUgcWithPlacePageData:(PlacePageData *)placePageData
                           model:(MWMUGCReviewModel *)model
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
  [resultHandler, info, placePageData](ugc::Storage::SettingResult const result)
  {
    if (result != ugc::Storage::SettingResult::Success)
    {
      resultHandler(NO);
      return;
    }

    resultHandler(YES);
    GetFramework().UpdatePlacePageInfoForCurrentSelection();
    [placePageData updateUgcStatus];

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
