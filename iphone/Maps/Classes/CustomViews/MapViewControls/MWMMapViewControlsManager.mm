//
//  MWMMapViewControlsManager.m
//  Maps
//
//  Created by Ilya Grechuhin on 14.05.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import "Framework.h"
#import "MapViewController.h"
#import "MWMLocationButton.h"
#import "MWMMapViewControlsManager.h"
#import "MWMNavigationDashboardManager.h"
#import "MWMPlacePageViewManager.h"
#import "MWMPlacePageViewManagerDelegate.h"
#import "MWMSideMenuManager.h"
#import "MWMZoomButtons.h"

@interface MWMMapViewControlsManager () <MWMPlacePageViewManagerDelegate, MWMNavigationDashboardManagerDelegate>

@property (nonatomic) MWMZoomButtons * zoomButtons;
@property (nonatomic) MWMLocationButton * locationButton;
@property (nonatomic) MWMSideMenuManager * menuManager;
@property (nonatomic) MWMPlacePageViewManager * placePageManager;
@property (nonatomic) MWMNavigationDashboardManager * navigationManager;

@property (weak, nonatomic) MapViewController * ownerController;

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
  self.menuManager = [[MWMSideMenuManager alloc] initWithParentController:controller];
  self.placePageManager = [[MWMPlacePageViewManager alloc] initWithViewController:controller delegate:self];
  self.navigationManager = [[MWMNavigationDashboardManager alloc] initWithParentView:controller.view delegate:self];
  self.hidden = NO;
  self.zoomHidden = NO;
  self.menuState = MWMSideMenuStateInactive;
  return self;
}

- (void)setTopBound:(CGFloat)bound
{
  [self.zoomButtons setTopBound:bound];
  [self.placePageManager setTopBound:bound];
}

#pragma mark - Layout

- (void)willRotateToInterfaceOrientation:(UIInterfaceOrientation)orientation
{
  [self.placePageManager willRotateToInterfaceOrientation:orientation];
}

#pragma mark - MWMPlacePageViewManager

- (void)dismissPlacePage
{
  [self.placePageManager dismissPlacePage];
}

- (void)showPlacePageWithUserMark:(unique_ptr<UserMarkCopy>)userMark
{
  [self.placePageManager showPlacePageWithUserMark:std::move(userMark)];
}

- (void)stopBuildingRoute
{
  [self.placePageManager stopBuildingRoute];
}

#pragma mark - MWMPlacePageViewManagerDelegate

- (void)dragPlacePage:(CGPoint)point
{
  [self.zoomButtons setBottomBound:point.y];
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
  enum MWMNavigationRouteType type = MWMNavigationRouteTypeVehicle;
  [self buildRouteWithType:type];
}

#pragma mark - MWMNavigationDashboardManager

- (void)buildRouteWithType:(enum MWMNavigationRouteType)type
{
  GetFramework().BuildRoute(self.routeDestination, 0 /* timeoutSec */);
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

- (void)setLocationHidden:(BOOL)locationHidden
{
  _locationHidden = locationHidden;
  self.locationButton.hidden = self.hidden || locationHidden;
}

- (BOOL)isDirectionViewShown
{
  return self.placePageManager.isDirectionViewShown;
}

@end
