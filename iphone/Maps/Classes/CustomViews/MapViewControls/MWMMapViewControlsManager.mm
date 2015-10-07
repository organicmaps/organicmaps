#import "CountryTreeVC.h"
#import "MapsAppDelegate.h"
#import "MapViewController.h"
#import "MWMAlertViewController.h"
#import "MWMAPIBar.h"
#import "MWMBottomMenuViewController.h"
#import "MWMMapViewControlsManager.h"
#import "MWMPlacePageViewManager.h"
#import "MWMPlacePageViewManagerDelegate.h"
#import "MWMRoutePreview.h"
#import "MWMSearchManager.h"
#import "MWMSearchView.h"
#import "MWMZoomButtons.h"
#import "RouteState.h"

#import "3party/Alohalytics/src/alohalytics_objc.h"

#include "Framework.h"

extern NSString * const kAlohalyticsTapEventKey;

@interface MWMMapViewControlsManager ()<
    MWMPlacePageViewManagerProtocol, MWMNavigationDashboardManagerProtocol,
    MWMSearchManagerProtocol, MWMSearchViewProtocol, MWMBottomMenuControllerProtocol, MWMRoutePreviewDataSource>

@property (nonatomic) MWMZoomButtons * zoomButtons;
@property (nonatomic) MWMBottomMenuViewController * menuController;
@property (nonatomic) MWMPlacePageViewManager * placePageManager;
@property (nonatomic) MWMNavigationDashboardManager * navigationManager;
@property (nonatomic) MWMSearchManager * searchManager;

@property (weak, nonatomic) MapViewController * ownerController;

@property (nonatomic) BOOL disableStandbyOnRouteFollowing;
@property (nonatomic) MWMRoutePoint routeSource;
@property (nonatomic) MWMRoutePoint routeDestination;

@property (nonatomic) CGFloat topBound;
@property (nonatomic) CGFloat leftBound;

@end

@implementation MWMMapViewControlsManager

- (instancetype)initWithParentController:(MapViewController *)controller
{
  if (!controller)
    return nil;
  self = [super init];
  if (!self)
    return nil;
  self.ownerController = controller;
  self.zoomButtons = [[MWMZoomButtons alloc] initWithParentView:controller.view];
  self.menuController = [[MWMBottomMenuViewController alloc] initWithParentController:controller delegate:self];
  self.placePageManager = [[MWMPlacePageViewManager alloc] initWithViewController:controller delegate:self];
  self.navigationManager = [[MWMNavigationDashboardManager alloc] initWithParentView:controller.view delegate:self];
  self.searchManager = [[MWMSearchManager alloc] initWithParentView:controller.view delegate:self];
  self.hidden = NO;
  self.zoomHidden = NO;
  self.menuState = MWMBottomMenuStateInactive;
  self.routeSource = {ToMercator(MapsAppDelegate.theApp.m_locationManager.lastLocation.coordinate)};
  self.routeDestination = MWMRoutePoint::MWMRoutePointZero();
  return self;
}

- (void)onEnterForeground
{
  [self.menuController onEnterForeground];
}

#pragma mark - Layout

- (void)willRotateToInterfaceOrientation:(UIInterfaceOrientation)toInterfaceOrientation
                                duration:(NSTimeInterval)duration
{
  [self.menuController willRotateToInterfaceOrientation:toInterfaceOrientation duration:duration];
  [self.placePageManager willRotateToInterfaceOrientation:toInterfaceOrientation];
  [self.navigationManager willRotateToInterfaceOrientation:toInterfaceOrientation];
  [self.searchManager willRotateToInterfaceOrientation:toInterfaceOrientation duration:duration];
  [self refreshHelperPanels:UIInterfaceOrientationIsLandscape(toInterfaceOrientation)];
}

- (void)viewWillTransitionToSize:(CGSize)size
       withTransitionCoordinator:(id<UIViewControllerTransitionCoordinator>)coordinator
{
  [self.menuController viewWillTransitionToSize:size withTransitionCoordinator:coordinator];
  [self.placePageManager viewWillTransitionToSize:size withTransitionCoordinator:coordinator];
  [self.navigationManager viewWillTransitionToSize:size withTransitionCoordinator:coordinator];
  [self.searchManager viewWillTransitionToSize:size withTransitionCoordinator:coordinator];
  [self refreshHelperPanels:size.height < size.width];
}

