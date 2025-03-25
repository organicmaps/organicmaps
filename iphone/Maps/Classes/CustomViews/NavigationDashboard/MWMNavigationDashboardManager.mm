#import "MWMNavigationDashboardManager.h"
#import "MWMRoutePreview.h"
#import "MWMSearch.h"
#import "MapViewController.h"
#import "MWMNavigationDashboardView.h"
#import "SwiftBridge.h"

@interface MWMMapViewControlsManager ()

@property(nonatomic) MWMNavigationDashboardManager * navigationManager;

@end

@interface MWMNavigationDashboardManager () <SearchOnMapManagerObserver, MWMRoutePreviewDelegate>

@property(copy, nonatomic) NSDictionary * etaAttributes;
@property(copy, nonatomic) NSDictionary * etaSecondaryAttributes;
@property(copy, nonatomic) NSString * errorMessage;
@property(copy, nonatomic) MWMNavigationDashboardEntity * entity;

@property(nonatomic, nonnull, readonly) id<NavigationDashboardView> navigationDashboardView;
@property(weak, nonatomic) UIView * ownerView;

@end

@implementation MWMNavigationDashboardManager

+ (MWMNavigationDashboardManager *)sharedManager {
  return [MWMMapViewControlsManager manager].navigationManager;
}

- (instancetype)initWithParentView:(UIView *)view {
  self = [super init];
  if (self) {
    _ownerView = view;
    _navigationDashboardView = [[MWMNavigationDashboardView alloc] initWithOwnerView:view];
    _navigationDashboardView.delegate = self;
  }
  return self;
}

- (SearchOnMapManager *)searchManager {
  return [[MapViewController sharedController] searchManager];
}

- (void)setRouteBuilderProgress:(CGFloat)progress {
  [self.navigationDashboardView setRouteBuilderProgress:[MWMRouter type] progress:progress / 100.];
}

#pragma mark - On route updates

- (void)onNavigationInfoUpdated {
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
  if (self.state == MWMNavigationDashboardStateHidden)
    self.state = MWMNavigationDashboardStatePrepare;
  [self.navigationDashboardView onRoutePointsUpdated];
}

#pragma mark - Properties

- (void)setState:(MWMNavigationDashboardState)state {
  if (state == MWMNavigationDashboardStateHidden)
    [self.searchManager removeObserver:self];
  else
    [self.searchManager addObserver:self];
  switch (state) {
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
  _state = state;
  [[MapViewController sharedController] updateStatusBarStyle];
  // Restore bottom buttons only if they were not already hidden by tapping anywhere on an empty map.
  if (!MWMMapViewControlsManager.manager.hidden)
    BottomTabBarViewController.controller.isHidden = state != MWMNavigationDashboardStateHidden;
}

- (MWMNavigationDashboardEntity *)entity {
  if (!_entity)
    _entity = [[MWMNavigationDashboardEntity alloc] init];
  return _entity;
}

#pragma mark - State changes

- (void)stateHidden {
  [self.navigationDashboardView setHidden];
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
  self.state = MWMNavigationDashboardStateHidden;
}

- (void)stateNavigation {
  [self.navigationDashboardView stateNavigation];
  [self onNavigationInfoUpdated];
}

#pragma mark - MWMRoutePreviewDelegate

- (void)routingStartButtonDidTap {
  [MWMRouter startRouting];
}

- (void)routePreviewDidPressDrivingOptions:(MWMRoutePreview *)routePreview {
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
  [[self sharedManager].navigationDashboardView updateNavigationInfoAvailableArea:frame];
}

@end
