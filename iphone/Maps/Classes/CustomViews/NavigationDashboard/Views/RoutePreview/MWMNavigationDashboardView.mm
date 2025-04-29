#import "MWMNavigationDashboardView.h"
#import "MWMNavigationDashboardManager.h"
#import "MWMMapViewControlsManager.h"
#import "MWMNavigationInfoView.h"
#import "MWMRoutePreview.h"
#import "MWMSearch.h"
#import "MapViewController.h"
#import "SwiftBridge.h"

NSString *kRoutePreviewIPhoneXibName = @"MWMiPhoneRoutePreview";
NSString *kNavigationInfoViewXibName = @"MWMNavigationInfoView";
NSString *kNavigationControlViewXibName = @"NavigationControlView";

@interface MWMNavigationDashboardView()

@property(nonatomic) IBOutlet MWMBaseRoutePreviewStatus *baseRoutePreviewStatus;
@property(nonatomic) IBOutlet MWMNavigationControlView *navigationControlView;
@property(nonatomic) IBOutlet MWMNavigationInfoView *navigationInfoView;
@property(nonatomic) IBOutlet MWMRoutePreview *routePreview;
@property(nonatomic) IBOutlet MWMTransportRoutePreviewStatus *transportRoutePreviewStatus;
@property(nonatomic) IBOutletCollection(MWMRouteStartButton) NSArray *goButtons;
@property(nonatomic) MWMRouteManagerTransitioningManager *routeManagerTransitioningManager;
@property(weak, nonatomic) IBOutlet UIButton *showRouteManagerButton;
@property(weak, nonatomic) IBOutlet UIView *goButtonsContainer;
@property(weak, nonatomic) UIView *ownerView;

@end

@implementation MWMNavigationDashboardView

- (instancetype)initWithOwnerView:(UIView *)ownerView {
  self = [super init];
  if (self) {
    self.ownerView = ownerView;
    [self loadPreview];
  }
  return self;
}

- (void)loadPreview {
  [NSBundle.mainBundle loadNibNamed:kRoutePreviewIPhoneXibName owner:self options:nil];
  auto const ownerView = self.ownerView;
  self.baseRoutePreviewStatus.ownerView = ownerView;
  self.transportRoutePreviewStatus.ownerView = ownerView;
}

- (void)setRouteBuilderProgress:(MWMRouterType)router progress:(CGFloat)progress  {
  [self.routePreview router:router setProgress:progress];
}

- (void)onNavigationInfoUpdated:(MWMNavigationDashboardEntity *)entity {
  [_navigationInfoView onNavigationInfoUpdated:entity];
  bool const isPublicTransport = ([MWMRouter type] == MWMRouterTypePublicTransport);
  bool const isRuler = ([MWMRouter type] == MWMRouterTypeRuler);
  if (isPublicTransport || isRuler)
    [_transportRoutePreviewStatus onNavigationInfoUpdated:entity prependDistance:isRuler];
  else
    [_baseRoutePreviewStatus onNavigationInfoUpdated:entity];
  [_navigationControlView onNavigationInfoUpdated:entity];
}

- (void)setDrivingOptionState:(MWMDrivingOptionsState)state {
  self.routePreview.drivingOptionsState = state;
}

- (void)onRoutePointsUpdated {
  [self.navigationInfoView updateToastView];
}

- (void)updateGoButtonTitle {
  NSString *title = L(@"p2p_start");

  for (MWMRouteStartButton *button in self.goButtons)
    [button setTitle:title forState:UIControlStateNormal];
}

#pragma mark - State changes

- (void)setHidden {
  self.routePreview = nil;
  self.navigationInfoView.state = MWMNavigationInfoViewStateHidden;
  self.navigationInfoView = nil;
  _navigationControlView.isVisible = NO;
  _navigationControlView = nil;
  [self.baseRoutePreviewStatus hide];
  [_transportRoutePreviewStatus hide];
  _transportRoutePreviewStatus = nil;
}

