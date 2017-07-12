#import "MWMMapViewControlsManager.h"
#import "EAGLView.h"
#import "MWMAPIBar.h"
#import "MWMAddPlaceNavigationBar.h"
#import "MWMAlertViewController.h"
#import "MWMAlertViewController.h"
#import "MWMAuthorizationCommon.h"
#import "MWMBottomMenuViewController.h"
#import "MWMButton.h"
#import "MWMCommon.h"
#import "MWMFrameworkListener.h"
#import "MWMNetworkPolicy.h"
#import "MWMObjectsCategorySelectorController.h"
#import "MWMPlacePageManager.h"
#import "MWMRoutePreview.h"
#import "MWMRouter.h"
#import "MWMSearchManager.h"
#import "MWMSideButtons.h"
#import "MWMTaxiPreviewDataSource.h"
#import "MWMToast.h"
#import "MWMTrafficButtonViewController.h"
#import "MapViewController.h"
#import "MapsAppDelegate.h"
#import "Statistics.h"
#import "SwiftBridge.h"

#import "3party/Alohalytics/src/alohalytics_objc.h"

#include "Framework.h"

#include "platform/local_country_file_utils.hpp"
#include "platform/platform.hpp"

#include "storage/storage_helpers.hpp"

namespace
{
NSString * const kMapToCategorySelectorSegue = @"MapToCategorySelectorSegue";
}  // namespace

extern NSString * const kAlohalyticsTapEventKey;

@interface MWMMapViewControlsManager ()<MWMNavigationDashboardManagerProtocol,
                                        MWMBottomMenuControllerProtocol>

@property(nonatomic) MWMSideButtons * sideButtons;
@property(nonatomic) MWMTrafficButtonViewController * trafficButton;
@property(nonatomic) MWMBottomMenuViewController * menuController;
@property(nonatomic) id<MWMPlacePageProtocol> placePageManager;
@property(nonatomic) MWMNavigationDashboardManager * navigationManager;
@property(nonatomic) MWMSearchManager * searchManager;

@property(weak, nonatomic) MapViewController * ownerController;

@property(nonatomic) BOOL disableStandbyOnRouteFollowing;

@property(nonatomic) CGFloat topBound;
@property(nonatomic) CGFloat leftBound;

@end

@implementation MWMMapViewControlsManager

+ (MWMMapViewControlsManager *)manager { return [MapViewController controller].controlsManager; }
- (instancetype)initWithParentController:(MapViewController *)controller
{
  if (!controller)
    return nil;
  self = [super init];
  if (!self)
    return nil;
  self.ownerController = controller;
  self.hidden = NO;
  self.sideButtonsHidden = NO;
  self.trafficButtonHidden = NO;
  self.isDirectionViewHidden = YES;
  self.menuState = MWMBottomMenuStateInactive;
  self.menuRestoreState = MWMBottomMenuStateInactive;
  return self;
}

- (UIStatusBarStyle)preferredStatusBarStyle
{
  if ([MWMToast affectsStatusBar])
    return [MWMToast preferredStatusBarStyle];

  BOOL const isSearchUnderStatusBar = !self.searchHidden;
  BOOL const isNavigationUnderStatusBar =
      self.navigationManager.state != MWMNavigationDashboardStateHidden &&
      self.navigationManager.state != MWMNavigationDashboardStateNavigation;
  BOOL const isMenuViewUnderStatusBar = self.menuState == MWMBottomMenuStateActive ||
                                        self.menuState == MWMBottomMenuStateRoutingExpanded;
  BOOL const isDirectionViewUnderStatusBar = !self.isDirectionViewHidden;
  BOOL const isAddPlaceUnderStatusBar =
      [self.ownerController.view hasSubviewWithViewClass:[MWMAddPlaceNavigationBar class]];
  BOOL const isNightMode = [UIColor isNightMode];
  BOOL const isSomethingUnderStatusBar = isSearchUnderStatusBar || isNavigationUnderStatusBar ||
                                         isDirectionViewUnderStatusBar ||
                                         isMenuViewUnderStatusBar || isAddPlaceUnderStatusBar;

  setStatusBarBackgroundColor(isSomethingUnderStatusBar ? [UIColor clearColor]
                                                        : [UIColor statusBarBackground]);
  return isSomethingUnderStatusBar || isNightMode ? UIStatusBarStyleLightContent
                                                  : UIStatusBarStyleDefault;
}

#pragma mark - My Position

- (void)processMyPositionStateModeEvent:(location::EMyPositionMode)mode
{
  [self.sideButtons processMyPositionStateModeEvent:mode];
}

