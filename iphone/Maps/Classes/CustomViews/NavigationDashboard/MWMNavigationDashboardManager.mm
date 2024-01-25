#import "MWMNavigationDashboardManager.h"
#import "MWMMapViewControlsManager.h"
#import "MWMNavigationInfoView.h"
#import "MWMRoutePreview.h"
#import "MWMSearch.h"
#import "MapViewController.h"

#import "SwiftBridge.h"

namespace {
NSString *const kRoutePreviewIPhoneXibName = @"MWMiPhoneRoutePreview";
NSString *const kNavigationInfoViewXibName = @"MWMNavigationInfoView";
NSString *const kNavigationControlViewXibName = @"NavigationControlView";
}  // namespace

@interface MWMMapViewControlsManager ()

@property(nonatomic) MWMNavigationDashboardManager *navigationManager;

@end

@interface MWMNavigationDashboardManager () <MWMSearchManagerObserver, MWMRoutePreviewDelegate>

@property(copy, nonatomic) NSDictionary *etaAttributes;
@property(copy, nonatomic) NSDictionary *etaSecondaryAttributes;
@property(copy, nonatomic) NSString *errorMessage;
@property(nonatomic) IBOutlet MWMBaseRoutePreviewStatus *baseRoutePreviewStatus;
@property(nonatomic) IBOutlet MWMNavigationControlView *navigationControlView;
@property(nonatomic) IBOutlet MWMNavigationInfoView *navigationInfoView;
@property(nonatomic) IBOutlet MWMRoutePreview *routePreview;
@property(nonatomic) IBOutlet MWMTransportRoutePreviewStatus *transportRoutePreviewStatus;
@property(nonatomic) IBOutletCollection(MWMRouteStartButton) NSArray *goButtons;
@property(nonatomic) MWMNavigationDashboardEntity *entity;
@property(nonatomic) MWMRouteManagerTransitioningManager *routeManagerTransitioningManager;
@property(weak, nonatomic) IBOutlet UIButton *showRouteManagerButton;
@property(weak, nonatomic) IBOutlet UIView *goButtonsContainer;
@property(weak, nonatomic) UIView *ownerView;

@end

@implementation MWMNavigationDashboardManager

+ (MWMNavigationDashboardManager *)sharedManager {
  return [MWMMapViewControlsManager manager].navigationManager;
}

- (instancetype)initWithParentView:(UIView *)view {
  self = [super init];
  if (self) {
    _ownerView = view;
  }
  return self;
}

- (void)loadPreviewWithStatusBoxes {
  [NSBundle.mainBundle loadNibNamed:kRoutePreviewIPhoneXibName owner:self options:nil];
  auto ownerView = self.ownerView;
  _baseRoutePreviewStatus.ownerView = ownerView;
  _transportRoutePreviewStatus.ownerView = ownerView;
}

#pragma mark - MWMRoutePreview

- (void)setRouteBuilderProgress:(CGFloat)progress {
  [self.routePreview router:[MWMRouter type] setProgress:progress / 100.];
}

#pragma mark - MWMNavigationGo

- (IBAction)routingStartTouchUpInside {
  [MWMRouter startRouting];
}
- (void)updateGoButtonTitle {
  NSString *title = L(@"p2p_start");

  for (MWMRouteStartButton *button in self.goButtons)
    [button setTitle:title forState:UIControlStateNormal];
}

- (void)onNavigationInfoUpdated {
  auto entity = self.entity;
  if (!entity.isValid)
    return;
  [_navigationInfoView onNavigationInfoUpdated:entity];
  bool const isPublicTransport = [MWMRouter type] == MWMRouterTypePublicTransport;
  bool const isRuler = [MWMRouter type] == MWMRouterTypeRuler;
  if (isPublicTransport || isRuler)
    [_transportRoutePreviewStatus onNavigationInfoUpdated:entity prependDistance:isRuler];
  else
    [_baseRoutePreviewStatus onNavigationInfoUpdated:entity];
  [_navigationControlView onNavigationInfoUpdated:entity];
}

#pragma mark - On route updates

- (void)onRoutePrepare {
  self.state = MWMNavigationDashboardStatePrepare;
  self.routePreview.drivingOptionsState = MWMDrivingOptionsStateNone;
}

- (void)onRoutePlanning {
  self.state = MWMNavigationDashboardStatePlanning;
  self.routePreview.drivingOptionsState = MWMDrivingOptionsStateNone;
}