- (void)refreshHelperPanels:(BOOL)isLandscape
{
  if (!self.placePageManager.entity)
    return;
  if (isLandscape)
    [self.navigationManager hideHelperPanels];
  else
    [self.navigationManager showHelperPanels];
}

#pragma mark - MWMPlacePageViewManager

- (void)dismissPlacePage
{
  [self.placePageManager hidePlacePage];
}

- (void)showPlacePageWithUserMark:(unique_ptr<UserMarkCopy>)userMark
{
  [self.placePageManager showPlacePageWithUserMark:move(userMark)];
  [self refreshHelperPanels:UIInterfaceOrientationIsLandscape(self.ownerController.interfaceOrientation)];
}

- (void)apiBack
{
  [self.ownerController.apiBar back];
}

#pragma mark - MWMSearchManagerProtocol

- (void)searchViewDidEnterState:(MWMSearchManagerState)state
{
  if (state == MWMSearchManagerStateHidden)
  {
    self.hidden = NO;
    self.leftBound = self.topBound = 0.0;
  }
  [self.ownerController setNeedsStatusBarAppearanceUpdate];
  if (IPAD)
  {
    MWMRoutingPlaneMode const m = MapsAppDelegate.theApp.routingPlaneMode;
    if (m != MWMRoutingPlaneModeNone)
    {
      [UIView animateWithDuration:0.15 animations:^{
        self.navigationManager.routePreview.alpha = state == MWMSearchManagerStateHidden ? 1. : 0.;
      }];
    }
  }
}

#pragma mark - MWMSearchViewProtocol

- (void)searchFrameUpdated:(CGRect)frame
{
  CGSize const s = frame.size;
  self.leftBound = s.width;
  self.topBound = s.height;
}

#pragma mark - MWMSearchManagerProtocol & MWMBottomMenuControllerProtocol

- (void)actionDownloadMaps
{
  [Alohalytics logEvent:kAlohalyticsTapEventKey withValue:@"downloader"];
  CountryTreeVC * vc = [[CountryTreeVC alloc] initWithNodePosition:-1];
  [self.ownerController.navigationController pushViewController:vc animated:YES];
}

#pragma mark - MWMBottomMenuControllerProtocol

- (void)closeInfoScreens
{
  if (IPAD)
  {
    self.searchManager.state = MWMSearchManagerStateHidden;
  }
  else
  {
    CGSize const ownerViewSize = self.ownerController.view.size;
    if (ownerViewSize.width > ownerViewSize.height)
      [self.placePageManager hidePlacePage];
  }
}

#pragma mark - MWMPlacePageViewManagerDelegate

- (void)dragPlacePage:(CGRect)frame
{
  if (IPAD)
    return;
  CGSize const ownerViewSize = self.ownerController.view.size;
  if (ownerViewSize.width > ownerViewSize.height)
    self.menuController.leftBound = frame.origin.x + frame.size.width;
  else
    [self.zoomButtons setBottomBound:frame.origin.y];
}

- (void)placePageDidClose
{
  if (UIInterfaceOrientationIsLandscape(self.ownerController.interfaceOrientation))
    [self.navigationManager showHelperPanels];
}

- (void)addPlacePageViews:(NSArray *)views
{
  UIView * ownerView = self.ownerController.view;
  for (UIView * view in views)
    [ownerView addSubview:view];
  [ownerView bringSubviewToFront:self.searchManager.view];
  if (IPAD)
    [ownerView bringSubviewToFront:self.menuController.view];
}

- (void)updateStatusBarStyle
{
  [self.ownerController updateStatusBarStyle];
}

- (void)setupBestRouter
{
  auto & f = GetFramework();
  MWMRoutePoint const zero = MWMRoutePoint::MWMRoutePointZero();
  if (self.isPossibleToBuildRoute)
    f.SetRouter(f.GetBestRouter(self.routeSource.point, self.routeDestination.point));
}

- (void)buildRouteFrom:(MWMRoutePoint const &)from to:(MWMRoutePoint const &)to
{
  self.routeSource = from;
  self.routeDestination = to;
  self.navigationManager.routePreview.extendButton.selected = NO;
  [self.navigationManager.routePreview reloadData];
  [self setupBestRouter];
  [self buildRoute];
}

