#import "Common.h"
#import "LocationManager.h"
#import "MWMAPIBar.h"
#import "MWMActivityViewController.h"
#import "MWMBasePlacePageView.h"
#import "MWMDirectionView.h"
#import "MWMFrameworkListener.h"
#import "MWMPlacePage.h"
#import "MWMPlacePageActionBar.h"
#import "MWMPlacePageEntity.h"
#import "MWMPlacePageNavigationBar.h"
#import "MWMPlacePageViewManager.h"
#import "MWMPlacePageViewManagerDelegate.h"
#import "MWMiPadPlacePage.h"
#import "MWMiPhoneLandscapePlacePage.h"
#import "MWMiPhonePortraitPlacePage.h"
#import "MapViewController.h"
#import "MapsAppDelegate.h"
#import "Statistics.h"

#import "3party/Alohalytics/src/alohalytics_objc.h"

#include "geometry/distance_on_sphere.hpp"
#include "map/place_page_info.hpp"
#include "platform/measurement_utils.hpp"

extern NSString * const kAlohalyticsTapEventKey;
extern NSString * const kBookmarksChangedNotification;

@interface MWMPlacePageViewManager () <LocationObserver>

@property (weak, nonatomic) UIViewController * ownerViewController;
@property (nonatomic, readwrite) MWMPlacePageEntity * entity;
@property (nonatomic) MWMPlacePage * placePage;
@property (nonatomic) MWMDirectionView * directionView;

@property (weak, nonatomic) id<MWMPlacePageViewManagerProtocol> delegate;

@end

@implementation MWMPlacePageViewManager

- (instancetype)initWithViewController:(UIViewController *)viewController
                              delegate:(id<MWMPlacePageViewManagerProtocol>)delegate
{
  self = [super init];
  if (self)
  {
    self.ownerViewController = viewController;
    self.delegate = delegate;
  }
  return self;
}

- (void)hidePlacePage
{
  [self.placePage hide];
}

- (void)dismissPlacePage
{
  [self.delegate placePageDidClose];
  [self.placePage dismiss];
  [[MapsAppDelegate theApp].locationManager stop:self];
  GetFramework().DeactivateMapSelection(false);
  self.placePage = nil;
}

- (void)showPlacePage:(place_page::Info const &)info
{
  [[MapsAppDelegate theApp].locationManager start:self];
  self.entity = [[MWMPlacePageEntity alloc] initWithInfo:info];
  if (IPAD)
    [self setPlacePageForiPad];
  else
    [self setPlacePageForiPhoneWithOrientation:self.ownerViewController.interfaceOrientation];
  [self configPlacePage];
}

#pragma mark - Layout

- (void)willRotateToInterfaceOrientation:(UIInterfaceOrientation)orientation
{
  [self rotateToOrientation:orientation];
}

- (void)viewWillTransitionToSize:(CGSize)size
       withTransitionCoordinator:(id<UIViewControllerTransitionCoordinator>)coordinator
{
  [self rotateToOrientation:size.height > size.width ? UIInterfaceOrientationPortrait : UIInterfaceOrientationLandscapeLeft];
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
  {
    BOOL hasSpeed;
    self.entity.category = [[MapsAppDelegate theApp].locationManager formattedSpeedAndAltitude:hasSpeed];
  }
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

- (BOOL)hasPlacePage
{
  return self.placePage != nil;
}

- (void)setPlacePageForiPad
{
  [self.placePage dismiss];
  self.placePage = [[MWMiPadPlacePage alloc] initWithManager:self];
}

- (void)updateMyPositionSpeedAndAltitude
{
  if (!self.entity.isMyPosition)
    return;
  BOOL hasSpeed = NO;
  [self.placePage updateMyPositionStatus:[[MapsAppDelegate theApp].locationManager
                                          formattedSpeedAndAltitude:hasSpeed]];
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

    case UIInterfaceOrientationUnknown:
      break;
  }
}

- (void)addSubviews:(NSArray *)views withNavigationController:(UINavigationController *)controller
{
  if (controller)
    [self.ownerViewController addChildViewController:controller];
  [self.delegate addPlacePageViews:views];
}

