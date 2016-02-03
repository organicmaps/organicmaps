#import "Common.h"
#import "LocationManager.h"
#import "MapsAppDelegate.h"
#import "MWMActivityViewController.h"
#import "MWMAPIBar.h"
#import "MWMBasePlacePageView.h"
#import "MWMDirectionView.h"
#import "MWMFrameworkListener.h"
#import "MWMiPadPlacePage.h"
#import "MWMiPhoneLandscapePlacePage.h"
#import "MWMiPhonePortraitPlacePage.h"
#import "MWMPlacePage.h"
#import "MWMPlacePageActionBar.h"
#import "MWMPlacePageEntity.h"
#import "MWMPlacePageNavigationBar.h"
#import "MWMPlacePageViewManager.h"
#import "MWMPlacePageViewManagerDelegate.h"
#import "Statistics.h"

#import "3party/Alohalytics/src/alohalytics_objc.h"

#include "geometry/distance_on_sphere.hpp"
#include "platform/measurement_utils.hpp"

extern NSString * const kAlohalyticsTapEventKey;
extern NSString * const kBookmarksChangedNotification;

typedef NS_ENUM(NSUInteger, MWMPlacePageManagerState)
{
  MWMPlacePageManagerStateClosed,
  MWMPlacePageManagerStateOpen
};

@interface MWMPlacePageViewManager () <LocationObserver>

@property (weak, nonatomic) UIViewController * ownerViewController;
@property (nonatomic, readwrite) MWMPlacePageEntity * entity;
@property (nonatomic) MWMPlacePage * placePage;
@property (nonatomic) MWMPlacePageManagerState state;
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
    self.state = MWMPlacePageManagerStateClosed;
  }
  return self;
}

- (void)hidePlacePage
{
  [self.placePage hide];
}

- (void)dismissPlacePage
{
  MWMFrameworkListener * listener = [MWMFrameworkListener listener];
  if (!listener.userMark)
    return;
  [self.delegate placePageDidClose];
  self.state = MWMPlacePageManagerStateClosed;
  [self.placePage dismiss];
  [[MapsAppDelegate theApp].m_locationManager stop:self];
  listener.userMark = nullptr;
  GetFramework().DeactivateUserMark();
  self.placePage = nil;
}

- (void)showPlacePage
{
  [[MapsAppDelegate theApp].m_locationManager start:self];
  [self reloadPlacePage];
}