- (void)onRouteError:(NSString *)error {
  self.errorMessage = error;
  self.state = MWMNavigationDashboardStateError;
  self.routePreview.drivingOptionsState =
    [MWMRouter hasActiveDrivingOptions] ? MWMDrivingOptionsStateChange : MWMDrivingOptionsStateNone;
}

- (void)onRouteReady:(BOOL)hasWarnings {
  if (self.state != MWMNavigationDashboardStateNavigation)
    self.state = MWMNavigationDashboardStateReady;
  if ([MWMRouter hasActiveDrivingOptions]) {
    self.routePreview.drivingOptionsState = MWMDrivingOptionsStateChange;
  } else {
    self.routePreview.drivingOptionsState = hasWarnings ? MWMDrivingOptionsStateDefine : MWMDrivingOptionsStateNone;
  }
}

- (void)onRoutePointsUpdated {
  if (self.state == MWMNavigationDashboardStateHidden)
    self.state = MWMNavigationDashboardStatePrepare;
  [self.navigationInfoView updateToastView];
}

#pragma mark - State changes

- (void)stateHidden {
  self.routePreview = nil;
  self.navigationInfoView.state = MWMNavigationInfoViewStateHidden;
  self.navigationInfoView = nil;
  _navigationControlView.isVisible = NO;
  _navigationControlView = nil;
  [_baseRoutePreviewStatus hide];
  _baseRoutePreviewStatus = nil;
  [_transportRoutePreviewStatus hide];
  _transportRoutePreviewStatus = nil;
}

- (void)statePrepare {
  self.navigationInfoView.state = MWMNavigationInfoViewStatePrepare;
  auto routePreview = self.routePreview;
  [routePreview addToView:self.ownerView];
  [routePreview statePrepare];
  [routePreview selectRouter:[MWMRouter type]];
  [self updateGoButtonTitle];
  [_baseRoutePreviewStatus hide];
  [_transportRoutePreviewStatus hide];
  for (MWMRouteStartButton *button in self.goButtons)
    [button statePrepare];
}

- (void)statePlanning {
  [self statePrepare];
  [self.routePreview router:[MWMRouter type] setState:MWMCircularProgressStateSpinner];
  [self setRouteBuilderProgress:0.];
}

- (void)stateError {
  if (_state == MWMNavigationDashboardStateReady)
    return;

  NSAssert(_state == MWMNavigationDashboardStatePlanning, @"Invalid state change (error)");
  auto routePreview = self.routePreview;
  [routePreview router:[MWMRouter type] setState:MWMCircularProgressStateFailed];
  [self updateGoButtonTitle];
  [self.baseRoutePreviewStatus showErrorWithMessage:self.errorMessage];
  for (MWMRouteStartButton *button in self.goButtons)
    [button stateError];
}

- (void)stateReady {
  // TODO: Here assert sometimes fires with _state = MWMNavigationDashboardStateReady, if app was stopped while navigating and then restarted.
  // Also in ruler mode when new point is added by single tap on the map state MWMNavigationDashboardStatePlanning is skipped and we get _state = MWMNavigationDashboardStateReady.
  NSAssert(_state == MWMNavigationDashboardStatePlanning || _state == MWMNavigationDashboardStateReady, @"Invalid state change (ready)");
  [self setRouteBuilderProgress:100.];
  [self updateGoButtonTitle];
  bool const isTransport = ([MWMRouter type] == MWMRouterTypePublicTransport);
  bool const isRuler = ([MWMRouter type] == MWMRouterTypeRuler);
  if (isTransport || isRuler)
    [self.transportRoutePreviewStatus showReady];
  else
    [self.baseRoutePreviewStatus showReady];
  self.goButtonsContainer.hidden = isTransport || isRuler;
  for (MWMRouteStartButton *button in self.goButtons)
  {
    if (isRuler)
      [button stateHidden];
    else
      [button stateReady];
  }
}

- (void)onRouteStart {
  self.state = MWMNavigationDashboardStateNavigation;
}
- (void)onRouteStop {
  self.state = MWMNavigationDashboardStateHidden;
}
- (void)stateNavigation {
  self.routePreview = nil;
  self.navigationInfoView.state = MWMNavigationInfoViewStateNavigation;
  self.navigationControlView.isVisible = YES;
  [_baseRoutePreviewStatus hide];
  _baseRoutePreviewStatus = nil;
  [_transportRoutePreviewStatus hide];
  _transportRoutePreviewStatus = nil;
  [self onNavigationInfoUpdated];
}

#pragma mark - MWMRoutePreviewStatus