- (void)buildRouteFrom:(MWMRoutePoint const &)from
{
  self.routeSource = from;
  if (from.isMyPosition && self.routeDestination.isMyPosition)
    self.routeDestination = MWMRoutePoint::MWMRoutePointZero();

  [self.navigationManager.routePreview reloadData];
  [self buildRoute];
}

- (void)buildRouteTo:(MWMRoutePoint const &)to
{
  self.routeDestination = to;
  if (to.isMyPosition && self.routeSource.isMyPosition)
    self.routeSource = MWMRoutePoint::MWMRoutePointZero();

  [self.navigationManager.routePreview reloadData];
  [self buildRoute];
}

#pragma mark - MWMNavigationDashboardManager

- (void)routePreviewDidChangeFrame:(CGRect)newFrame
{
  if (IPAD)
  {
    CGFloat const bound = newFrame.origin.x + newFrame.size.width;
    if (self.searchManager.state == MWMSearchManagerStateHidden)
    {
      self.placePageManager.leftBound = self.menuController.leftBound = newFrame.origin.x + newFrame.size.width;
    }
    else
    {
      [UIView animateWithDuration:0.15 animations:^
      {
        self.searchManager.view.alpha = bound > 0 ? 0. : 1.;
      }];
    }
  }
  else
  {
    CGFloat const bound = newFrame.origin.y + newFrame.size.height;
    self.placePageManager.topBound = self.zoomButtons.topBound = bound;
  }
}

- (void)setupRoutingDashboard:(location::FollowingInfo const &)info
{
  [self.navigationManager setupDashboard:info];
  if (self.menuController.state == MWMBottomMenuStateText)
    [self.menuController setStreetName:@(info.m_sourceName.c_str())];
}

- (void)playTurnNotifications
{
  [self.navigationManager playTurnNotifications];
}

- (void)handleRoutingError
{
  self.navigationManager.state = MWMNavigationDashboardStateError;
  MapsAppDelegate.theApp.routingPlaneMode = MWMRoutingPlaneModePlacePage;
}

- (void)buildRoute
{
  LocationManager * locMgr = [MapsAppDelegate theApp].m_locationManager;
  if (![locMgr lastLocationIsValid])
  {
    MWMAlertViewController * alert =
        [[MWMAlertViewController alloc] initWithViewController:self.ownerController];
    [alert presentLocationAlert];
    return;
  }

  if (!self.isPossibleToBuildRoute)
    return;

  [locMgr start:self.navigationManager];
  MapsAppDelegate.theApp.routingPlaneMode = MWMRoutingPlaneModePlacePage;
  m2::PointD const locationPoint = ToMercator(locMgr.lastLocation.coordinate);
  if (self.routeSource.isMyPosition)
    self.routeSource.point = locationPoint;
  if (self.routeDestination.isMyPosition)
    self.routeDestination.point = locationPoint;

  self.navigationManager.state = MWMNavigationDashboardStatePlanning;
  [self.menuController setPlanning];
  GetFramework().BuildRoute(self.routeSource.point, self.routeDestination.point, 0 /* timeoutSec */);
  // This hack is needed to instantly show initial progress.
  // Because user may think that nothing happens when he is building a route.
  dispatch_async(dispatch_get_main_queue(), ^
  {
    CGFloat const initialRoutingProgress = 5.;
    [self setRouteBuildingProgress:initialRoutingProgress];
  });
}

- (BOOL)isPossibleToBuildRoute
{
  MWMRoutePoint const zeroPoint = MWMRoutePoint::MWMRoutePointZero();
  return self.routeSource != zeroPoint && self.routeDestination != zeroPoint;
}

- (void)navigationDashBoardDidUpdate
{
  CGFloat const topBound = self.topBound + self.navigationManager.height;
  if (!IPAD)
    [self.zoomButtons setTopBound:topBound];
  [self.placePageManager setTopBound:topBound];
}

- (void)didStartFollowing
{
  self.hidden = NO;
  self.zoomHidden = NO;
  GetFramework().FollowRoute();
  self.disableStandbyOnRouteFollowing = YES;
  [RouteState save];
}

- (void)didCancelRouting
{
  [[MapsAppDelegate theApp].m_locationManager stop:self.navigationManager];
  GetFramework().CloseRouting();
  self.disableStandbyOnRouteFollowing = NO;
  [MapsAppDelegate theApp].routingPlaneMode = MWMRoutingPlaneModeNone;
  [RouteState remove];
  [self.menuController setInactive];
  [self resetRoutingPoint];
}

