#import "Common.h"
#import "Macros.h"
#import "MapsAppDelegate.h"
#import "MWMCircularProgress.h"
#import "MWMLanesPanel.h"
#import "MWMNavigationDashboard.h"
#import "MWMNavigationDashboardEntity.h"
#import "MWMNavigationDashboardManager.h"
#import "MWMNextTurnPanel.h"
#import "MWMRouteHelperPanelsDrawer.h"
#import "MWMRoutePreview.h"
#import "MWMTextToSpeech.h"
#import "MWMRouteTypeButton.h"

@interface MWMNavigationDashboardManager () <MWMCircularProgressProtocol>

@property (nonatomic) IBOutlet MWMRoutePreview * iPhoneRoutePreview;
@property (nonatomic) IBOutlet MWMRoutePreview * iPadRoutePreview;
@property (weak, nonatomic, readwrite) MWMRoutePreview * routePreview;

@property (nonatomic) IBOutlet MWMNavigationDashboard * navigationDashboardLandscape;
@property (nonatomic) IBOutlet MWMNavigationDashboard * navigationDashboardPortrait;
@property (weak, nonatomic) MWMNavigationDashboard * navigationDashboard;
@property (weak, nonatomic) MWMCircularProgress * activeRouteTypeButton;

@property (weak, nonatomic) UIView * ownerView;

@property (nonatomic) MWMNavigationDashboardEntity * entity;
@property (nonatomic) MWMTextToSpeech * tts;
//@property (nonatomic) MWMLanesPanel * lanesPanel;
@property (nonatomic) MWMNextTurnPanel * nextTurnPanel;
@property (nonatomic) MWMRouteHelperPanelsDrawer * drawer;
@property (nonatomic) NSMutableArray * helperPanels;

@end

@implementation MWMNavigationDashboardManager

- (instancetype)initWithParentView:(UIView *)view delegate:(id<MWMNavigationDashboardManagerProtocol, MWMRoutePreviewDataSource>)delegate
{
  self = [super init];
  if (self)
  {
    self.ownerView = view;
    _delegate = delegate;
    BOOL const isPortrait = self.ownerView.width < self.ownerView.height;
    if (IPAD)
    {
      [NSBundle.mainBundle loadNibNamed:@"MWMiPadRoutePreview" owner:self options:nil];
      self.routePreview = self.iPadRoutePreview;
    }
    else
    {
      [NSBundle.mainBundle loadNibNamed:[MWMRoutePreview className] owner:self options:nil];
      self.routePreview = self.iPhoneRoutePreview;
    }

    self.routePreview.dashboardManager = self;
    self.routePreview.pedestrianProgressView.delegate = self.routePreview.vehicleProgressView.delegate = self;
    self.routePreview.delegate = delegate;
    self.routePreview.dataSource = delegate;
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
    self.helperPanels = [NSMutableArray array];
  }
  return self;
}

#pragma mark - Layout

- (void)willRotateToInterfaceOrientation:(UIInterfaceOrientation)orientation
{
  [self updateInterface:UIInterfaceOrientationIsPortrait(orientation)];
}

- (void)viewWillTransitionToSize:(CGSize)size
       withTransitionCoordinator:(id<UIViewControllerTransitionCoordinator>)coordinator
{
  [self updateInterface:size.height > size.width];
}

- (void)updateInterface:(BOOL)isPortrait
{
  if (IPAD)
    return;

  MWMNavigationDashboard * navigationDashboard = isPortrait ? self.navigationDashboardPortrait :
  self.navigationDashboardLandscape;
  if (self.navigationDashboard.isVisible && ![navigationDashboard isEqual:self.navigationDashboard])
  {
    [self.navigationDashboard remove];
    [navigationDashboard addToView:self.ownerView];
  }
  self.navigationDashboard = navigationDashboard;
  [self.drawer invalidateTopBounds:self.helperPanels topView:self.navigationDashboard];
}

- (void)hideHelperPanels
{
  if (IPAD)
    return;
  for (MWMRouteHelperPanel * p in self.helperPanels)
    [UIView animateWithDuration:kDefaultAnimationDuration animations:^{ p.alpha = 0.; }];
}

- (void)showHelperPanels
{
  if (IPAD)
    return;
  for (MWMRouteHelperPanel * p in self.helperPanels)
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
  if (self.state == MWMNavigationDashboardStateNavigation)
    [self.tts playTurnNotifications];
}

- (void)handleError
{
  [self.routePreview stateError];
}

- (void)updateDashboard
{
  BOOL const isPrepareState = self.state == MWMNavigationDashboardStateHidden &&
                              self.state == MWMNavigationDashboardStateError &&
                              self.state == MWMNavigationDashboardStatePlanning;
  if (isPrepareState)
    return;

  [self.routePreview configureWithEntity:self.entity];
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
  [self.drawer invalidateTopBounds:self.helperPanels topView:self.navigationDashboard];
}

- (void)addPanel:(MWMRouteHelperPanel *)panel
{
  switch (self.helperPanels.count)
  {
    case 0:
      [self.helperPanels addObject:panel];
      return;
    case 1:
      if (![self.helperPanels.firstObject isKindOfClass:panel.class])
      {
        [self.helperPanels addObject:panel];
        return;
      }
      return;
    case 2:
      for (MWMRouteHelperPanel * p in self.helperPanels)
      {
        if ([p isEqual:panel])
          continue;

        if ([p isKindOfClass:panel.class])
        {
          NSUInteger const index = [self.helperPanels indexOfObject:p];
          self.helperPanels[index] = panel;
        }
      }
      return;
    default:
      NSAssert(false, @"Incorrect array size!");
      break;
  }
}