#pragma mark - Layout

- (void)mwm_refreshUI
{
  [self.trafficButton mwm_refreshUI];
  [self.sideButtons mwm_refreshUI];
  [self.navigationManager mwm_refreshUI];
  [self.searchManager mwm_refreshUI];
  [self.menuController mwm_refreshUI];
  [self.placePageManager mwm_refreshUI];
  [self.ownerController setNeedsStatusBarAppearanceUpdate];
}

- (void)viewWillTransitionToSize:(CGSize)size
       withTransitionCoordinator:(id<UIViewControllerTransitionCoordinator>)coordinator
{
  [self.trafficButton viewWillTransitionToSize:size withTransitionCoordinator:coordinator];
  [self.menuController viewWillTransitionToSize:size withTransitionCoordinator:coordinator];
  [self.searchManager viewWillTransitionToSize:size withTransitionCoordinator:coordinator];
  [coordinator animateAlongsideTransition:^(
                   id<UIViewControllerTransitionCoordinatorContext> _Nonnull context) {
    self.navigationManager.leftBound = self.leftBound;
  }
                               completion:nil];
}

#pragma mark - MWMPlacePageViewManager

- (void)dismissPlacePage
{
  self.trafficButtonHidden = NO;
  [self.placePageManager close];
}

- (void)showPlacePage:(place_page::Info const &)info
{
  auto show = ^(place_page::Info const & info) {
    self.trafficButtonHidden = YES;
    [self.placePageManager show:info];
  };

  using namespace network_policy;
  if (GetPlatform().ConnectionStatus() == Platform::EConnectionType::CONNECTION_WWAN &&
      !CanUseNetwork() && GetStage() == platform::NetworkPolicy::Stage::Session)
  {
    [[MWMAlertViewController activeAlertController]
        presentMobileInternetAlertWithBlock:[show, info] { show(info); }];
  }
  else
  {
    show(info);
  }
}

- (void)searchViewDidEnterState:(MWMSearchManagerState)state
{
  if (state == MWMSearchManagerStateMapSearch)
  {
    [self.navigationManager setMapSearch];
  }
  if (state == MWMSearchManagerStateHidden)
  {
    if (!IPAD)
    {
      self.hidden = NO;
      self.leftBound = self.topBound = 0.0;
    }
  }
  [self.ownerController setNeedsStatusBarAppearanceUpdate];
}

- (void)searchFrameUpdated:(CGRect)frame
{
  CGSize const s = frame.size;
  self.leftBound = s.width;
  self.topBound = s.height;
}

- (void)searchTextOnMap:(NSString *)text forInputLocale:(NSString *)locale
{
  if (![self searchText:text forInputLocale:locale])
    return;
  
  self.searchManager.state = MWMSearchManagerStateMapSearch;
}

- (BOOL)searchText:(NSString *)text forInputLocale:(NSString *)locale
{
  if (text.length == 0)
    return NO;
  
  self.searchManager.state = MWMSearchManagerStateTableSearch;
  [self.searchManager searchText:text forInputLocale:locale];
  return YES;
}

#pragma mark - MWMBottomMenuControllerProtocol

- (void)actionDownloadMaps:(mwm::DownloaderMode)mode
{
  MapViewController * ownerController = self.ownerController;
  if (platform::migrate::NeedMigrate())
  {
    if ([MWMRouter isRoutingActive])
    {
      [Statistics logEvent:kStatDownloaderMigrationProhibitedDialogue
            withParameters:@{kStatFrom : kStatDownloader}];
      [[MWMAlertViewController activeAlertController] presentMigrationProhibitedAlert];
    }
    else
    {
      [Statistics logEvent:kStatDownloaderMigrationDialogue
            withParameters:@{kStatFrom : kStatDownloader}];
      [ownerController openMigration];
    }
  }
  else
  {
    [ownerController openMapsDownloader:mode];
  }
}

- (void)closeInfoScreens
{
  if (IPAD)
  {
    if (!self.searchHidden)
      self.searchManager.state = MWMSearchManagerStateHidden;
    else
      [MWMRouter stopRouting];
  }
  else
  {
    CGSize const ownerViewSize = self.ownerController.view.size;
    if (ownerViewSize.width > ownerViewSize.height)
      [self dismissPlacePage];
  }
}

- (void)didFinishAddingPlace
{
  self.trafficButtonHidden = NO;
  self.menuState = MWMBottomMenuStateInactive;
  static_cast<EAGLView *>(self.ownerController.view).widgetsManager.fullScreen = NO;
}