- (void)statePrepare {
  self.navigationInfoView.state = MWMNavigationInfoViewStatePrepare;
  [self.routePreview addToView:self.ownerView];
  [self.routePreview statePrepare];
  [self.routePreview selectRouter:[MWMRouter type]];
  [self updateGoButtonTitle];
  [self.baseRoutePreviewStatus hide];
  [_transportRoutePreviewStatus hide];
  for (MWMRouteStartButton *button in self.goButtons)
    [button statePrepare];
}

- (void)statePlanning {
//  [self statePrepare];
  [self.routePreview router:[MWMRouter type] setState:MWMCircularProgressStateSpinner];
}

- (void)stateError:(NSString *_Nonnull)errorMessage {
  [self.routePreview router:[MWMRouter type] setState:MWMCircularProgressStateFailed];
  [self updateGoButtonTitle];
  [self.baseRoutePreviewStatus showErrorWithMessage:errorMessage];
  for (MWMRouteStartButton *button in self.goButtons)
    [button stateError];
}

- (void)stateReady {
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
}

- (void)onRouteStop {
}

- (void)stateNavigation {
  self.routePreview = nil;
  self.navigationInfoView.state = MWMNavigationInfoViewStateNavigation;
  self.navigationControlView.isVisible = YES;
  [self.baseRoutePreviewStatus hide];
  [_transportRoutePreviewStatus hide];
  _transportRoutePreviewStatus = nil;
}

#pragma mark - MWMRoutePreviewStatus

- (IBAction)showRouteManager {
  MWMRouteManagerViewModel * routeManagerViewModel = [[MWMRouteManagerViewModel alloc] init];
  MWMRouteManagerViewController * routeManager = [[MWMRouteManagerViewController alloc] initWithViewModel:routeManagerViewModel];
  routeManager.modalPresentationStyle = UIModalPresentationCustom;

  self.routeManagerTransitioningManager = [[MWMRouteManagerTransitioningManager alloc] init];
  routeManager.transitioningDelegate = self.routeManagerTransitioningManager;

  [[MapViewController sharedController] presentViewController:routeManager animated:YES completion:nil];
}

#pragma mark - Properties

@synthesize routePreview = _routePreview;
- (MWMRoutePreview *)routePreview {
  if (!_routePreview)
    [self loadPreview];
  return _routePreview;
}

- (void)setRoutePreview:(MWMRoutePreview *)routePreview {
  if (routePreview == _routePreview)
    return;
  [_routePreview remove];
  _routePreview = routePreview;
  _routePreview.delegate = self.delegate;
}

- (MWMBaseRoutePreviewStatus *)baseRoutePreviewStatus {
  if (!_baseRoutePreviewStatus)
    [self loadPreview];
  return _baseRoutePreviewStatus;
}

- (MWMTransportRoutePreviewStatus *)transportRoutePreviewStatus {
  if (!_transportRoutePreviewStatus)
    [self loadPreview];
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

#pragma mark - Button tap Actions

- (void)ttsButtonAction {
  [self.delegate ttsButtonDidTap];
}

- (void)settingsButtonAction {
  [self.delegate settingsButtonDidTap];
}

- (void)stopRoutingButtonAction {
  [self.delegate stopRoutingButtonDidTap];
}

- (IBAction)routingStartTouchUpInside {
  [self.delegate routingStartButtonDidTap];
}

#pragma mark - SearchOnMapManagerObserver

- (void)searchManagerWithDidChangeState:(SearchOnMapState)state {
  switch (state) {
    case SearchOnMapStateClosed:
      [self.navigationInfoView setSearchState:NavigationSearchStateMinimizedNormal animated:YES];
      break;
    case SearchOnMapStateHidden:
    case SearchOnMapStateSearching:
      [self.navigationInfoView setMapSearch];
  }
}

#pragma mark - Available area

- (void)updateNavigationInfoAvailableArea:(CGRect)frame {
  _navigationInfoView.availableArea = frame;
}

@end
