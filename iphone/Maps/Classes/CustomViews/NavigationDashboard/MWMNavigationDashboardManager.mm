#import "MWMNavigationDashboardManager.h"
#import <AudioToolbox/AudioServices.h>
#import <Crashlytics/Crashlytics.h>
#import "MWMCommon.h"
#import "MWMLocationHelpers.h"
#import "MWMMapViewControlsManager.h"
#import "MWMNavigationInfoView.h"
#import "MWMRoutePoint+CPP.h"
#import "MWMRoutePreview.h"
#import "MWMRouter.h"
#import "MWMTaxiPreviewDataSource.h"
#import "MWMTextToSpeech.h"
#import "MapViewController.h"
#import "MapsAppDelegate.h"
#import "Statistics.h"

#include "platform/platform.hpp"

namespace
{
NSString * const kRoutePreviewXibName = @"MWMRoutePreview";
NSString * const kRoutePreviewIPADXibName = @"MWMiPadRoutePreview";
NSString * const kNavigationInfoViewXibName = @"MWMNavigationInfoView";

using TInfoDisplay = id<MWMNavigationDashboardInfoProtocol>;
using TInfoDisplays = NSHashTable<__kindof TInfoDisplay>;
}  // namespace

@interface MWMMapViewControlsManager ()

@property(nonatomic) MWMNavigationDashboardManager * navigationManager;

@end

@interface MWMNavigationDashboardManager ()

@property(nonatomic, readwrite) IBOutlet MWMRoutePreview * routePreview;
@property(nonatomic) IBOutlet MWMNavigationInfoView * navigationInfoView;

@property(nonatomic) TInfoDisplays * infoDisplays;

@property(weak, nonatomic) UIView * ownerView;

@property(nonatomic) MWMNavigationDashboardEntity * entity;

@end

@implementation MWMNavigationDashboardManager

+ (MWMNavigationDashboardManager *)manager
{
  return [MWMMapViewControlsManager manager].navigationManager;
}

- (instancetype)initWithParentView:(UIView *)view
                          delegate:(id<MWMNavigationDashboardManagerProtocol>)delegate
{
  self = [super init];
  if (self)
  {
    _ownerView = view;
    _delegate = delegate;
    _infoDisplays = [TInfoDisplays weakObjectsHashTable];
  }
  return self;
}

- (void)updateFollowingInfo:(location::FollowingInfo const &)info
{
  if ([MWMRouter isRouteFinished])
  {
    [MWMRouter stopRouting];
    AudioServicesPlaySystemSound(kSystemSoundID_Vibrate);
    return;
  }

  [self.entity updateFollowingInfo:info];
  [self updateDashboard];
}

- (void)handleError
{
  if ([MWMRouter isTaxi])
    return;

  [self.routePreview stateError];
  [self.routePreview router:[MWMRouter type] setState:MWMCircularProgressStateFailed];
}

- (void)updateDashboard
{
  if (!self.entity.isValid)
    return;
  for (TInfoDisplay infoDisplay in self.infoDisplays)
    [infoDisplay updateNavigationInfo:self.entity];
}

#pragma mark - MWMRoutePreview

- (void)setRouteBuilderProgress:(CGFloat)progress
{
  [self.routePreview router:[MWMRouter type] setProgress:progress / 100.];
}

#pragma mark - MWMNavigationDashboard

- (IBAction)routingStopTouchUpInside
{
  if (IPAD && self.state != MWMNavigationDashboardStateNavigation)
    [self.delegate routePreviewDidChangeFrame:{}];
  [MWMRouter stopRouting];
}

#pragma mark - MWMTaxiDataSource

- (MWMTaxiPreviewDataSource *)taxiDataSource
{
  if (!_taxiDataSource)
  {
    _taxiDataSource = [[MWMTaxiPreviewDataSource alloc] initWithCollectionView:IPAD ?
                       self.routePreview.taxiCollectionView : self.delegate.taxiCollectionView];
  }
  return _taxiDataSource;
}

#pragma mark - MWMNavigationGo

- (IBAction)routingStartTouchUpInside { [MWMRouter startRouting]; }

#pragma mark - State changes