- (void)reloadPlacePage
{
  MWMFrameworkListener * listener = [MWMFrameworkListener listener];
  if (!listener.userMark)
    return;
  self.entity = [[MWMPlacePageEntity alloc] init];
  self.state = MWMPlacePageManagerStateOpen;
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
  if (self.entity.type == MWMPlacePageEntityTypeMyPosition)
  {
    BOOL hasSpeed;
    self.entity.category = [[MapsAppDelegate theApp].m_locationManager formattedSpeedAndAltitude:hasSpeed];
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
  if (self.entity.type != MWMPlacePageEntityTypeMyPosition)
    return;
  BOOL hasSpeed = NO;
  [self.placePage updateMyPositionStatus:[[MapsAppDelegate theApp].m_locationManager
                                          formattedSpeedAndAltitude:hasSpeed]];
}

- (void)setPlacePageForiPhoneWithOrientation:(UIInterfaceOrientation)orientation
{
  if (self.state == MWMPlacePageManagerStateClosed)
    return;

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
  [[Statistics instance] logEvent:kStatEventName(kStatPlacePage, kStatBuildRoute)
                   withParameters:@{kStatValue : kStatDestination}];
  [Alohalytics logEvent:kAlohalyticsTapEventKey withValue:@"ppRoute"];
  MWMFrameworkListener * listener = [MWMFrameworkListener listener];
  m2::PointD const & destination = listener.userMark->GetPivot();
  m2::PointD const myPosition([MapsAppDelegate theApp].m_locationManager.lastLocation.mercator);
  using namespace location;
  auto const mode = listener.myPositionMode;
  [self.delegate buildRouteFrom:mode != EMyPositionMode::MODE_UNKNOWN_POSITION &&
                                        mode != EMyPositionMode::MODE_PENDING_POSITION
                                    ? MWMRoutePoint(myPosition)
                                    : MWMRoutePoint::MWMRoutePointZero()
                             to:{destination, self.placePage.basePlacePageView.titleLabel.text}];
}

- (void)routeFrom
{
  [[Statistics instance] logEvent:kStatEventName(kStatPlacePage, kStatBuildRoute)
                   withParameters:@{kStatValue : kStatSource}];
  [Alohalytics logEvent:kAlohalyticsTapEventKey withValue:@"ppRoute"];
  [self.delegate buildRouteFrom:self.target];
  [self hidePlacePage];
}

- (void)routeTo
{
  [[Statistics instance] logEvent:kStatEventName(kStatPlacePage, kStatBuildRoute)
                   withParameters:@{kStatValue : kStatDestination}];
  [Alohalytics logEvent:kAlohalyticsTapEventKey withValue:@"ppRoute"];
  [self.delegate buildRouteTo:self.target];
  [self hidePlacePage];
}

- (MWMRoutePoint)target
{
  UserMark const * m = [MWMFrameworkListener listener].userMark;
  m2::PointD const & org = m->GetPivot();
  return m->GetMarkType() == UserMark::Type::MY_POSITION ?
                          MWMRoutePoint(org) :
                          MWMRoutePoint(org, self.placePage.basePlacePageView.titleLabel.text);
}

- (void)share
{
  [[Statistics instance] logEvent:kStatEventName(kStatPlacePage, kStatShare)];
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
  [[Statistics instance] logEvent:kStatEventName(kStatPlacePage, kStatAPI)];
  ApiMarkPoint const * p = static_cast<ApiMarkPoint const *>([MWMFrameworkListener listener].userMark);
  NSURL * url = [NSURL URLWithString:@(GetFramework().GenerateApiBackUrl(*p).c_str())];
  [[UIApplication sharedApplication] openURL:url];
  [self.delegate apiBack];
}

- (void)changeBookmarkCategory:(BookmarkAndCategory)bac;
{
  BookmarkCategory * category = GetFramework().GetBmCategory(bac.first);
  BookmarkCategory::Guard guard(*category);
  [MWMFrameworkListener listener].userMark = guard.m_controller.GetUserMark(bac.second);
}

- (void)editPlace
{
  [[Statistics instance] logEvent:kStatEventName(kStatPlacePage, kStatEdit)];
  [self.ownerViewController performSegueWithIdentifier:@"Map2EditorSegue" sender:self.entity];
}

- (void)addBookmark
{
  [[Statistics instance] logEvent:kStatEventName(kStatPlacePage, kStatBookmarks)
                   withParameters:@{kStatValue : kStatAdd}];
  Framework & f = GetFramework();
  BookmarkData data = BookmarkData(self.entity.title.UTF8String, f.LastEditedBMType());
  MWMFrameworkListener * listener = [MWMFrameworkListener listener];
  size_t const categoryIndex = f.LastEditedBMCategory();
  m2::PointD const mercator = listener.userMark->GetPivot();
  size_t const bookmarkIndex = f.GetBookmarkManager().AddBookmark(categoryIndex, mercator, data);
  self.entity.bac = make_pair(categoryIndex, bookmarkIndex);
  self.entity.type = MWMPlacePageEntityTypeBookmark;

  BookmarkCategory::Guard guard(*f.GetBmCategory(categoryIndex));

  UserMark const * bookmark = guard.m_controller.GetUserMark(bookmarkIndex);
  // TODO(AlexZ): Refactor bookmarks code together to hide this code in the Framework/Drape.
  // UI code should never know about any guards, pointers to UserMark etc.
  const_cast<UserMark *>(bookmark)->SetFeature(f.GetFeatureAtPoint(mercator));
  listener.userMark = bookmark;
  [NSNotificationCenter.defaultCenter postNotificationName:kBookmarksChangedNotification
                                                    object:nil
                                                  userInfo:nil];
  [self updateDistance];
}

- (void)removeBookmark
{
  [[Statistics instance] logEvent:kStatEventName(kStatPlacePage, kStatBookmarks)
                   withParameters:@{kStatValue : kStatRemove}];
  Framework & f = GetFramework();
  BookmarkAndCategory bookmarkAndCategory = self.entity.bac;
  BookmarkCategory * bookmarkCategory = f.GetBookmarkManager().GetBmCategory(bookmarkAndCategory.first);
  if (!bookmarkCategory)
    return;

  UserMark const * bookmark = bookmarkCategory->GetUserMark(bookmarkAndCategory.second);
  ASSERT_EQUAL(bookmarkAndCategory, f.FindBookmark(bookmark), ());

  self.entity.type = MWMPlacePageEntityTypeRegular;

  // TODO(AlexZ): SetFeature is called in GetAddressMark here.
  // UI code should never know about any guards, pointers to UserMark etc.
  [MWMFrameworkListener listener].userMark = f.GetAddressMark(bookmark->GetPivot());
  if (bookmarkCategory)
  {
    {
      BookmarkCategory::Guard guard(*bookmarkCategory);
      guard.m_controller.DeleteUserMark(bookmarkAndCategory.second);
    }
    bookmarkCategory->SaveToKMLFile();
  }
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
  CLLocation * location = [MapsAppDelegate theApp].m_locationManager.lastLocation;
  UserMark const * userMark = [MWMFrameworkListener listener].userMark;
  if (!location || !userMark)
    return @"";
  string distance;
  CLLocationCoordinate2D const coord = location.coordinate;
  ms::LatLon const target = MercatorBounds::ToLatLon(userMark->GetPivot());
  MeasurementUtils::FormatDistance(ms::DistanceOnEarth(coord.latitude, coord.longitude,
                                                       target.lat, target.lon), distance);
  return @(distance.c_str());
}

- (void)onCompassUpdate:(location::CompassInfo const &)info
{
  CLLocation * location = [MapsAppDelegate theApp].m_locationManager.lastLocation;
  UserMark const * userMark = [MWMFrameworkListener listener].userMark;
  if (!location || !userMark)
    return;

  CGFloat const angle = ang::AngleTo(location.mercator, userMark->GetPivot()) + info.m_bearing;
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