- (void)buildRoute
{
  [Statistics logEvent:kStatEventName(kStatPlacePage, kStatBuildRoute)
                   withParameters:@{kStatValue : kStatDestination}];
  [Alohalytics logEvent:kAlohalyticsTapEventKey withValue:@"ppRoute"];

  LocationManager * lm = MapsAppDelegate.theApp.locationManager;
  [self.delegate buildRouteFrom:lm.isLocationModeUnknownOrPending ? MWMRoutePoint::MWMRoutePointZero()
                                                                  : MWMRoutePoint(lm.lastLocation.mercator)
                             to:{self.entity.mercator, self.placePage.basePlacePageView.titleLabel.text}];
}

- (void)routeFrom
{
  [Statistics logEvent:kStatEventName(kStatPlacePage, kStatBuildRoute)
                   withParameters:@{kStatValue : kStatSource}];
  [Alohalytics logEvent:kAlohalyticsTapEventKey withValue:@"ppRoute"];
  [self.delegate buildRouteFrom:self.target];
  [self hidePlacePage];
}

- (void)routeTo
{
  [Statistics logEvent:kStatEventName(kStatPlacePage, kStatBuildRoute)
                   withParameters:@{kStatValue : kStatDestination}];
  [Alohalytics logEvent:kAlohalyticsTapEventKey withValue:@"ppRoute"];
  [self.delegate buildRouteTo:self.target];
  [self hidePlacePage];
}

- (MWMRoutePoint)target
{
  m2::PointD const & org = self.entity.mercator;
  return self.entity.isMyPosition ? MWMRoutePoint(org)
                               : MWMRoutePoint(org, self.placePage.basePlacePageView.titleLabel.text);
}

- (void)share
{
  [Statistics logEvent:kStatEventName(kStatPlacePage, kStatShare)];
  [Alohalytics logEvent:kAlohalyticsTapEventKey withValue:@"ppShare"];
  MWMPlacePageEntity * entity = self.entity;
  NSString * title = entity.bookmarkTitle ? entity.bookmarkTitle : entity.title;
  CLLocationCoordinate2D const coord = CLLocationCoordinate2DMake(entity.latlon.lat, entity.latlon.lon);
  MWMActivityViewController * shareVC =
      [MWMActivityViewController shareControllerForLocationTitle:title
                                                        location:coord
                                                      myPosition:NO];
  [shareVC presentInParentViewController:self.ownerViewController
                              anchorView:self.placePage.actionBar.shareButton];
}

- (void)apiBack
{
  [Statistics logEvent:kStatEventName(kStatPlacePage, kStatAPI)];
  [[UIApplication sharedApplication] openURL:[NSURL URLWithString:self.entity.apiURL]];
  [self.delegate apiBack];
}

- (void)editPlace
{
  [(MapViewController *)self.ownerViewController openEditor];
}

- (void)reportProblem
{
  [static_cast<MapViewController *>(self.ownerViewController) showReportController];
}

- (void)addBookmark
{
  [Statistics logEvent:kStatEventName(kStatPlacePage, kStatBookmarks)
                   withParameters:@{kStatValue : kStatAdd}];
  Framework & f = GetFramework();
  BookmarkData bmData = { self.entity.titleForNewBookmark, f.LastEditedBMType() };
  size_t const categoryIndex = f.LastEditedBMCategory();
  size_t const bookmarkIndex = f.GetBookmarkManager().AddBookmark(categoryIndex, self.entity.mercator, bmData);
  self.entity.bac = {categoryIndex, bookmarkIndex};
  self.entity.bookmarkTitle = @(bmData.GetName().c_str());
  self.entity.bookmarkCategory = @(f.GetBmCategory(categoryIndex)->GetName().c_str());
  [NSNotificationCenter.defaultCenter postNotificationName:kBookmarksChangedNotification
                                                    object:nil
                                                  userInfo:nil];
  [self updateDistance];
}