- (void)hideState
{
  [self.routePreview remove];
  self.routePreview = nil;
  self.navigationInfoView.state = MWMNavigationInfoViewStateHidden;
  self.navigationInfoView = nil;
}

- (void)showStatePrepare
{
  self.navigationInfoView.state = MWMNavigationInfoViewStatePrepare;
  [self.routePreview addToView:self.ownerView];
  [self.routePreview statePrepare];
  [self.routePreview selectRouter:[MWMRouter type]];
  [self setMenuState:MWMBottomMenuStateHidden];
}

- (void)showStatePlanning
{
  [self showStatePrepare];
  [self.routePreview router:[MWMRouter type] setState:MWMCircularProgressStateSpinner];
  [self setRouteBuilderProgress:0.];
  if (![MWMRouter isTaxi])
    return;

  auto showError = ^(NSString * errorMessage)
  {
    [self.routePreview stateError];
    [self.routePreview router:MWMRouterTypeTaxi setState:MWMCircularProgressStateFailed];
    [self setMenuErrorStateWithErrorMessage:errorMessage];
  };

  auto pFrom = [MWMRouter startPoint];
  auto pTo = [MWMRouter finishPoint];
  if (!pFrom || !pTo)
    return;
  if (!Platform::IsConnected())
  {
    [[MapViewController controller].alertController presentNoConnectionAlert];
    showError(L(@"dialog_taxi_offline"));
    return;
  }
  [self.taxiDataSource requestTaxiFrom:pFrom
                                    to:pTo
                            completion:^{
                              [self setMenuState:MWMBottomMenuStateGo];
                              [self.routePreview stateReady];
                              [self setRouteBuilderProgress:100.];
                            }
                               failure:^(NSString * errorMessage) {
                                 showError(errorMessage);
                               }];
}

- (void)showStateReady
{
  if ([MWMRouter isTaxi])
    return;

  [self setMenuState:MWMBottomMenuStateGo];
  [self.routePreview stateReady];
}

- (void)showStateNavigation
{
  [self setMenuState:MWMBottomMenuStateRouting];
  [self.routePreview remove];
  self.routePreview = nil;
  self.navigationInfoView.state = MWMNavigationInfoViewStateNavigation;
}

- (void)updateStartButtonTitle:(UIButton *)startButton
{
  auto t = self.startButtonTitle;
  [startButton setTitle:t forState:UIControlStateNormal];
  [startButton setTitle:t forState:UIControlStateDisabled];
}

- (void)onRoutePointsUpdated { [self.navigationInfoView updateToastView]; }
- (void)setMenuErrorStateWithErrorMessage:(NSString *)message
{
  [self.delegate setRoutingErrorMessage:message];
  [self setMenuState:MWMBottomMenuStateRoutingError];
}

- (void)setMenuState:(MWMBottomMenuState)menuState
{
  id<MWMNavigationDashboardManagerProtocol> delegate = self.delegate;
  [delegate setMenuState:menuState];
  [delegate setMenuRestoreState:menuState];
}

- (void)mwm_refreshUI
{
  if (_routePreview)
    [self.routePreview mwm_refreshUI];
  if (_navigationInfoView)
    [self.navigationInfoView mwm_refreshUI];
}

#pragma mark - Properties

- (void)setState:(MWMNavigationDashboardState)state
{
  switch (state)
  {
  case MWMNavigationDashboardStateHidden: [self hideState]; break;
  case MWMNavigationDashboardStatePrepare: [self showStatePrepare]; break;
  case MWMNavigationDashboardStatePlanning: [self showStatePlanning]; break;
  case MWMNavigationDashboardStateError:
    NSAssert(
        _state == MWMNavigationDashboardStatePlanning || _state == MWMNavigationDashboardStateReady,
        @"Invalid state change (error)");
    [self handleError];
    break;
  case MWMNavigationDashboardStateReady:
    NSAssert(_state == MWMNavigationDashboardStatePlanning, @"Invalid state change (ready)");
    [self showStateReady];
    break;
  case MWMNavigationDashboardStateNavigation: [self showStateNavigation]; break;
  }
  _state = state;
  [[MapViewController controller] updateStatusBarStyle];
  [self updateDashboard];
}

