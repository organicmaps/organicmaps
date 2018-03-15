#import "MWMMapViewControlsManager.h"
#import "MWMNavigationDashboardManager.h"
#import "MWMNavigationInfoView.h"
#import "MWMRoutePreview.h"
#import "MWMSearch.h"
#import "MapViewController.h"

#import <AudioToolbox/AudioServices.h>
#import <Crashlytics/Crashlytics.h>
#import "SwiftBridge.h"

#include "platform/platform.hpp"

extern NSString * const kAlohalyticsTapEventKey;

namespace
{
NSString * const kRoutePreviewIPADXibName = @"MWMiPadRoutePreview";
NSString * const kRoutePreviewIPhoneXibName = @"MWMiPhoneRoutePreview";
NSString * const kNavigationInfoViewXibName = @"MWMNavigationInfoView";
NSString * const kNavigationControlViewXibName = @"NavigationControlView";

using Observer = id<MWMNavigationDashboardObserver>;
using Observers = NSHashTable<Observer>;
}  // namespace

@interface MWMMapViewControlsManager ()

@property(nonatomic) MWMNavigationDashboardManager * navigationManager;

@end

@interface MWMNavigationDashboardManager ()<MWMSearchManagerObserver>

@property(copy, nonatomic) NSDictionary * etaAttributes;
@property(copy, nonatomic) NSDictionary * etaSecondaryAttributes;
@property(copy, nonatomic) NSString * errorMessage;
@property(nonatomic) IBOutlet MWMBaseRoutePreviewStatus * baseRoutePreviewStatus;
@property(nonatomic) IBOutlet MWMNavigationControlView * navigationControlView;
@property(nonatomic) IBOutlet MWMNavigationInfoView * navigationInfoView;
@property(nonatomic) IBOutlet MWMRoutePreview * routePreview;
@property(nonatomic) IBOutlet MWMTransportRoutePreviewStatus * transportRoutePreviewStatus;
@property(nonatomic) IBOutletCollection(MWMRouteStartButton) NSArray * goButtons;
@property(nonatomic) MWMNavigationDashboardEntity * entity;
@property(nonatomic) MWMRouteManagerTransitioningManager * routeManagerTransitioningManager;
@property(nonatomic) Observers * observers;
@property(nonatomic, readwrite) MWMTaxiPreviewDataSource * taxiDataSource;
@property(weak, nonatomic) IBOutlet MWMTaxiCollectionView * taxiCollectionView;
@property(weak, nonatomic) IBOutlet UIButton * showRouteManagerButton;
@property(weak, nonatomic) IBOutlet UIView * goButtonsContainer;
@property(weak, nonatomic) UIView * ownerView;

@end

@implementation MWMNavigationDashboardManager

+ (MWMNavigationDashboardManager *)manager
{
  return [MWMMapViewControlsManager manager].navigationManager;
}

- (instancetype)initWithParentView:(UIView *)view
{
  self = [super init];
  if (self)
  {
    _ownerView = view;
    _observers = [Observers weakObjectsHashTable];
  }
  return self;
}

- (void)loadPreviewWithStatusBoxes
{
  [NSBundle.mainBundle loadNibNamed:IPAD ? kRoutePreviewIPADXibName : kRoutePreviewIPhoneXibName
                              owner:self
                            options:nil];
  auto ownerView = self.ownerView;
  _baseRoutePreviewStatus.ownerView = ownerView;
  _transportRoutePreviewStatus.ownerView = ownerView;
}

#pragma mark - MWMRoutePreview

- (void)setRouteBuilderProgress:(CGFloat)progress
{
  [self.routePreview router:[MWMRouter type] setProgress:progress / 100.];
}

#pragma mark - MWMNavigationGo

- (IBAction)routingStartTouchUpInside { [MWMRouter startRouting]; }
- (void)updateGoButtonTitle
{
  NSString * title = nil;
  if ([MWMRouter isTaxi])
    title = [self.taxiDataSource isTaxiInstalled] ? L(@"taxi_order") : L(@"install_app");
  else
    title = L(@"p2p_start");

  for (MWMRouteStartButton * button in self.goButtons)
    [button setTitle:title forState:UIControlStateNormal];
}

- (void)mwm_refreshUI
{
  [_routePreview mwm_refreshUI];
  [_navigationInfoView mwm_refreshUI];
  [_navigationControlView mwm_refreshUI];
  [_baseRoutePreviewStatus mwm_refreshUI];
  [_transportRoutePreviewStatus mwm_refreshUI];
  _etaAttributes = nil;
  _etaSecondaryAttributes = nil;
}

- (void)onNavigationInfoUpdated
{
  auto entity = self.entity;
  if (!entity.isValid)
    return;
  [_navigationInfoView onNavigationInfoUpdated:entity];
  if ([MWMRouter type] == MWMRouterTypePublicTransport)
    [_transportRoutePreviewStatus onNavigationInfoUpdated:entity];
  else
    [_baseRoutePreviewStatus onNavigationInfoUpdated:entity];
  [_navigationControlView onNavigationInfoUpdated:entity];
}

#pragma mark - On route updates

