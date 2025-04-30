#import "MWMNavigationDashboardManager.h"
#import "MWMRoutePreview.h"
#import "MWMSearch.h"
#import "MapViewController.h"
#import "MWMNavigationDashboardView.h"
#import "SwiftBridge.h"

@interface MWMMapViewControlsManager ()

@property(nonatomic) MWMNavigationDashboardManager * navigationManager;

@end

@interface MWMNavigationDashboardManager () <SearchOnMapManagerObserver, MWMRoutePreviewDelegate, RouteNavigationControlsDelegate>

@property(copy, nonatomic) NSDictionary * etaAttributes;
@property(copy, nonatomic) NSDictionary * etaSecondaryAttributes;
@property(copy, nonatomic) NSString * errorMessage;
@property(copy, nonatomic) MWMNavigationDashboardEntity * entity;

@property(weak, nonatomic) id<NavigationDashboardView> navigationDashboardView;
@property(weak, nonatomic) MapViewController * parentViewController;

@end

@implementation MWMNavigationDashboardManager

+ (MWMNavigationDashboardManager *)sharedManager {
  return [MWMMapViewControlsManager manager].navigationManager;
}

- (instancetype)initWithParentViewController:(MapViewController *)viewController {
  self = [super init];
  if (self) {
    _parentViewController = viewController;
  }
  return self;
}

- (id<NavigationDashboardView>)navigationDashboardView {
  auto navigationDashboardView = _navigationDashboardView;
  if (navigationDashboardView)
    return navigationDashboardView;
  RoutePreviewViewController * routePreviewViewController = [RoutePreviewBuilder buildWithDelegate:self];
  [routePreviewViewController addTo:self.parentViewController];
  _navigationDashboardView = routePreviewViewController.interactor;
  return navigationDashboardView;
}

- (SearchOnMapManager *)searchManager {
  return [[MapViewController sharedController] searchManager];
}

- (void)setRouteBuilderProgress:(CGFloat)progress {
  if (self.state == MWMNavigationDashboardStateClosed)
    return;
  [self.navigationDashboardView setRouteBuilderProgress:[MWMRouter type] progress:progress / 100.];
}

#pragma mark - On route updates

- (void)onSelectPlacePage:(BOOL)selected {
  if (self.state == MWMNavigationDashboardStateClosed ||
      self.state == MWMNavigationDashboardStateNavigation)
    return;
  self.state = selected ? MWMNavigationDashboardStateHidden : MWMNavigationDashboardStateReady;
}

- (void)onNavigationInfoUpdated {
  if (self.state == MWMNavigationDashboardStateClosed)
    return;
  auto entity = self.entity;
  if (!entity.isValid)
    return;
  [self.navigationDashboardView onNavigationInfoUpdated:entity];
}

- (void)onRoutePrepare {
  self.state = MWMNavigationDashboardStatePrepare;
  [self.navigationDashboardView setDrivingOptionState:MWMDrivingOptionsStateNone];
}

- (void)onRoutePlanning {
  self.state = MWMNavigationDashboardStatePlanning;
  [self.navigationDashboardView setDrivingOptionState:MWMDrivingOptionsStateNone];
}

- (void)onRouteError:(NSString *)error {
  self.errorMessage = error;
  self.state = MWMNavigationDashboardStateError;
  [self.navigationDashboardView setDrivingOptionState:[MWMRouter hasActiveDrivingOptions] ? MWMDrivingOptionsStateChange : MWMDrivingOptionsStateNone];
}

- (void)onRouteReady:(BOOL)hasWarnings {
  if (self.state != MWMNavigationDashboardStateNavigation)
    self.state = MWMNavigationDashboardStateReady;
  if ([MWMRouter hasActiveDrivingOptions]) {
    [self.navigationDashboardView setDrivingOptionState:MWMDrivingOptionsStateChange];
  } else {
    [self.navigationDashboardView setDrivingOptionState:hasWarnings ? MWMDrivingOptionsStateDefine : MWMDrivingOptionsStateNone];
  }
}

- (void)onRoutePointsUpdated {
  if (self.state == MWMNavigationDashboardStateClosed)
    return;
  if (self.state == MWMNavigationDashboardStateHidden)
    self.state = MWMNavigationDashboardStatePrepare;
  [self.navigationDashboardView onRoutePointsUpdated];
}

#pragma mark - Properties

