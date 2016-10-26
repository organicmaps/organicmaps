#import "MWMPlacePageViewManager.h"
#import <Pushwoosh/PushNotificationManager.h>
#import "Common.h"
#import "MWMAPIBar.h"
#import "MWMActivityViewController.h"
#import "MWMBasePlacePageView.h"
#import "MWMDirectionView.h"
#import "MWMFrameworkListener.h"
#import "MWMLocationHelpers.h"
#import "MWMLocationManager.h"
#import "MWMPlacePage.h"
#import "MWMPlacePageActionBar.h"
#import "MWMPlacePageEntity.h"
#import "MWMPlacePageNavigationBar.h"
#import "MWMRouter.h"
#import "MWMiPadPlacePage.h"
#import "MWMiPhoneLandscapePlacePage.h"
#import "MWMiPhonePortraitPlacePage.h"
#import "MapViewController.h"
#import "MapsAppDelegate.h"
#import "Statistics.h"

#import "3party/Alohalytics/src/alohalytics_objc.h"

#include "MWMRoutePoint.h"

#include "geometry/distance_on_sphere.hpp"
#include "map/place_page_info.hpp"
#include "platform/measurement_utils.hpp"

extern NSString * const kAlohalyticsTapEventKey;
extern NSString * const kBookmarksChangedNotification;

@interface MWMPlacePageViewManager ()<MWMLocationObserver>

@property(nonatomic, readwrite) MWMPlacePageEntity * entity;
@property(nonatomic) MWMPlacePage * placePage;
@property(nonatomic) MWMDirectionView * directionView;

@end

@implementation MWMPlacePageViewManager

- (void)hidePlacePage { [self.placePage hide]; }
- (void)dismissPlacePage
{
  [self.placePage dismiss];
  [MWMLocationManager removeObserver:self];
  GetFramework().DeactivateMapSelection(false);
  self.placePage = nil;
}

- (void)showPlacePage:(place_page::Info const &)info
{
  [MWMLocationManager addObserver:self];
  self.entity = [[MWMPlacePageEntity alloc] initWithInfo:info];
  if (IPAD)
    [self setPlacePageForiPad];
  else
    [self setPlacePageForiPhoneWithOrientation:self.ownerViewController.interfaceOrientation];
  [self configPlacePage];
}

- (FeatureID const &)featureId { return self.entity.featureID; }
#pragma mark - Layout

- (void)willRotateToInterfaceOrientation:(UIInterfaceOrientation)orientation
{
  [self rotateToOrientation:orientation];
}

- (void)viewWillTransitionToSize:(CGSize)size
       withTransitionCoordinator:(id<UIViewControllerTransitionCoordinator>)coordinator
{
  [self rotateToOrientation:size.height > size.width ? UIInterfaceOrientationPortrait
                                                     : UIInterfaceOrientationLandscapeLeft];
}

- (void)rotateToOrientation:(UIInterfaceOrientation)orientation
{
  if (!self.placePage)
    return;

  if (IPAD)
  {
    self.placePage.parentViewHeight = self.ownerViewController.view.width;
    [(MWMiPadPlacePage *)self.placePage updatePlacePageLayoutAnimated:NO];
  }
  else
  {
    [self.placePage dismiss];
    [self setPlacePageForiPhoneWithOrientation:orientation];
    [self configPlacePage];
  }
}

- (void)configPlacePage
{
  if (self.entity.isMyPosition)
    self.entity.subtitle =
        location_helpers::formattedSpeedAndAltitude([MWMLocationManager lastLocation]);
  self.placePage.parentViewHeight = self.ownerViewController.view.height;
  [self.placePage configure];
  self.placePage.topBound = self.topBound;
  self.placePage.leftBound = self.leftBound;
  [self refreshPlacePage];
}

- (void)refreshPlacePage
{
  [self.placePage show];
  [self updateDistance];
}

- (void)mwm_refreshUI
{
  [self.placePage.extendedPlacePageView mwm_refreshUI];
  [self.placePage.actionBar mwm_refreshUI];
}

- (BOOL)hasPlacePage { return self.placePage != nil; }
- (void)setPlacePageForiPad
{
  [self.placePage dismiss];
  self.placePage = [[MWMiPadPlacePage alloc] initWithManager:self];
}

- (void)updateMyPositionSpeedAndAltitude
{
  if (self.entity.isMyPosition)
    [self.placePage updateMyPositionStatus:location_helpers::formattedSpeedAndAltitude(
                                               [MWMLocationManager lastLocation])];
}

- (void)setPlacePageForiPhoneWithOrientation:(UIInterfaceOrientation)orientation
{
  switch (orientation)
  {
  case UIInterfaceOrientationLandscapeLeft:
  case UIInterfaceOrientationLandscapeRight:
    if (![self.placePage isKindOfClass:[MWMiPhoneLandscapePlacePage class]])
      self.placePage = [[MWMiPhoneLandscapePlacePage alloc] initWithManager:self];
    break;

  case UIInterfaceOrientationPortrait:
  case UIInterfaceOrientationPortraitUpsideDown:
    if (![self.placePage isKindOfClass:[MWMiPhonePortraitPlacePage class]])
      self.placePage = [[MWMiPhonePortraitPlacePage alloc] initWithManager:self];
    break;

  case UIInterfaceOrientationUnknown: break;
  }
}