- (void)onRoutePrepare { self.state = MWMNavigationDashboardStatePrepare; }
- (void)onRoutePlanning { self.state = MWMNavigationDashboardStatePlanning; }
- (void)onRouteError:(NSString *)error
{
  self.errorMessage = error;
  self.state = MWMNavigationDashboardStateError;
}

- (void)onRouteReady
{
  if (self.state != MWMNavigationDashboardStateNavigation && ![MWMRouter isTaxi])
    self.state = MWMNavigationDashboardStateReady;
}

- (void)onRoutePointsUpdated
{
  if (self.state == MWMNavigationDashboardStateHidden)
    self.state = MWMNavigationDashboardStatePrepare;
  [self.navigationInfoView updateToastView];
}

#pragma mark - State changes

- (void)stateHidden
{
  self.taxiDataSource = nil;
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

- (void)statePrepare
{
  self.navigationInfoView.state = MWMNavigationInfoViewStatePrepare;
  auto routePreview = self.routePreview;
  [routePreview addToView:self.ownerView];
  [routePreview statePrepare];
  [routePreview selectRouter:[MWMRouter type]];
  [self updateGoButtonTitle];
  [_baseRoutePreviewStatus hide];
  [_transportRoutePreviewStatus hide];
  for (MWMRouteStartButton * button in self.goButtons)
    [button statePrepare];
}

- (void)statePlanning
{
  [self statePrepare];
  [self.routePreview router:[MWMRouter type] setState:MWMCircularProgressStateSpinner];
  [self setRouteBuilderProgress:0.];
  if (![MWMRouter isTaxi])
    return;

  auto pFrom = [MWMRouter startPoint];
  auto pTo = [MWMRouter finishPoint];
  if (!pFrom || !pTo)
    return;
  if (!Platform::IsConnected())
  {
    [[MapViewController controller].alertController presentNoConnectionAlert];
    [self onRouteError:L(@"dialog_taxi_offline")];
    return;
  }
  __weak auto wSelf = self;
  [self.taxiDataSource requestTaxiFrom:pFrom
      to:pTo
      completion:^{
        wSelf.state = MWMNavigationDashboardStateReady;
      }
      failure:^(NSString * error) {
        [wSelf onRouteError:error];
      }];
}

- (void)stateError
{
  NSAssert(_state == MWMNavigationDashboardStatePlanning, @"Invalid state change (error)");
  auto routePreview = self.routePreview;
  [routePreview router:[MWMRouter type] setState:MWMCircularProgressStateFailed];
  [self updateGoButtonTitle];
  [self.baseRoutePreviewStatus showErrorWithMessage:self.errorMessage];
  for (MWMRouteStartButton * button in self.goButtons)
    [button stateError];
}

- (void)stateReady
{
  NSAssert(_state == MWMNavigationDashboardStatePlanning, @"Invalid state change (ready)");
  [self setRouteBuilderProgress:100.];
  [self updateGoButtonTitle];
  auto const isTransport = ([MWMRouter type] == MWMRouterTypePublicTransport);
  if (isTransport)
    [self.transportRoutePreviewStatus showReady];
  else
    [self.baseRoutePreviewStatus showReady];
  self.goButtonsContainer.hidden = isTransport;
  for (MWMRouteStartButton * button in self.goButtons)
    [button stateReady];
}

- (void)onRouteStart { self.state = MWMNavigationDashboardStateNavigation; }
- (void)onRouteStop { self.state = MWMNavigationDashboardStateHidden; }
- (void)stateNavigation
{
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

- (IBAction)showRouteManager
{
  auto routeManagerViewModel = [[MWMRouteManagerViewModel alloc] init];
  auto routeManager =
      [[MWMRouteManagerViewController alloc] initWithViewModel:routeManagerViewModel];
  routeManager.modalPresentationStyle = UIModalPresentationCustom;
  if (IPAD)
  {
    self.routeManagerTransitioningManager = [[MWMRouteManagerTransitioningManager alloc]
        initWithPopoverSourceView:self.showRouteManagerButton
         permittedArrowDirections:UIPopoverArrowDirectionLeft];
  }
  else
  {
    self.routeManagerTransitioningManager = [[MWMRouteManagerTransitioningManager alloc] init];
  }
  routeManager.transitioningDelegate = self.routeManagerTransitioningManager;
  [[MapViewController controller] presentViewController:routeManager animated:YES completion:nil];
}

#pragma mark - MWMNavigationControlView

- (IBAction)ttsButtonAction
{
  BOOL const isEnabled = [MWMTextToSpeech tts].active;
  [Statistics logEvent:kStatMenu withParameters:@{kStatTTS : isEnabled ? kStatOn : kStatOff}];
  [MWMTextToSpeech tts].active = !isEnabled;
}

- (IBAction)trafficButtonAction
{
  BOOL const switchOn = ([MWMTrafficManager state] == MWMTrafficManagerStateDisabled);
  [Statistics logEvent:kStatMenu withParameters:@{kStatTraffic : switchOn ? kStatOn : kStatOff}];
  [MWMTrafficManager enableTraffic:switchOn];
}

- (IBAction)settingsButtonAction
{
  [Statistics logEvent:kStatMenu withParameters:@{kStatButton : kStatSettings}];
  [Alohalytics logEvent:kAlohalyticsTapEventKey withValue:@"settingsAndMore"];
  [[MapViewController controller] performSegueWithIdentifier:@"Map2Settings" sender:nil];
}

- (IBAction)stopRoutingButtonAction
{
  [MWMSearch clear];
  [MWMRouter stopRouting];
}

#pragma mark - Add/Remove Observers

+ (void)addObserver:(id<MWMNavigationDashboardObserver>)observer
{
  [[self manager].observers addObject:observer];
}

+ (void)removeObserver:(id<MWMNavigationDashboardObserver>)observer
{
  [[self manager].observers removeObject:observer];
}

#pragma mark - MWMNavigationDashboardObserver

- (void)onNavigationDashboardStateChanged
{
  for (Observer observer in self.observers)
    [observer onNavigationDashboardStateChanged];
}

#pragma mark - MWMSearchManagerObserver

- (void)onSearchManagerStateChanged
{
  auto state = [MWMSearchManager manager].state;
  if (state == MWMSearchManagerStateMapSearch)
    [self setMapSearch];
}

#pragma mark - Available area

+ (void)updateNavigationInfoAvailableArea:(CGRect)frame
{
  [[self manager] updateNavigationInfoAvailableArea:frame];
}

- (void)updateNavigationInfoAvailableArea:(CGRect)frame
{
  _navigationInfoView.availableArea = frame;
}
#pragma mark - Properties

- (NSDictionary *)etaAttributes
{
  if (!_etaAttributes)
  {
    _etaAttributes = @{
      NSForegroundColorAttributeName : [UIColor blackPrimaryText],
      NSFontAttributeName : [UIFont medium17]
    };
  }
  return _etaAttributes;
}

- (NSDictionary *)etaSecondaryAttributes
{
  if (!_etaSecondaryAttributes)
  {
    _etaSecondaryAttributes = @{
      NSForegroundColorAttributeName: [UIColor blackSecondaryText],
      NSFontAttributeName: [UIFont medium17]
    };
  }
  return _etaSecondaryAttributes;
}

- (void)setState:(MWMNavigationDashboardState)state
{
  if (_state == state)
    return;
  if (state == MWMNavigationDashboardStateHidden)
    [MWMSearchManager removeObserver:self];
  else
    [MWMSearchManager addObserver:self];
  switch (state)
  {
  case MWMNavigationDashboardStateHidden: [self stateHidden]; break;
  case MWMNavigationDashboardStatePrepare: [self statePrepare]; break;
  case MWMNavigationDashboardStatePlanning: [self statePlanning]; break;
  case MWMNavigationDashboardStateError: [self stateError]; break;
  case MWMNavigationDashboardStateReady: [self stateReady]; break;
  case MWMNavigationDashboardStateNavigation: [self stateNavigation]; break;
  }
  _state = state;
  [[MapViewController controller] updateStatusBarStyle];
  [self onNavigationDashboardStateChanged];
}

- (MWMTaxiPreviewDataSource *)taxiDataSource
{
  if (!_taxiDataSource)
    _taxiDataSource =
        [[MWMTaxiPreviewDataSource alloc] initWithCollectionView:self.taxiCollectionView];
  return _taxiDataSource;
}

@synthesize routePreview = _routePreview;
- (MWMRoutePreview *)routePreview
{
  if (!_routePreview)
    [self loadPreviewWithStatusBoxes];
  return _routePreview;
}

- (void)setRoutePreview:(MWMRoutePreview *)routePreview
{
  if (routePreview == _routePreview)
    return;
  [_routePreview remove];
  _routePreview = routePreview;
}

- (MWMBaseRoutePreviewStatus *)baseRoutePreviewStatus
{
  if (!_baseRoutePreviewStatus)
    [self loadPreviewWithStatusBoxes];
  return _baseRoutePreviewStatus;
}

- (MWMTransportRoutePreviewStatus *)transportRoutePreviewStatus
{
  if (!_transportRoutePreviewStatus)
    [self loadPreviewWithStatusBoxes];
  return _transportRoutePreviewStatus;
}

- (MWMNavigationInfoView *)navigationInfoView
{
  if (!_navigationInfoView)
  {
    [NSBundle.mainBundle loadNibNamed:kNavigationInfoViewXibName owner:self options:nil];
    _navigationInfoView.state = MWMNavigationInfoViewStateHidden;
    _navigationInfoView.ownerView = self.ownerView;
  }
  return _navigationInfoView;
}

- (MWMNavigationControlView *)navigationControlView
{
  if (!_navigationControlView)
  {
    [NSBundle.mainBundle loadNibNamed:kNavigationControlViewXibName owner:self options:nil];
    _navigationControlView.ownerView = self.ownerView;
  }
  return _navigationControlView;
}

- (MWMNavigationDashboardEntity *)entity
{
  if (!_entity)
    _entity = [[MWMNavigationDashboardEntity alloc] init];
  return _entity;
}

- (void)setMapSearch
{
  [_navigationInfoView setMapSearch];
}

@end