- (void)setState:(MWMNavigationDashboardState)state {
  if (state == MWMNavigationDashboardStateClosed)
    [self.searchManager removeObserver:self];
  else
    [self.searchManager addObserver:self];
  _state = state;
  switch (state) {
    case MWMNavigationDashboardStateClosed:
      [self stateClosed];
      break;
    case MWMNavigationDashboardStateHidden:
      [self stateHidden];
      break;
    case MWMNavigationDashboardStatePrepare:
      [self statePrepare];
      break;
    case MWMNavigationDashboardStatePlanning:
      [self statePlanning];
      break;
    case MWMNavigationDashboardStateError:
      [self stateError];
      break;
    case MWMNavigationDashboardStateReady:
      [self stateReady];
      break;
    case MWMNavigationDashboardStateNavigation:
      [self stateNavigation];
      break;
  }
  [[MapViewController sharedController] updateStatusBarStyle];
  // Restore bottom buttons only if they were not already hidden by tapping anywhere on an empty map.
  if (!MWMMapViewControlsManager.manager.hidden)
    BottomTabBarViewController.controller.isHidden = state != MWMNavigationDashboardStateClosed;
}

- (MWMNavigationDashboardEntity *)entity {
  if (!_entity)
    _entity = [[MWMNavigationDashboardEntity alloc] init];
  return _entity;
}

#pragma mark - State changes

- (void)stateClosed {
  [self.navigationDashboardView stateClosed];
}

- (void)stateHidden {
  [self.navigationDashboardView setHidden:YES];
}

- (MWMBaseRoutePreviewStatus *)baseRoutePreviewStatus {
  if (!_baseRoutePreviewStatus)
    [self loadPreviewWithStatusBoxes];
  return _baseRoutePreviewStatus;
- (void)statePrepare {
  [self.navigationDashboardView statePrepare];
}

- (MWMTransportRoutePreviewStatus *)transportRoutePreviewStatus {
  if (!_transportRoutePreviewStatus)
    [self loadPreviewWithStatusBoxes];
  return _transportRoutePreviewStatus;
- (void)statePlanning {
  [self statePrepare];
  [self.navigationDashboardView statePlanning];
  [self setRouteBuilderProgress:0.];
}

- (MWMNavigationInfoView *)navigationInfoView {
  if (!_navigationInfoView) {
    [NSBundle.mainBundle loadNibNamed:kNavigationInfoViewXibName owner:self options:nil];
    _navigationInfoView.state = MWMNavigationInfoViewStateHidden;
    _navigationInfoView.ownerView = self.ownerView;
  }
  return _navigationInfoView;
- (void)stateError {
  if (_state == MWMNavigationDashboardStateReady)
    return;
  NSAssert(_state == MWMNavigationDashboardStatePlanning, @"Invalid state change (error)");
  [self.navigationDashboardView stateError:self.errorMessage];
}

- (MWMNavigationControlView *)navigationControlView {
  if (!_navigationControlView) {
    [NSBundle.mainBundle loadNibNamed:kNavigationControlViewXibName owner:self options:nil];
    _navigationControlView.ownerView = self.ownerView;
  }
  return _navigationControlView;
- (void)stateReady {
  // TODO: Here assert sometimes fires with _state = MWMNavigationDashboardStateReady, if app was stopped while navigating and then restarted.
  // Also in ruler mode when new point is added by single tap on the map state MWMNavigationDashboardStatePlanning is skipped and we get _state = MWMNavigationDashboardStateReady.
  NSAssert(_state == MWMNavigationDashboardStatePlanning || _state == MWMNavigationDashboardStateReady, @"Invalid state change (ready)");
  [self setRouteBuilderProgress:100.];
  [self.navigationDashboardView stateReady];
}

- (MWMNavigationDashboardEntity *)entity {
  if (!_entity)
    _entity = [[MWMNavigationDashboardEntity alloc] init];
  return _entity;
- (void)onRouteStart {
  self.state = MWMNavigationDashboardStateNavigation;
}
- (void)onRouteStop {
  self.state = MWMNavigationDashboardStateClosed;
}

- (void)stateNavigation {
  [self.navigationDashboardView stateNavigation];
  [self onNavigationInfoUpdated];
}

#pragma mark - MWMRoutePreviewDelegate

- (void)routingStartButtonDidTap {
  [MWMRouter startRouting];
}

- (void)routePreviewDidPressDrivingOptions {
  [[MapViewController sharedController] openDrivingOptions];
}

- (void)ttsButtonDidTap {
  BOOL const isEnabled = [MWMTextToSpeech tts].active;
  [MWMTextToSpeech tts].active = !isEnabled;
}

- (void)settingsButtonDidTap {
  [[MapViewController sharedController] openSettings];
}

- (void)stopRoutingButtonDidTap {
  [MWMSearch clear];
  [MWMRouter stopRouting];
  [self.searchManager close];
}

#pragma mark - SearchOnMapManagerObserver

- (void)searchManagerWithDidChangeState:(SearchOnMapState)state {
  [self.navigationDashboardView searchManagerWithDidChangeState:state];
}

#pragma mark - Available area

+ (void)updateNavigationInfoAvailableArea:(CGRect)frame {
  if ([self sharedManager].state == MWMNavigationDashboardStateClosed)
    return;
  [[self sharedManager].navigationDashboardView updateNavigationInfoAvailableArea:frame];
}

@end
