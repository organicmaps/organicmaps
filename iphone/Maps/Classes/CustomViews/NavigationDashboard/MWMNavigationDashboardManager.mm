#import "MWMNavigationDashboardManager.h"
#import "Common.h"
#import "MWMLocationHelpers.h"
#import "MWMMapViewControlsManager.h"
#import "MWMNavigationInfoView.h"
#import "MWMRoutePreview.h"
#import "MWMRouter.h"
#import "MWMTextToSpeech.h"
#import "Macros.h"
#import "MapViewController.h"
#import "MapsAppDelegate.h"
#import "Statistics.h"

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
  for (TInfoDisplay infoDisplay in self.infoDisplays)
    [infoDisplay updateNavigationInfo:self.entity];
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

- (IBAction)routingStartTouchUpInside { [[MWMRouter router] start]; }
#pragma mark - State changes

- (void)hideState
{
  [self.routePreview remove];
  self.routePreview = nil;
  [self.navigationInfoView remove];
  self.navigationInfoView = nil;
  [MWMMapViewControlsManager manager].searchHidden = YES;
}

- (void)showStatePrepare
{
  [self.navigationInfoView remove];
  self.navigationInfoView = nil;
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
  self.routePreview = nil;
  [self.navigationInfoView addToView:self.ownerView];
  [MWMMapViewControlsManager manager].searchHidden = YES;
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
  if (_state == state)
    return;
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
}
- (void)setLeftBound:(CGFloat)leftBound
{
  _leftBound = leftBound;
  if (_routePreview && IPAD)
    self.routePreview.leftBound = leftBound;
  if (_navigationInfoView)
    self.navigationInfoView.leftBound = leftBound;
}

- (CGFloat)leftHeight
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

- (CGFloat)rightHeight
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

- (void)addInfoDisplay:(TInfoDisplay)infoDisplay { [self.infoDisplays addObject:infoDisplay]; }
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

- (CGFloat)extraCompassBottomOffset
{
  if (!_navigationInfoView)
    return 0;
  return self.navigationInfoView.extraCompassBottomOffset;
}

- (void)setMapSearch
{
  if (_navigationInfoView)
    [self.navigationInfoView setMapSearch];
}

@end
