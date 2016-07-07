#import "Common.h"
#import "Macros.h"
#import "MapsAppDelegate.h"
#import "MapViewController.h"
#import "MWMLocationHelpers.h"
#import "MWMMapViewControlsManager.h"
#import "MWMNavigationDashboardManager.h"
#import "MWMRoutePreview.h"
#import "MWMRouter.h"
#import "MWMTextToSpeech.h"
#import "Statistics.h"

namespace
{
NSString * const kRoutePreviewXibName = @"MWMRoutePreview";
NSString * const kRoutePreviewIPADXibName = @"MWMiPadRoutePreview";
}  // namespace

@interface MWMMapViewControlsManager ()

@property (nonatomic) MWMNavigationDashboardManager * navigationManager;

@end

@interface MWMNavigationDashboardManager ()

@property (nonatomic, readwrite) IBOutlet MWMRoutePreview * routePreview;

@property (weak, nonatomic) UIView * ownerView;
@property (weak, nonatomic) id<MWMNavigationDashboardInfoProtocol> infoDisplay;

@property (nonatomic) MWMNavigationDashboardEntity * entity;

@end

@implementation MWMNavigationDashboardManager

+ (MWMNavigationDashboardManager *)manager
{
  return [MWMMapViewControlsManager manager].navigationManager;
}

- (instancetype)initWithParentView:(UIView *)view infoDisplay:(id<MWMNavigationDashboardInfoProtocol>)infoDisplay delegate:(id<MWMNavigationDashboardManagerProtocol>)delegate
{
  self = [super init];
  if (self)
  {
    _ownerView = view;
    _infoDisplay = infoDisplay;
    _delegate = delegate;
  }
  return self;
}

- (void)updateFollowingInfo:(location::FollowingInfo const &)info
{
  [self.entity updateFollowingInfo:info];
  [self updateDashboard];
}

- (void)handleError
{
  [self.routePreview stateError];
  [self.routePreview router:[MWMRouter router].type setState:MWMCircularProgressStateFailed];
}

- (void)updateDashboard
{
  if (!self.entity.isValid)
    return;
  [self.infoDisplay updateRoutingInfo:self.entity];
  [self.routePreview configureWithEntity:self.entity];
}

#pragma mark - MWMRoutePreview

- (void)setRouteBuilderProgress:(CGFloat)progress
{
  [self.routePreview router:[MWMRouter router].type setProgress:progress / 100.];
}

#pragma mark - MWMNavigationDashboard

- (IBAction)routingStopTouchUpInside
{
  if (IPAD && self.state != MWMNavigationDashboardStateNavigation)
    [self.delegate routePreviewDidChangeFrame:{}];
  [[MWMRouter router] stop];
}

#pragma mark - MWMNavigationGo

- (IBAction)routingStartTouchUpInside
{
  [[MWMRouter router] start];
}

#pragma mark - State changes

- (void)hideState
{
  [self.routePreview remove];
}

- (void)showStatePrepare
{
  [self.routePreview addToView:self.ownerView];
  [self.routePreview statePrepare];
  [self.routePreview selectRouter:[MWMRouter router].type];
}

- (void)showStatePlanning
{
  [self showStatePrepare];
  [self.delegate setMenuState:MWMBottomMenuStatePlanning];
  [self.routePreview router:[MWMRouter router].type setState:MWMCircularProgressStateSpinner];
  [self setRouteBuilderProgress:0.];
}

- (void)showStateReady
{
  [self.delegate setMenuState:MWMBottomMenuStateGo];
  [self.routePreview stateReady];
}

- (void)showStateNavigation
{
  [self.delegate setMenuState:MWMBottomMenuStateRouting];
  [self.routePreview remove];
}

- (void)mwm_refreshUI
{
  [self.routePreview mwm_refreshUI];
}

#pragma mark - Properties

- (void)setState:(MWMNavigationDashboardState)state
{
  if (_state == state)
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
  [[MapViewController controller] updateStatusBarStyle];
}

- (void)setTopBound:(CGFloat)topBound
{
  _topBound = self.routePreview.topBound = topBound;
}

- (void)setLeftBound:(CGFloat)leftBound
{
  _leftBound = self.routePreview.leftBound = leftBound;
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
    return 0.0; // TODO: Replace with real value
  }
}

#pragma mark - MWMLocationObserver

- (void)onHeadingUpdate:(location::CompassInfo const &)info
{
  auto & f = GetFramework();
  if (f.GetRouter() != routing::RouterType::Pedestrian)
    return;

  CLLocation * lastLocation = [MWMLocationManager lastLocation];
  if (!lastLocation)
    return;

  location::FollowingInfo res;
  f.GetRouteFollowingInfo(res);
  if (!res.IsValid())
    return;

  CGFloat const angle = ang::AngleTo(lastLocation.mercator,
                                     location_helpers::ToMercator(res.m_pedestrianDirectionPos)) +
                        info.m_bearing;
  CGAffineTransform const transform(CGAffineTransformMakeRotation(M_PI_2 - angle));
}

#pragma mark - Properties

- (MWMRoutePreview *)routePreview
{
  if (!_routePreview)
  {
    [NSBundle.mainBundle loadNibNamed:IPAD ? kRoutePreviewIPADXibName : kRoutePreviewXibName owner:self options:nil];
    _routePreview.dashboardManager = self;
    _routePreview.delegate = self.delegate;
  }
  return _routePreview;
}

- (MWMNavigationDashboardEntity *)entity
{
  if (!_entity)
    _entity = [[MWMNavigationDashboardEntity alloc] init];
  return _entity;
}

@end
