//
//  MWMNavigationDashboardManager.m
//  Maps
//
//  Created by v.mikhaylenko on 20.07.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import "Macros.h"
#import "MapsAppDelegate.h"
#import "MWMNavigationDashboard.h"
#import "MWMNavigationDashboardEntity.h"
#import "MWMNavigationDashboardManager.h"
#import "MWMRoutePreview.h"

@interface MWMNavigationDashboardManager ()

@property (nonatomic) IBOutlet MWMRoutePreview * routePreviewLandscape;
@property (nonatomic) IBOutlet MWMRoutePreview * routePreviewPortrait;
@property (weak, nonatomic) MWMRoutePreview * routePreview;

@property (nonatomic) IBOutlet MWMNavigationDashboard * navigationDashboardLandscape;
@property (nonatomic) IBOutlet MWMNavigationDashboard * navigationDashboardPortrait;
@property (weak, nonatomic) MWMNavigationDashboard * navigationDashboard;

@property (weak, nonatomic) UIView * ownerView;
@property (weak, nonatomic) id<MWMNavigationDashboardManagerProtocol> delegate;

@property (nonatomic, readwrite) MWMNavigationDashboardEntity * entity;
@end

@implementation MWMNavigationDashboardManager

- (instancetype)initWithParentView:(UIView *)view delegate:(id<MWMNavigationDashboardManagerProtocol>)delegate
{
  self = [super init];
  if (self)
  {
    self.ownerView = view;
    self.delegate = delegate;
    BOOL const isPortrait = self.ownerView.width < self.ownerView.height;

    [NSBundle.mainBundle loadNibNamed:@"MWMPortraitRoutePreview" owner:self options:nil];
    [NSBundle.mainBundle loadNibNamed:@"MWMLandscapeRoutePreview" owner:self options:nil];
    self.routePreview = isPortrait ? self.routePreviewPortrait : self.routePreviewLandscape;
    self.routePreviewPortrait.delegate = self.routePreviewLandscape.delegate = delegate;

    [NSBundle.mainBundle loadNibNamed:@"MWMPortraitNavigationDashboard" owner:self options:nil];
    [NSBundle.mainBundle loadNibNamed:@"MWMLandscapeNavigationDashboard" owner:self options:nil];
    self.navigationDashboard = isPortrait ? self.navigationDashboardPortrait : self.navigationDashboardLandscape;
    self.navigationDashboardPortrait.delegate = self.navigationDashboardLandscape.delegate = delegate;
  }
  return self;
}

#pragma mark - Layout

- (void)willRotateToInterfaceOrientation:(UIInterfaceOrientation)orientation
{
  BOOL const isPortrait = orientation == UIInterfaceOrientationPortrait ||
                          orientation == UIInterfaceOrientationPortraitUpsideDown;
  MWMRoutePreview * routePreview = isPortrait ? self.routePreviewPortrait : self.routePreviewLandscape;
  if (self.routePreview.isVisible && ![routePreview isEqual:self.routePreview])
  {
    [self.routePreview remove];
    [routePreview addToView:self.ownerView];
  }
  self.routePreview = routePreview;

  MWMNavigationDashboard * navigationDashboard = isPortrait ? self.navigationDashboardPortrait :
                                                              self.navigationDashboardLandscape;
  if (self.navigationDashboard.isVisible && ![navigationDashboard isEqual:self.navigationDashboard])
  {
    [self.navigationDashboard remove];
    [navigationDashboard addToView:self.ownerView];
  }
  self.navigationDashboard = navigationDashboard;
}

- (void)setupDashboard:(location::FollowingInfo const &)info
{
  if (!self.entity)
    self.entity = [[MWMNavigationDashboardEntity alloc] initWithFollowingInfo:info];
  else
    [self.entity updateWithFollowingInfo:info];
  [self updateDashboard];
}

- (void)handleError
{
  [self.routePreviewPortrait stateError];
  [self.routePreviewLandscape stateError];
}

- (void)updateDashboard
{
  [self.routePreviewLandscape configureWithEntity:self.entity];
  [self.routePreviewPortrait configureWithEntity:self.entity];
  [self.navigationDashboardLandscape configureWithEntity:self.entity];
  [self.navigationDashboardPortrait configureWithEntity:self.entity];
}

#pragma mark - MWMRoutePreview

- (IBAction)routePreviewChange:(UIButton *)sender
{
  if (sender.selected)
    return;
  sender.selected = YES;
  auto & f = GetFramework();
  if ([sender isEqual:self.routePreview.pedestrian])
  {
    self.routePreview.vehicle.selected = NO;
    f.SetRouter(routing::RouterType::Pedestrian);
  }
  else
  {
    self.routePreview.pedestrian.selected = NO;
    f.SetRouter(routing::RouterType::Vehicle);
  }
  f.CloseRouting();
  [self showStatePlanning];
  [self.delegate buildRouteWithType:f.GetRouter()];
}

