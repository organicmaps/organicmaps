//
//  MWMMapViewControlsManager.m
//  Maps
//
//  Created by Ilya Grechuhin on 14.05.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import "Framework.h"
#import "MapsAppDelegate.h"
#import "MapViewController.h"
#import "MWMAPIBar.h"
#import "MWMLocationButton.h"
#import "MWMMapViewControlsManager.h"
#import "MWMPlacePageViewManager.h"
#import "MWMPlacePageViewManagerDelegate.h"
#import "MWMSideMenuManager.h"
#import "MWMSideMenuManagerDelegate.h"
#import "MWMZoomButtons.h"
#import "RouteState.h"

@interface MWMMapViewControlsManager () <MWMPlacePageViewManagerProtocol, MWMNavigationDashboardManagerProtocol,
                                         MWMSideMenuManagerProtocol>

@property (nonatomic) MWMZoomButtons * zoomButtons;
@property (nonatomic) MWMLocationButton * locationButton;
@property (nonatomic) MWMSideMenuManager * menuManager;
@property (nonatomic) MWMPlacePageViewManager * placePageManager;
@property (nonatomic) MWMNavigationDashboardManager * navigationManager;

@property (weak, nonatomic) MapViewController * ownerController;

@property (nonatomic) BOOL disableStandbyOnRouteFollowing;
@property (nonatomic) m2::PointD routeDestination;

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
  self.locationButton = [[MWMLocationButton alloc] initWithParentView:controller.view];
  self.menuManager = [[MWMSideMenuManager alloc] initWithParentController:controller delegate:self];
  self.placePageManager = [[MWMPlacePageViewManager alloc] initWithViewController:controller delegate:self];
  self.navigationManager = [[MWMNavigationDashboardManager alloc] initWithParentView:controller.view delegate:self];
  self.hidden = NO;
  self.zoomHidden = NO;
  self.menuState = MWMSideMenuStateInactive;
  return self;
}

#pragma mark - Layout

- (void)willRotateToInterfaceOrientation:(UIInterfaceOrientation)orientation
{
  [self.placePageManager willRotateToInterfaceOrientation:orientation];
  [self.navigationManager willRotateToInterfaceOrientation:orientation];
  if (!self.placePageManager.entity)
    return;
  if (UIInterfaceOrientationIsLandscape(orientation))
    [self.navigationManager hideHelperPanels];
  else
    [self.navigationManager showHelperPanels];
}

#pragma mark - MWMPlacePageViewManager

- (void)dismissPlacePage
{
  [self.placePageManager dismissPlacePage];
}

- (void)showPlacePageWithUserMark:(unique_ptr<UserMarkCopy>)userMark
{
  [self.placePageManager showPlacePageWithUserMark:move(userMark)];
  if (UIInterfaceOrientationIsLandscape(self.ownerController.interfaceOrientation))
    [self.navigationManager hideHelperPanels];
}

- (void)apiBack
{
  [self.ownerController.apiBar hideBarAndClearAnimated:NO];
}

#pragma mark - MWMSideMenuManagerProtocol

- (void)sideMenuDidUpdateLayout
{
  [self.zoomButtons setBottomBound:self.menuManager.menuButtonFrameWithSpacing.origin.y];
}

#pragma mark - MWMPlacePageViewManagerDelegate

- (void)dragPlacePage:(CGPoint)point
{
  CGFloat const bound = MIN(self.menuManager.menuButtonFrameWithSpacing.origin.y, point.y);
  [self.zoomButtons setBottomBound:bound];
}

- (void)placePageDidClose
{
  if (UIInterfaceOrientationIsLandscape(self.ownerController.interfaceOrientation))
    [self.navigationManager showHelperPanels];
}

- (void)addPlacePageViews:(NSArray *)views
{
  [self.ownerController addPlacePageViews:views];
}

- (void)updateStatusBarStyle
{
  [self.ownerController updateStatusBarStyle];
}

- (void)buildRoute:(m2::PointD)destination
{
  self.routeDestination = destination;
  // Determine route type
  [self buildRouteWithType:GetFramework().GetRouter()];
}

#pragma mark - MWMNavigationDashboardManager

- (void)setupRoutingDashboard:(location::FollowingInfo const &)info
{
  [self.navigationManager setupDashboard:info];
}

- (void)playTurnNotifications
{
  [self.navigationManager playTurnNotifications];
}

- (void)handleRoutingError
{
  self.navigationManager.state = MWMNavigationDashboardStateError;
}

- (void)buildRouteWithType:(enum routing::RouterType)type
{
  [[MapsAppDelegate theApp].m_locationManager start:self.navigationManager];
  self.navigationManager.state = MWMNavigationDashboardStatePlanning;
  GetFramework().BuildRoute(ToMercator([MapsAppDelegate theApp].m_locationManager.lastLocation.coordinate), self.routeDestination, 0 /* timeoutSec */);
  // This hack is needed to instantly show initial progress.
  // Because user may think that nothing happens when he is building a route.
  dispatch_async(dispatch_get_main_queue(), ^
  {
    CGFloat const initialRoutingProgress = 5.;
    [self setRouteBuildingProgress:initialRoutingProgress];
  });
}

- (void)navigationDashBoardDidUpdate
{
  CGFloat const topBound = self.topBound + self.navigationManager.height;
  [self.zoomButtons setTopBound:topBound];
  [self.placePageManager setTopBound:topBound];
}

- (void)didStartFollowing
{
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
  [RouteState remove];
}

- (void)routingReady
{
  self.navigationManager.state = MWMNavigationDashboardStateReady;
}

- (void)routingNavigation
{
  self.navigationManager.state = MWMNavigationDashboardStateNavigation;
  [self didStartFollowing];
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

#pragma mark - Properties

@synthesize menuState = _menuState;

- (void)setHidden:(BOOL)hidden
{
  if (_hidden == hidden)
    return;
  _hidden = hidden;
  self.zoomHidden = _zoomHidden;
  self.menuState = _menuState;
  self.locationHidden = _locationHidden;
  GetFramework().SetFullScreenMode(hidden);
}

- (void)setZoomHidden:(BOOL)zoomHidden
{
  _zoomHidden = zoomHidden;
  self.zoomButtons.hidden = self.hidden || zoomHidden;
}

- (void)setMenuState:(MWMSideMenuState)menuState
{
  _menuState = menuState;
  self.menuManager.state = self.hidden ? MWMSideMenuStateHidden : menuState;
}

- (MWMSideMenuState)menuState
{
  if (self.menuManager.state == MWMSideMenuStateActive)
    return MWMSideMenuStateActive;
  return _menuState;
}

- (MWMNavigationDashboardState)navigationState
{
  return self.navigationManager.state;
}

- (void)setLocationHidden:(BOOL)locationHidden
{
  _locationHidden = locationHidden;
  self.locationButton.hidden = self.hidden || locationHidden;
}

- (BOOL)isDirectionViewShown
{
  return self.placePageManager.isDirectionViewShown;
}

- (void)setTopBound:(CGFloat)topBound
{
  _topBound = topBound;
  self.placePageManager.topBound = self.zoomButtons.topBound = self.navigationManager.topBound = topBound;
}

@end