- (IBAction)showRouteManager {
  auto routeManagerViewModel = [[MWMRouteManagerViewModel alloc] init];
  auto routeManager = [[MWMRouteManagerViewController alloc] initWithViewModel:routeManagerViewModel];
  routeManager.modalPresentationStyle = UIModalPresentationCustom;

  self.routeManagerTransitioningManager = [[MWMRouteManagerTransitioningManager alloc] init];
  routeManager.transitioningDelegate = self.routeManagerTransitioningManager;

  [[MapViewController sharedController] presentViewController:routeManager animated:YES completion:nil];
}

#pragma mark - MWMNavigationControlView

- (IBAction)ttsButtonAction {
  BOOL const isEnabled = [MWMTextToSpeech tts].active;
  [MWMTextToSpeech tts].active = !isEnabled;
}

- (IBAction)settingsButtonAction {
  [[MapViewController sharedController] performSegueWithIdentifier:@"Map2Settings" sender:nil];
}

- (IBAction)stopRoutingButtonAction {
  [MWMSearch clear];
  [MWMRouter stopRouting];
}

#pragma mark - MWMSearchManagerObserver

- (void)onSearchManagerStateChanged {
  auto state = [MWMSearchManager manager].state;
  if (state == MWMSearchManagerStateMapSearch)
    [self setMapSearch];
}

#pragma mark - Available area

+ (void)updateNavigationInfoAvailableArea:(CGRect)frame {
  [[self sharedManager] updateNavigationInfoAvailableArea:frame];
}

- (void)updateNavigationInfoAvailableArea:(CGRect)frame {
  _navigationInfoView.availableArea = frame;
}
#pragma mark - Properties

- (NSDictionary *)etaAttributes {
  if (!_etaAttributes) {
    _etaAttributes =
      @{NSForegroundColorAttributeName: [UIColor blackPrimaryText], NSFontAttributeName: [UIFont medium17]};
  }
  return _etaAttributes;
}

- (NSDictionary *)etaSecondaryAttributes {
  if (!_etaSecondaryAttributes) {
    _etaSecondaryAttributes =
      @{NSForegroundColorAttributeName: [UIColor blackSecondaryText], NSFontAttributeName: [UIFont medium17]};
  }
  return _etaSecondaryAttributes;
}

- (void)setState:(MWMNavigationDashboardState)state {
  if (state == MWMNavigationDashboardStateHidden)
    [MWMSearchManager removeObserver:self];
  else
    [MWMSearchManager addObserver:self];
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

@synthesize routePreview = _routePreview;
- (MWMRoutePreview *)routePreview {
  if (!_routePreview)
    [self loadPreviewWithStatusBoxes];
  return _routePreview;
}

- (void)setRoutePreview:(MWMRoutePreview *)routePreview {
  if (routePreview == _routePreview)
    return;
  [_routePreview remove];
  _routePreview = routePreview;
  _routePreview.delegate = self;
}

- (MWMBaseRoutePreviewStatus *)baseRoutePreviewStatus {
  if (!_baseRoutePreviewStatus)
    [self loadPreviewWithStatusBoxes];
  return _baseRoutePreviewStatus;
}

- (MWMTransportRoutePreviewStatus *)transportRoutePreviewStatus {
  if (!_transportRoutePreviewStatus)
    [self loadPreviewWithStatusBoxes];
  return _transportRoutePreviewStatus;
}

- (MWMNavigationInfoView *)navigationInfoView {
  if (!_navigationInfoView) {
    [NSBundle.mainBundle loadNibNamed:kNavigationInfoViewXibName owner:self options:nil];
    _navigationInfoView.state = MWMNavigationInfoViewStateHidden;
    _navigationInfoView.ownerView = self.ownerView;
  }
  return _navigationInfoView;
}

- (MWMNavigationControlView *)navigationControlView {
  if (!_navigationControlView) {
    [NSBundle.mainBundle loadNibNamed:kNavigationControlViewXibName owner:self options:nil];
    _navigationControlView.ownerView = self.ownerView;
  }
  return _navigationControlView;
}

- (MWMNavigationDashboardEntity *)entity {
  if (!_entity)
    _entity = [[MWMNavigationDashboardEntity alloc] init];
  return _entity;
}

- (void)setMapSearch {
  [_navigationInfoView setMapSearch];
}

#pragma mark - MWMRoutePreviewDelegate

- (void)routePreviewDidPressDrivingOptions:(MWMRoutePreview *)routePreview {
  [[MapViewController sharedController] openDrivingOptions];
}

@end