- (void)setRouteBuildingProgress:(CGFloat)progress
{
  [self.routePreviewLandscape setRouteBuildingProgress:progress];
  [self.routePreviewPortrait setRouteBuildingProgress:progress];
}

#pragma mark - MWMNavigationDashboard

- (IBAction)navigationCancelPressed:(UIButton *)sender
{
  self.state = MWMNavigationDashboardStateHidden;
  [self.delegate didCancelRouting];
}

#pragma mark - MWMNavigationGo

- (IBAction)navigationGoPressed:(UIButton *)sender
{
  self.state = MWMNavigationDashboardStateNavigation;
  [self.delegate didStartFollowing];
}

#pragma mark - State changes

- (void)hideState
{
  [self.routePreview remove];
  [self.navigationDashboard remove];
}

- (void)showStatePlanning
{
  [self.navigationDashboard remove];
  [self.routePreview addToView:self.ownerView];
  [self.routePreviewLandscape statePlaning];
  [self.routePreviewPortrait statePlaning];
  auto const state = GetFramework().GetRouter();
  switch (state)
  {
    case routing::RouterType::Pedestrian:
      self.routePreviewLandscape.pedestrian.selected = YES;
      self.routePreviewPortrait.pedestrian.selected = YES;
      self.routePreviewPortrait.vehicle.selected = NO;
      self.routePreviewLandscape.vehicle.selected = NO;
      break;
    case routing::RouterType::Vehicle:
      self.routePreviewLandscape.vehicle.selected = YES;
      self.routePreviewPortrait.vehicle.selected = YES;
      self.routePreviewLandscape.pedestrian.selected = NO;
      self.routePreviewPortrait.pedestrian.selected = NO;
      break;
  }
}

- (void)showStateReady
{
  [self showGoButton:YES];
}

- (void)showStateNavigation
{
  [self.routePreview remove];
  [self.navigationDashboard addToView:self.ownerView];
}

- (void)showGoButton:(BOOL)show
{
  [self.routePreviewPortrait showGoButtonAnimated:show];
  [self.routePreviewLandscape showGoButtonAnimated:show];
}

#pragma mark - Properties

- (void)setState:(MWMNavigationDashboardState)state
{
  if (_state == state && state != MWMNavigationDashboardStatePlanning)
    return;
  switch (state)
  {
    case MWMNavigationDashboardStateHidden:
      [self hideState];
      break;
    case MWMNavigationDashboardStatePlanning:
      [self showStatePlanning];
      break;
    case MWMNavigationDashboardStateError:
      NSAssert(_state == MWMNavigationDashboardStatePlanning, @"Invalid state change (error)");
      [self handleError];
      break;
    case MWMNavigationDashboardStateReady:
      NSAssert(_state == MWMNavigationDashboardStatePlanning, @"Invalid state change (ready)");
      [self showStateReady];
      break;
    case MWMNavigationDashboardStateNavigation:
      [self showStateNavigation];
      break;
  }
  _state = state;
  [self.delegate updateStatusBarStyle];
}

- (void)setTopBound:(CGFloat)topBound
{
  _topBound = self.routePreviewLandscape.topBound = self.routePreviewPortrait.topBound =
  self.navigationDashboardLandscape.topBound = self.navigationDashboardPortrait.topBound = topBound;
}

- (CGFloat)height
{
  switch (self.state)
  {
    case MWMNavigationDashboardStateHidden:
      return 0.0;
    case MWMNavigationDashboardStatePlanning:
    case MWMNavigationDashboardStateReady:
    case MWMNavigationDashboardStateError:
      return self.routePreview.visibleHeight;
    case MWMNavigationDashboardStateNavigation:
      return self.navigationDashboard.visibleHeight;
  }
}

#pragma mark - LocationObserver

- (void)onLocationUpdate:(const location::GpsInfo &)info
{
// We don't need information about location update in this class,
// but in LocationObserver protocol this method is required
// since we don't want runtime overhead for introspection.
}

- (void)onCompassUpdate:(location::CompassInfo const &)info
{
  auto & f = GetFramework();
  if (f.GetRouter() != routing::RouterType::Pedestrian)
    return;

  CLLocation * location = [MapsAppDelegate theApp].m_locationManager.lastLocation;
  if (!location)
    return;

  location::FollowingInfo res;
  f.GetRouteFollowingInfo(res);
  if (!res.IsValid())
    return;

  CGFloat const angle = ang::AngleTo(ToMercator(location.coordinate),
                                     ToMercator(res.m_pedestrianDirectionPos)) + info.m_bearing;
  CGAffineTransform const transform (CGAffineTransformMakeRotation(M_PI_2 - angle));
  self.navigationDashboardPortrait.direction.transform = transform;
  self.navigationDashboardLandscape.direction.transform = transform;
}

@end