- (void)addPlace:(BOOL)isBusiness hasPoint:(BOOL)hasPoint point:(m2::PointD const &)point
{
  self.trafficButtonHidden = YES;
  self.menuState = MWMBottomMenuStateHidden;
  MapViewController * ownerController = self.ownerController;
  static_cast<EAGLView *>(ownerController.view).widgetsManager.fullScreen = YES;
  [self.placePageManager close];
  self.searchManager.state = MWMSearchManagerStateHidden;

  [MWMAddPlaceNavigationBar showInSuperview:ownerController.view
      isBusiness:isBusiness
      applyPosition:hasPoint
      position:point
      doneBlock:^{
        auto & f = GetFramework();

        if (IsPointCoveredByDownloadedMaps(f.GetViewportCenter(), f.GetStorage(),
                                           f.GetCountryInfoGetter()))
          [ownerController performSegueWithIdentifier:kMapToCategorySelectorSegue sender:nil];
        else
          [ownerController.alertController presentIncorrectFeauturePositionAlert];

        [self didFinishAddingPlace];
      }
      cancelBlock:^{
        [self didFinishAddingPlace];
      }];
  [ownerController setNeedsStatusBarAppearanceUpdate];
}

#pragma mark - MWMNavigationDashboardManager

- (void)routePreviewDidChangeFrame:(CGRect)newFrame
{
  if (IPAD)
  {
    CGFloat const bound = newFrame.origin.x + newFrame.size.width;
    self.navigationManager.leftBound = bound;
    self.trafficButton.leftBound = bound;
  }
  else
  {
    CGFloat const bound = newFrame.origin.y + newFrame.size.height;
    self.sideButtons.topBound = bound;
    self.trafficButton.topBound = bound;
  }
}

- (void)navigationDashBoardDidUpdate
{
  auto nm = self.navigationManager;
  if (IPAD)
    return;
  auto const topBound = self.topBound + nm.rightTop;
  auto const bottomBound = nm.bottom;
  [self.sideButtons setTopBound:topBound];
  [self.sideButtons setBottomBound:bottomBound];
  [MWMMapWidgets widgetsManager].bottomBound = bottomBound;
  [MWMMapWidgets widgetsManager].leftBound = nm.left;
}

- (void)setDisableStandbyOnRouteFollowing:(BOOL)disableStandbyOnRouteFollowing
{
  if (_disableStandbyOnRouteFollowing == disableStandbyOnRouteFollowing)
    return;
  _disableStandbyOnRouteFollowing = disableStandbyOnRouteFollowing;
  if (disableStandbyOnRouteFollowing)
    [[MapsAppDelegate theApp] disableStandby];
  else
    [[MapsAppDelegate theApp] enableStandby];
}

- (MWMTaxiCollectionView *)taxiCollectionView
{
  return self.menuController.taxiCollectionView;
}

#pragma mark - Routing

- (void)onRoutePrepare
{
  self.navigationManager.state = MWMNavigationDashboardStatePrepare;
  self.menuController.p2pButton.selected = YES;
  [self onRoutePointsUpdated];
}

- (void)onRouteRebuild
{
  if (IPAD)
    self.searchManager.state = MWMSearchManagerStateHidden;

  self.navigationManager.state = MWMNavigationDashboardStatePlanning;
}

- (void)onRouteError
{
  self.navigationManager.state = MWMNavigationDashboardStateError;
}

- (void)onRouteReady
{
  auto startPoint = [MWMRouter startPoint];
  if (!startPoint || !startPoint.isMyPosition)
  {
    dispatch_async(dispatch_get_main_queue(), ^{
      [MWMRouter disableFollowMode];
      [self.navigationManager updateDashboard];
    });
  }
  if (self.navigationManager.state != MWMNavigationDashboardStateNavigation)
    self.navigationManager.state = MWMNavigationDashboardStateReady;
}

- (void)onRouteStart
{
  self.hidden = NO;
  self.sideButtons.zoomHidden = self.zoomHidden;
  self.sideButtonsHidden = NO;
  self.disableStandbyOnRouteFollowing = YES;
  self.trafficButtonHidden = YES;
  self.navigationManager.state = MWMNavigationDashboardStateNavigation;
}

- (void)onRouteStop
{
  self.sideButtons.zoomHidden = self.zoomHidden;
  self.navigationManager.state = MWMNavigationDashboardStateHidden;
  self.disableStandbyOnRouteFollowing = NO;
  self.trafficButtonHidden = NO;
  self.menuState = MWMBottomMenuStateInactive;
  [self navigationDashBoardDidUpdate];
}

