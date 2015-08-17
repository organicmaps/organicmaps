//
//  MWMNavigationDashboardManager.m
//  Maps
//
//  Created by v.mikhaylenko on 20.07.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import "Common.h"
#import "Macros.h"
#import "MapsAppDelegate.h"
#import "MWMLanesPanel.h"
#import "MWMNavigationDashboard.h"
#import "MWMNavigationDashboardEntity.h"
#import "MWMNavigationDashboardManager.h"
#import "MWMNextTurnPanel.h"
#import "MWMRouteHelperPanelsDrawer.h"
#import "MWMRoutePreview.h"
#import "MWMTextToSpeech.h"
#import "UIKitCategories.h"

@interface MWMNavigationDashboardManager ()
{
  vector<MWMRouteHelperPanel *> helperPanels;
}

@property (nonatomic) IBOutlet MWMRoutePreview * routePreviewLandscape;
@property (nonatomic) IBOutlet MWMRoutePreview * routePreviewPortrait;
@property (weak, nonatomic) MWMRoutePreview * routePreview;

@property (nonatomic) IBOutlet MWMNavigationDashboard * navigationDashboardLandscape;
@property (nonatomic) IBOutlet MWMNavigationDashboard * navigationDashboardPortrait;
@property (weak, nonatomic) MWMNavigationDashboard * navigationDashboard;

@property (weak, nonatomic) UIView * ownerView;
@property (weak, nonatomic) id<MWMNavigationDashboardManagerProtocol> delegate;

@property (nonatomic) MWMNavigationDashboardEntity * entity;
@property (nonatomic) MWMTextToSpeech * tts;
//@property (nonatomic) MWMLanesPanel * lanesPanel;
@property (nonatomic) MWMNextTurnPanel * nextTurnPanel;
@property (nonatomic) MWMRouteHelperPanelsDrawer * drawer;

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
    if (IPAD)
    {
      [NSBundle.mainBundle loadNibNamed:@"MWMNiPadNavigationDashboard" owner:self options:nil];
      self.navigationDashboard = self.navigationDashboardPortrait;
      self.navigationDashboard.delegate = delegate;
    }
    else
    {
      [NSBundle.mainBundle loadNibNamed:@"MWMPortraitNavigationDashboard" owner:self options:nil];
      [NSBundle.mainBundle loadNibNamed:@"MWMLandscapeNavigationDashboard" owner:self options:nil];
      self.navigationDashboard = isPortrait ? self.navigationDashboardPortrait : self.navigationDashboardLandscape;
      self.navigationDashboardPortrait.delegate = self.navigationDashboardLandscape.delegate = delegate;
    }
    self.tts = [[MWMTextToSpeech alloc] init];
  }
  return self;
}

#pragma mark - Layout

- (void)willRotateToInterfaceOrientation:(UIInterfaceOrientation)orientation
{
  if (IPAD)
    return;
  BOOL const isPortrait = UIInterfaceOrientationIsPortrait(orientation);
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
  [self.drawer invalidateTopBounds:helperPanels forOrientation:orientation];
}

- (void)hideHelperPanels
{
  for (auto p : helperPanels)
    [UIView animateWithDuration:kDefaultAnimationDuration animations:^{ p.alpha = 0.; }];
}

- (void)showHelperPanels
{
  for (auto p : helperPanels)
    [UIView animateWithDuration:kDefaultAnimationDuration animations:^{ p.alpha = 1.; }];
}

- (MWMNavigationDashboardEntity *)entity
{
  if (!_entity)
    _entity = [[MWMNavigationDashboardEntity alloc] init];
  return _entity;
}

- (void)setupDashboard:(location::FollowingInfo const &)info
{
  [self.entity updateWithFollowingInfo:info];
  [self updateDashboard];
}

- (void)playTurnNotifications
{
  [self.tts playTurnNotifications];
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
  if (self.state != MWMNavigationDashboardStateNavigation)
    return;
//  if (self.entity.lanes.size())
//  {
//    [self.lanesPanel configureWithLanes:self.entity.lanes];
//    [self addPanel:self.lanesPanel];
//  }
//  else
//  {
//    [self removePanel:self.lanesPanel];
//  }
  if (self.entity.nextTurnImage)
  {
    [self.nextTurnPanel configureWithImage:self.entity.nextTurnImage];
    [self addPanel:self.nextTurnPanel];
  }
  else
  {
    [self removePanel:self.nextTurnPanel];
  }
  [self.drawer drawPanels:helperPanels];
}

- (void)addPanel:(MWMRouteHelperPanel *)panel
{
  if (helperPanels.empty())
  {
    helperPanels.push_back(panel);
    return;
  }
  if (helperPanels.size() == 1)
  {
    if (![helperPanels.front() isKindOfClass:panel.class])
    {
      helperPanels.push_back(panel);
      return;
    }
  }
  for (auto p : helperPanels)
  {
    if ([p isEqual:panel])
      continue;

    if ([p isKindOfClass:panel.class])
      replace(helperPanels.begin(), helperPanels.end(), p, panel);
  }
}

- (void)removePanel:(MWMRouteHelperPanel *)panel
{
  if (find(helperPanels.begin(), helperPanels.end(), panel) != helperPanels.end())
    helperPanels.erase(remove(helperPanels.begin(), helperPanels.end(), panel));
  panel.hidden = YES;
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
  [self removePanel:self.nextTurnPanel];
//  [self removePanel:self.lanesPanel];
  helperPanels.clear();
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
  [self removePanel:self.nextTurnPanel];
//  [self removePanel:self.lanesPanel];
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

- (MWMRouteHelperPanelsDrawer *)drawer
{
  if (!_drawer)
  {
    if (IPAD)
      _drawer = [[MWMRouteHelperPanelsDrawer alloc] initWithView:self.navigationDashboard];
    else
      _drawer = [[MWMRouteHelperPanelsDrawer alloc] initWithView:self.ownerView];
  }
  return _drawer;
}

//- (MWMLanesPanel *)lanesPanel
//{
//  if (!_lanesPanel)
//  {
//    if (IPAD)
//      _lanesPanel = [[MWMLanesPanel alloc] initWithParentView:self.navigationDashboard];
//    else
//      _lanesPanel = [[MWMLanesPanel alloc] initWithParentView:self.ownerView];
//  }
//  return _lanesPanel;
//}

- (MWMNextTurnPanel *)nextTurnPanel
{
  if (!_nextTurnPanel)
  {
    if (IPAD)
      _nextTurnPanel = [MWMNextTurnPanel turnPanelWithOwnerView:self.navigationDashboard];
    else
      _nextTurnPanel = [MWMNextTurnPanel turnPanelWithOwnerView:self.ownerView];
  }
  return _nextTurnPanel;
}

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