- (void)addSubviews:(NSArray *)views withNavigationController:(UINavigationController *)controller
{
  if (controller)
    [self.ownerViewController addChildViewController:controller];
  [[MWMMapViewControlsManager manager] addPlacePageViews:views];
}

- (void)routeFrom
{
  [Statistics logEvent:kStatEventName(kStatPlacePage, kStatBuildRoute)
        withParameters:@{kStatValue : kStatSource}];
  [Alohalytics logEvent:kAlohalyticsTapEventKey withValue:@"ppRoute"];
  [[MWMRouter router] buildFromPoint:self.target bestRouter:YES];
  [self hidePlacePage];
}

- (void)routeTo
{
  [Statistics logEvent:kStatEventName(kStatPlacePage, kStatBuildRoute)
        withParameters:@{kStatValue : kStatDestination}];
  [Alohalytics logEvent:kAlohalyticsTapEventKey withValue:@"ppRoute"];
  auto r = [MWMRouter router];
  [r buildToPoint:self.target bestRouter:YES];
  [self hidePlacePage];
}

- (MWMRoutePoint)target
{
  NSString * name = nil;
  if (self.entity.title.length > 0)
    name = self.entity.title;
  else if (self.entity.address.length > 0)
    name = self.entity.address;
  else if (self.entity.subtitle.length > 0)
    name = self.entity.subtitle;
  else if (self.entity.isBookmark)
    name = self.entity.bookmarkTitle;
  else
    name = L(@"placepage_unknown_place");

  m2::PointD const & org = self.entity.mercator;
  return self.entity.isMyPosition ? MWMRoutePoint(org) : MWMRoutePoint(org, name);
}

- (void)share
{
  [Statistics logEvent:kStatEventName(kStatPlacePage, kStatShare)];
  [Alohalytics logEvent:kAlohalyticsTapEventKey withValue:@"ppShare"];
  MWMPlacePageEntity * entity = self.entity;
  MWMActivityViewController * shareVC = [MWMActivityViewController
      shareControllerForPlacePageObject:static_cast<id<MWMPlacePageObject>>(entity)];
  [shareVC presentInParentViewController:self.ownerViewController
                              anchorView:self.placePage.actionBar.shareAnchor];
}

- (void)book:(BOOL)isDescription
{
  MWMPlacePageEntity * data = self.entity;
  BOOL const isBooking = data.isBooking;
  auto const & latLon = data.latLon;
  NSMutableDictionary * stat = [@{} mutableCopy];
  if (isBooking)
  {
    stat[kStatProvider] = kStatBooking;
    stat[kStatHotel] = data.sponsoredId;
    stat[kStatHotelLat] = @(latLon.lat);
    stat[kStatHotelLon] = @(latLon.lon);
  }
  else
  {
    stat[kStatProvider] = kStatOpentable;
    stat[kStatRestaurant] = data.sponsoredId;
    stat[kStatRestaurantLat] = @(latLon.lat);
    stat[kStatRestaurantLon] = @(latLon.lon);
  }

  NSString * eventName = isBooking ? kPlacePageHotelBook : kPlacePageRestaurantBook;
  [Statistics logEvent:isDescription ? kPlacePageHotelDetails : eventName
        withParameters:stat
            atLocation:[MWMLocationManager lastLocation]];

  UIViewController * vc = static_cast<UIViewController *>([MapViewController controller]);
  NSURL * url = isDescription ? self.entity.sponsoredDescriptionURL : self.entity.sponsoredURL;
  NSAssert(url, @"Sponsored url can't be nil!");
  [vc openUrl:url];
}

- (void)call
{
  NSString * tel = [self.entity getCellValue:MWMPlacePageCellTypePhoneNumber];
  NSAssert(tel, @"Phone number can't be nil!");
  NSString * phoneNumber = [[@"telprompt:" stringByAppendingString:tel]
      stringByAddingPercentEscapesUsingEncoding:NSUTF8StringEncoding];
  [[UIApplication sharedApplication] openURL:[NSURL URLWithString:phoneNumber]];
}

- (void)apiBack
{
  [Statistics logEvent:kStatEventName(kStatPlacePage, kStatAPI)];
  [[UIApplication sharedApplication] openURL:[NSURL URLWithString:self.entity.apiURL]];
  [((MapViewController *)self.ownerViewController).apiBar back];
}

- (void)editPlace
{
  [Statistics logEvent:kStatEventName(kStatPlacePage, kStatEdit)];
  [[PushNotificationManager pushManager] setTags:@{ @"editor_edit_discovered" : @YES }];
  [(MapViewController *)self.ownerViewController openEditor];
}