- (void)removePanel:(MWMRouteHelperPanel *)panel
{
  if ([self.helperPanels containsObject:panel])
    [self.helperPanels removeObject:panel];
  panel.hidden = YES;
}

#pragma mark - MWMCircularProgressDelegate

- (void)progressButtonPressed:(nonnull MWMCircularProgress *)progress
{
  if (progress.state == MWMCircularProgressStateSelected || progress.state == MWMCircularProgressStateCompleted)
    return;
  progress.state = MWMCircularProgressStateSelected;
  self.activeRouteTypeButton = progress;
  auto & f = GetFramework();
  routing::RouterType type;
  if ([progress isEqual:self.routePreview.pedestrianProgressView])
  {
    self.routePreview.vehicleProgressView.state = MWMCircularProgressStateNormal;
    type = routing::RouterType::Pedestrian;
  }
  else
  {
    self.routePreview.pedestrianProgressView.state = MWMCircularProgressStateNormal;
    type = routing::RouterType::Vehicle;
  }
  f.CloseRouting();
  f.SetRouter(type);
  f.SetLastUsedRouter(type);
  if (!self.delegate.isPossibleToBuildRoute)
    return;
  [progress startSpinner];
  [self.delegate buildRoute];
}

#pragma mark - MWMRoutePreview

- (void)setRouteBuildingProgress:(CGFloat)progress
{
  [self.activeRouteTypeButton setProgress:progress / 100.];
}

#pragma mark - MWMNavigationDashboard

- (IBAction)navigationCancelPressed:(UIButton *)sender
{
  if (IPAD && self.state != MWMNavigationDashboardStateNavigation)
    [self.delegate routePreviewDidChangeFrame:{}];
  self.state = MWMNavigationDashboardStateHidden;
  [self removePanel:self.nextTurnPanel];
//  [self removePanel:self.lanesPanel];
  self.helperPanels = [NSMutableArray array];
  [self.delegate didCancelRouting];
}

#pragma mark - MWMNavigationGo

- (IBAction)navigationGoPressed:(UIButton *)sender
{
  if ([self.delegate didStartFollowing])
    self.state = MWMNavigationDashboardStateNavigation;
}

#pragma mark - State changes

- (void)hideState
{
  [self.routePreview remove];
  [self.navigationDashboard remove];
  [self removePanel:self.nextTurnPanel];
//  [self removePanel:self.lanesPanel];
}

- (void)showStatePrepare
{
  [self.routePreview addToView:self.ownerView];
  [self.routePreview statePrepare];
  [self setupActualRoute];
}

- (void)showStatePlanning
{
  [self.navigationDashboard remove];
  [self.routePreview addToView:self.ownerView];
  [self.routePreview statePlanning];
  [self removePanel:self.nextTurnPanel];
//  [self removePanel:self.lanesPanel];
  [self setupActualRoute];
}

- (void)showStateReady
{
  [self.routePreview stateReady];
}

- (void)showStateNavigation
{
  [self.routePreview remove];
  [self.navigationDashboard addToView:self.ownerView];
}

- (void)setupActualRoute
{
  switch (GetFramework().GetRouter())
  {
  case routing::RouterType::Pedestrian:
    self.routePreview.pedestrianProgressView.state = MWMCircularProgressStateSelected;
    self.routePreview.vehicleProgressView.state = MWMCircularProgressStateNormal;
    self.activeRouteTypeButton = self.routePreview.pedestrianProgressView;
    break;
  case routing::RouterType::Vehicle:
    self.routePreview.vehicleProgressView.state = MWMCircularProgressStateSelected;
    self.routePreview.pedestrianProgressView.state = MWMCircularProgressStateNormal;
    self.activeRouteTypeButton = self.routePreview.vehicleProgressView;
    break;
  }
}

#pragma mark - Properties

- (MWMRouteHelperPanelsDrawer *)drawer
{
  if (!_drawer)
    _drawer = [[MWMRouteHelperPanelsDrawer alloc] initWithTopView:self.navigationDashboard];
  return _drawer;
}

//- (MWMLanesPanel *)lanesPanel
//{
//  if (!_lanesPanel)
//    _lanesPanel = [[MWMLanesPanel alloc] initWithParentView:IPAD ? self.navigationDashboard : self.ownerView];
//  return _lanesPanel;
//}

- (MWMNextTurnPanel *)nextTurnPanel
{
  if (!_nextTurnPanel)
    _nextTurnPanel = [MWMNextTurnPanel turnPanelWithOwnerView:IPAD ? self.navigationDashboard : self.ownerView];
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
  case MWMNavigationDashboardStatePrepare:
    [self showStatePrepare];
    break;
  case MWMNavigationDashboardStatePlanning:
    [self showStatePlanning];
    break;
  case MWMNavigationDashboardStateError:
    NSAssert(_state == MWMNavigationDashboardStatePlanning || _state == MWMNavigationDashboardStateReady, @"Invalid state change (error)");
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
  _topBound = self.routePreview.topBound =
  self.navigationDashboardLandscape.topBound = self.navigationDashboardPortrait.topBound = topBound;
  [self.drawer invalidateTopBounds:self.helperPanels topView:self.navigationDashboard];
}

- (void)setLeftBound:(CGFloat)leftBound
{
  _leftBound = self.routePreview.leftBound =
  self.navigationDashboardLandscape.leftBound = self.navigationDashboardPortrait.leftBound = leftBound;
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
  case MWMNavigationDashboardStatePrepare:
    if (IPAD)
      return self.topBound;
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