- (void)setTopBound:(CGFloat)topBound
{
  _topBound = topBound;
  if (_routePreview)
    self.routePreview.topBound = topBound;
  if (_navigationInfoView)
    self.navigationInfoView.topBound = topBound;
}

- (void)setLeftBound:(CGFloat)leftBound
{
  _leftBound = leftBound;
  if (_routePreview && IPAD)
    self.routePreview.leftBound = leftBound;
  if (_navigationInfoView)
    self.navigationInfoView.leftBound = leftBound;
}

- (CGFloat)leftTop
{
  switch (self.state)
  {
  case MWMNavigationDashboardStateHidden: return 0.0;
  case MWMNavigationDashboardStatePlanning:
  case MWMNavigationDashboardStateReady:
  case MWMNavigationDashboardStateError:
  case MWMNavigationDashboardStatePrepare:
    return IPAD ? self.topBound : self.routePreview.visibleHeight;
  case MWMNavigationDashboardStateNavigation: return self.navigationInfoView.leftHeight;
  }
}

- (CGFloat)rightTop
{
  switch (self.state)
  {
  case MWMNavigationDashboardStateHidden: return 0.0;
  case MWMNavigationDashboardStatePlanning:
  case MWMNavigationDashboardStateReady:
  case MWMNavigationDashboardStateError:
  case MWMNavigationDashboardStatePrepare:
    return IPAD ? self.topBound : self.routePreview.visibleHeight;
  case MWMNavigationDashboardStateNavigation: return self.navigationInfoView.rightHeight;
  }
}

- (CGFloat)bottom
{
  auto ov = self.ownerView;
  switch (self.state)
  {
  case MWMNavigationDashboardStateHidden: return ov.height;
  case MWMNavigationDashboardStatePlanning:
  case MWMNavigationDashboardStateReady:
  case MWMNavigationDashboardStateError:
  case MWMNavigationDashboardStatePrepare:
  case MWMNavigationDashboardStateNavigation:
    return IPAD ? ov.height : self.navigationInfoView.bottom;
  }
}

- (CGFloat)left
{
  switch (self.state)
  {
  case MWMNavigationDashboardStateHidden: return 0.0;
  case MWMNavigationDashboardStatePlanning:
  case MWMNavigationDashboardStateReady:
  case MWMNavigationDashboardStateError:
  case MWMNavigationDashboardStatePrepare:
  case MWMNavigationDashboardStateNavigation:
    return self.leftBound + (IPAD ? 0 : self.navigationInfoView.left);
  }
}

- (void)addInfoDisplay:(TInfoDisplay)infoDisplay { [self.infoDisplays addObject:infoDisplay]; }
- (NSString *)startButtonTitle
{
  if (![MWMRouter isTaxi])
    return L(@"p2p_start");
  return self.taxiDataSource.isTaxiInstalled ? L(@"taxi_order") : L(@"install_app");
}

#pragma mark - Properties

- (MWMRoutePreview *)routePreview
{
  if (!_routePreview)
  {
    [NSBundle.mainBundle loadNibNamed:IPAD ? kRoutePreviewIPADXibName : kRoutePreviewXibName
                                owner:self
                              options:nil];
    _routePreview.dashboardManager = self;
    _routePreview.delegate = self.delegate;
    [self addInfoDisplay:_routePreview];
  }
  return _routePreview;
}

- (MWMNavigationInfoView *)navigationInfoView
{
  if (!_navigationInfoView)
  {
    [NSBundle.mainBundle loadNibNamed:kNavigationInfoViewXibName owner:self options:nil];
    _navigationInfoView.state = MWMNavigationInfoViewStateHidden;
    _navigationInfoView.ownerView = self.ownerView;
    [self addInfoDisplay:_navigationInfoView];
  }
  return _navigationInfoView;
}

- (MWMNavigationDashboardEntity *)entity
{
  if (!_entity)
    _entity = [[MWMNavigationDashboardEntity alloc] init];
  return _entity;
}

- (void)setMapSearch
{
  if (_navigationInfoView)
    [self.navigationInfoView setMapSearch];
}

@end