- (void)addBusiness
{
  [Statistics logEvent:kStatEditorAddClick withParameters:@{kStatValue : kStatPlacePage}];
  [[MWMMapViewControlsManager manager] addPlace:YES hasPoint:NO point:m2::PointD()];
}

- (void)addPlace
{
  [Statistics logEvent:kStatEditorAddClick
        withParameters:@{kStatValue : kStatPlacePageNonBuilding}];
  [[MWMMapViewControlsManager manager] addPlace:NO hasPoint:YES point:self.entity.mercator];
}

- (void)addBookmark
{
  [Statistics logEvent:kStatEventName(kStatPlacePage, kStatBookmarks)
        withParameters:@{kStatValue : kStatAdd}];
  Framework & f = GetFramework();
  BookmarkData bmData = {self.entity.titleForNewBookmark, f.LastEditedBMType()};
  auto const categoryIndex = f.LastEditedBMCategory();
  auto const bookmarkIndex = f.GetBookmarkManager().AddBookmark(categoryIndex, self.entity.mercator, bmData);
  self.entity.bac = {bookmarkIndex, categoryIndex};
  self.entity.bookmarkTitle = @(bmData.GetName().c_str());
  self.entity.bookmarkCategory = @(f.GetBmCategory(categoryIndex)->GetName().c_str());
  [NSNotificationCenter.defaultCenter postNotificationName:kBookmarksChangedNotification
                                                    object:nil
                                                  userInfo:nil];
  [self updateDistance];
  [self.placePage addBookmark];
}

- (void)removeBookmark
{
  [Statistics logEvent:kStatEventName(kStatPlacePage, kStatBookmarks)
        withParameters:@{kStatValue : kStatRemove}];
  Framework & f = GetFramework();
  BookmarkCategory * bookmarkCategory =
      f.GetBookmarkManager().GetBmCategory(self.entity.bac.m_categoryIndex);
  if (bookmarkCategory)
  {
    {
      BookmarkCategory::Guard guard(*bookmarkCategory);
      guard.m_controller.DeleteUserMark(self.entity.bac.m_bookmarkIndex);
    }
    bookmarkCategory->SaveToKMLFile();
  }
  self.entity.bac = {};
  self.entity.bookmarkTitle = nil;
  self.entity.bookmarkCategory = nil;
  [NSNotificationCenter.defaultCenter postNotificationName:kBookmarksChangedNotification
                                                    object:nil
                                                  userInfo:nil];
  [self updateDistance];
  [self.placePage removeBookmark];
}

- (void)reloadBookmark
{
  [self.entity synchronize];
  [self.placePage reloadBookmark];
  [self updateDistance];
}

- (void)dragPlacePage:(CGRect)frame { [[MWMMapViewControlsManager manager] dragPlacePage:frame]; }
- (void)updateDistance
{
  NSString * distance = [self distance];
  self.directionView.distanceLabel.text = distance;
  [self.placePage setDistance:distance];
}

- (NSString *)distance
{
  CLLocation * lastLocation = [MWMLocationManager lastLocation];
  if (!lastLocation)
    return @"";
  string distance;
  CLLocationCoordinate2D const coord = lastLocation.coordinate;
  ms::LatLon const & target = self.entity.latLon;
  measurement_utils::FormatDistance(
      ms::DistanceOnEarth(coord.latitude, coord.longitude, target.lat, target.lon), distance);
  return @(distance.c_str());
}

- (void)showDirectionViewWithTitle:(NSString *)title type:(NSString *)type
{
  MWMDirectionView * directionView = self.directionView;
  directionView.titleLabel.text = title;
  directionView.typeLabel.text = type;
  [directionView show];
  [self updateDistance];
}

- (void)changeHeight:(CGFloat)height
{
  if (!IPAD)
    return;
  ((MWMiPadPlacePage *)self.placePage).height = height;
}

#pragma mark - MWMLocationObserver

- (void)onHeadingUpdate:(location::CompassInfo const &)info
{
  CLLocation * lastLocation = [MWMLocationManager lastLocation];
  if (!lastLocation)
    return;
  CGFloat const angle = ang::AngleTo(lastLocation.mercator, self.entity.mercator) + info.m_bearing;
  CGAffineTransform transform = CGAffineTransformMakeRotation(M_PI_2 - angle);
  [self.placePage setDirectionArrowTransform:transform];
  [self.directionView setDirectionArrowTransform:transform];
}

- (void)onLocationUpdate:(location::GpsInfo const &)locationInfo
{
  [self updateDistance];
  [self updateMyPositionSpeedAndAltitude];
}

#pragma mark - Properties

- (MWMDirectionView *)directionView
{
  if (!_directionView)
    _directionView = [[MWMDirectionView alloc] initWithManager:self];
  return _directionView;
}

- (MapViewController *)ownerViewController { return [MapViewController controller]; }
- (void)setTopBound:(CGFloat)topBound { _topBound = self.placePage.topBound = topBound; }
- (void)setLeftBound:(CGFloat)leftBound { _leftBound = self.placePage.leftBound = leftBound; }
- (void)editBookmark {}
@end