- (void)removeBookmark
{
  [Statistics logEvent:kStatEventName(kStatPlacePage, kStatBookmarks)
                   withParameters:@{kStatValue : kStatRemove}];
  Framework & f = GetFramework();
  BookmarkCategory * bookmarkCategory = f.GetBookmarkManager().GetBmCategory(self.entity.bac.first);
  if (bookmarkCategory)
  {
    {
      BookmarkCategory::Guard guard(*bookmarkCategory);
      guard.m_controller.DeleteUserMark(self.entity.bac.second);
    }
    bookmarkCategory->SaveToKMLFile();
  }
  self.entity.bac = MakeEmptyBookmarkAndCategory();
  self.entity.bookmarkTitle = nil;
  self.entity.bookmarkCategory = nil;
  [NSNotificationCenter.defaultCenter postNotificationName:kBookmarksChangedNotification
                                                    object:nil
                                                  userInfo:nil];
  [self updateDistance];
}

- (void)reloadBookmark
{
  [self.entity synchronize];
  [self.placePage reloadBookmark];
  [self updateDistance];
}

- (void)dragPlacePage:(CGRect)frame
{
  [self.delegate dragPlacePage:frame];
}

- (void)onLocationUpdate:(location::GpsInfo const &)info
{
  [self updateDistance];
  [self updateMyPositionSpeedAndAltitude];
}

- (void)updateDistance
{
  NSString * distance = [self distance];
  self.directionView.distanceLabel.text = distance;
  [self.placePage setDistance:distance];
}

- (NSString *)distance
{
  CLLocation * location = [MapsAppDelegate theApp].locationManager.lastLocation;
  // TODO(AlexZ): Do we REALLY need this check? Why this method is called if user mark/m_info is empty?
  // TODO(AlexZ): Can location be checked before calling this method?
  if (!location/* || !m_userMark*/)
    return @"";
  string distance;
  CLLocationCoordinate2D const coord = location.coordinate;
  ms::LatLon const target = self.entity.latlon;
  MeasurementUtils::FormatDistance(ms::DistanceOnEarth(coord.latitude, coord.longitude,
                                                       target.lat, target.lon), distance);
  return @(distance.c_str());
}

- (void)onCompassUpdate:(location::CompassInfo const &)info
{
  CLLocation * location = [MapsAppDelegate theApp].locationManager.lastLocation;
  // TODO(AlexZ): Do we REALLY need this check? Why compass update is here if user mark/m_info is empty?
  // TODO(AlexZ): Can location be checked before calling this method?
  if (!location/* || !m_userMark*/)
    return;

  CGFloat const angle = ang::AngleTo(location.mercator, self.entity.mercator) + info.m_bearing;
  CGAffineTransform transform = CGAffineTransformMakeRotation(M_PI_2 - angle);
  [self.placePage setDirectionArrowTransform:transform];
  [self.directionView setDirectionArrowTransform:transform];
}

- (void)showDirectionViewWithTitle:(NSString *)title type:(NSString *)type
{
  MWMDirectionView * directionView = self.directionView;
  UIView * ownerView = self.ownerViewController.view;
  directionView.titleLabel.text = title;
  directionView.typeLabel.text = type;
  [ownerView addSubview:directionView];
  [ownerView endEditing:YES];
  [directionView setNeedsLayout];
  [self.delegate updateStatusBarStyle];
  [(MapsAppDelegate *)[UIApplication sharedApplication].delegate disableStandby];
  [self updateDistance];
}

- (void)hideDirectionView
{
  [self.directionView removeFromSuperview];
  [self.delegate updateStatusBarStyle];
  [(MapsAppDelegate *)[UIApplication sharedApplication].delegate enableStandby];
}

- (void)changeHeight:(CGFloat)height
{
  if (!IPAD)
    return;
  ((MWMiPadPlacePage *)self.placePage).height = height;
}

#pragma mark - Properties

- (MWMDirectionView *)directionView
{
  if (!_directionView)
    _directionView = [[MWMDirectionView alloc] initWithManager:self];
  return _directionView;
}

- (BOOL)isDirectionViewShown
{
  return self.directionView.superview != nil;
}

- (void)setTopBound:(CGFloat)topBound
{
  _topBound = self.placePage.topBound = topBound;
}

- (void)setLeftBound:(CGFloat)leftBound
{
  _leftBound = self.placePage.leftBound = leftBound;
}

@end