- (void)onRoutePointsUpdated { [self.navigationManager onRoutePointsUpdated]; }
#pragma mark - Properties

- (MWMSideButtons *)sideButtons
{
  if (!_sideButtons)
    _sideButtons = [[MWMSideButtons alloc] initWithParentView:self.ownerController.view];
  return _sideButtons;
}

- (MWMTrafficButtonViewController *)trafficButton
{
  if (!_trafficButton)
    _trafficButton = [[MWMTrafficButtonViewController alloc] init];
  return _trafficButton;
}

- (MWMBottomMenuViewController *)menuController
{
  if (!_menuController)
    _menuController =
        [[MWMBottomMenuViewController alloc] initWithParentController:self.ownerController
                                                             delegate:self];
  return _menuController;
}

- (id<MWMPlacePageProtocol>)placePageManager
{
  if (!_placePageManager)
    _placePageManager = [[MWMPlacePageManager alloc] init];
  return _placePageManager;
}

- (MWMNavigationDashboardManager *)navigationManager
{
  if (!_navigationManager)
  {
    _navigationManager =
        [[MWMNavigationDashboardManager alloc] initWithParentView:self.ownerController.view
                                                         delegate:self];
    [_navigationManager addInfoDisplay:self.menuController];
  }
  return _navigationManager;
}

- (MWMSearchManager *)searchManager
{
  if (!_searchManager)
    _searchManager = [[MWMSearchManager alloc] init];
  return _searchManager;
}

@synthesize menuState = _menuState;

- (void)setHidden:(BOOL)hidden
{
  if (_hidden == hidden)
    return;
  _hidden = hidden;
  self.sideButtonsHidden = _sideButtonsHidden;
  self.trafficButtonHidden = _trafficButtonHidden;
  self.menuState = _menuState;
  EAGLView * glView = (EAGLView *)self.ownerController.view;
  glView.widgetsManager.fullScreen = hidden;
}

- (void)setZoomHidden:(BOOL)zoomHidden
{
  _zoomHidden = zoomHidden;
  self.sideButtons.zoomHidden = zoomHidden;
}

- (void)setSideButtonsHidden:(BOOL)sideButtonsHidden
{
  _sideButtonsHidden = sideButtonsHidden;
  self.sideButtons.hidden = self.hidden || sideButtonsHidden;
}

- (void)setTrafficButtonHidden:(BOOL)trafficButtonHidden
{
  BOOL const isNavigation = self.navigationManager.state == MWMNavigationDashboardStateNavigation;
  _trafficButtonHidden = isNavigation || trafficButtonHidden;
  self.trafficButton.hidden = self.hidden || _trafficButtonHidden;
}

- (void)setMenuState:(MWMBottomMenuState)menuState
{
  _menuState = menuState;
  self.menuController.state = self.hidden ? MWMBottomMenuStateHidden : menuState;
}

- (void)setRoutingErrorMessage:(NSString *)message
{
  [self.menuController setRoutingErrorMessage:message];
}

- (MWMBottomMenuState)menuState
{
  MWMBottomMenuState const state = self.menuController.state;
  if (state != MWMBottomMenuStateHidden)
    return state;
  return _menuState;
}

- (MWMNavigationDashboardState)navigationState { return self.navigationManager.state; }
- (void)setTopBound:(CGFloat)topBound
{
  if (IPAD)
    return;
  _topBound = topBound;
  self.sideButtons.topBound = topBound;
  self.navigationManager.topBound = topBound;
  self.trafficButton.topBound = topBound;
}

- (void)setLeftBound:(CGFloat)leftBound
{
  if (!IPAD)
    return;
  if ([MWMRouter isRoutingActive])
    return;
  _leftBound = self.navigationManager.leftBound = self.menuController.leftBound =
      self.trafficButton.leftBound = leftBound;
}

- (BOOL)searchHidden { return self.searchManager.state == MWMSearchManagerStateHidden; }
- (void)setSearchHidden:(BOOL)searchHidden
{
  self.searchManager.state =
      searchHidden ? MWMSearchManagerStateHidden : MWMSearchManagerStateDefault;
}

#pragma mark - MWMFeatureHolder

- (id<MWMFeatureHolder>)featureHolder { return self.placePageManager; }

#pragma mark - MWMBookingInfoHolder
- (id<MWMBookingInfoHolder>)bookingInfoHolder { return self.placePageManager; }

@end