- (void)swapPointsAndRebuildRouteIfPossible
{
  MWMRoutePoint::Swap(_routeSource, _routeDestination);
  [self buildRoute];
}

- (void)didStartEditingRoutePoint:(BOOL)isSource
{
  MapsAppDelegate.theApp.routingPlaneMode = isSource ? MWMRoutingPlaneModeSearchSource : MWMRoutingPlaneModeSearchDestination;
  self.searchManager.state = MWMSearchManagerStateDefault;
}

- (void)resetRoutingPoint
{
  self.routeSource = {ToMercator(MapsAppDelegate.theApp.m_locationManager.lastLocation.coordinate)};
  self.routeDestination = MWMRoutePoint::MWMRoutePointZero();
}

- (void)routingHidden
{
  self.navigationManager.state = MWMNavigationDashboardStateHidden;
  [self.menuController setInactive];
  [self resetRoutingPoint];
}

- (void)routingReady
{
  self.navigationManager.state = MWMNavigationDashboardStateReady;
  [self.menuController setGo];
}

- (void)routingPrepare
{
  self.navigationManager.state = MWMNavigationDashboardStatePrepare;
  MapsAppDelegate.theApp.routingPlaneMode = MWMRoutingPlaneModePlacePage;
}

- (void)routingNavigation
{
  self.navigationManager.state = MWMNavigationDashboardStateNavigation;
  MapsAppDelegate.theApp.routingPlaneMode = MWMRoutingPlaneModeNone;
  [self didStartFollowing];
  [self.menuController setStreetName:@""];
}

- (void)setDisableStandbyOnRouteFollowing:(BOOL)disableStandbyOnRouteFollowing
{
  if (_disableStandbyOnRouteFollowing == disableStandbyOnRouteFollowing)
    return;
  _disableStandbyOnRouteFollowing = disableStandbyOnRouteFollowing;
  if (disableStandbyOnRouteFollowing)
    [[MapsAppDelegate theApp] disableStandby];
  else
    [[MapsAppDelegate theApp] enableStandby];
}

- (void)setRouteBuildingProgress:(CGFloat)progress
{
  [self.navigationManager setRouteBuildingProgress:progress];
}

#pragma mark - MWMRoutePreviewDataSource

- (NSString *)source
{
  return self.routeSource.name;
}

- (NSString *)destination
{
  return self.routeDestination.name;
}

#pragma mark - Properties

@synthesize menuState = _menuState;

- (void)setHidden:(BOOL)hidden
{
  if (_hidden == hidden)
    return;
  _hidden = hidden;
  self.zoomHidden = _zoomHidden;
  self.menuState = _menuState;
  GetFramework().SetFullScreenMode(hidden);
}

- (void)setZoomHidden:(BOOL)zoomHidden
{
  _zoomHidden = zoomHidden;
  self.zoomButtons.hidden = self.hidden || zoomHidden;
}

- (void)setMenuState:(MWMBottomMenuState)menuState
{
  _menuState = menuState;
  self.menuController.state = self.hidden ? MWMBottomMenuStateHidden : menuState;
}

- (MWMBottomMenuState)menuState
{
  MWMBottomMenuState const state = self.menuController.state;
  if (state != MWMBottomMenuStateHidden)
    return state;
  return _menuState;
}

- (MWMNavigationDashboardState)navigationState
{
  return self.navigationManager.state;
}

- (BOOL)isDirectionViewShown
{
  return self.placePageManager.isDirectionViewShown;
}

- (void)setTopBound:(CGFloat)topBound
{
  if (IPAD)
    return;
  _topBound = self.placePageManager.topBound = self.zoomButtons.topBound = self.navigationManager.topBound = topBound;
}

- (void)setLeftBound:(CGFloat)leftBound
{
  if (!IPAD)
    return;
  MWMRoutingPlaneMode const m = MapsAppDelegate.theApp.routingPlaneMode;
  if (m != MWMRoutingPlaneModeNone)
    return;
  _leftBound = self.placePageManager.leftBound = self.navigationManager.leftBound = self.menuController.leftBound = leftBound;
}

- (BOOL)searchHidden
{
  return self.searchManager.state == MWMSearchManagerStateHidden;
}

- (void)setSearchHidden:(BOOL)searchHidden
{
  self.searchManager.state = searchHidden ? MWMSearchManagerStateHidden : MWMSearchManagerStateDefault;
}

@end
