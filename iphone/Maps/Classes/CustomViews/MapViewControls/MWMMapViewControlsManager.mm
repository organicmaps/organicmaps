#import "MWMMapViewControlsManager.h"
#import "MWMAddPlaceNavigationBar.h"
#import "MWMBottomMenuControllerProtocol.h"
#import "MWMCommon.h"
#import "MWMNetworkPolicy.h"
#import "MWMPlacePageManager.h"
#import "MWMPlacePageProtocol.h"
#import "MWMSearchManager.h"
#import "MWMSideButtons.h"
#import "MWMToast.h"
#import "MWMTrafficButtonViewController.h"
#import "MapViewController.h"
#import "MapsAppDelegate.h"
#import "SwiftBridge.h"

#include "Framework.h"

#include "platform/local_country_file_utils.hpp"
#include "platform/platform.hpp"

#include "storage/storage_helpers.hpp"

#include "map/place_page_info.hpp"

namespace
{
NSString * const kMapToCategorySelectorSegue = @"MapToCategorySelectorSegue";
}  // namespace

extern NSString * const kAlohalyticsTapEventKey;

@interface MWMMapViewControlsManager ()<MWMBottomMenuControllerProtocol, MWMSearchManagerObserver>

@property(nonatomic) MWMSideButtons * sideButtons;
@property(nonatomic) MWMTrafficButtonViewController * trafficButton;
@property(nonatomic) MWMBottomMenuViewController * menuController;
@property(nonatomic) id<MWMPlacePageProtocol> placePageManager;
@property(nonatomic) MWMNavigationDashboardManager * navigationManager;
@property(nonatomic) MWMSearchManager * searchManager;

@property(weak, nonatomic) MapViewController * ownerController;

@property(nonatomic) BOOL disableStandbyOnRouteFollowing;

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
  self.menuRestoreState = MWMBottomMenuStateInactive;
  return self;
}

- (UIStatusBarStyle)preferredStatusBarStyle
{
  if ([MWMToast affectsStatusBar])
    return [MWMToast preferredStatusBarStyle];

  BOOL const isSearchUnderStatusBar = (self.searchManager.state != MWMSearchManagerStateHidden);
  BOOL const isNavigationUnderStatusBar =
      self.navigationManager.state != MWMNavigationDashboardStateHidden &&
      self.navigationManager.state != MWMNavigationDashboardStateNavigation;
  BOOL const isMenuViewUnderStatusBar = self.menuState == MWMBottomMenuStateActive;
  BOOL const isDirectionViewUnderStatusBar = !self.isDirectionViewHidden;
  BOOL const isAddPlaceUnderStatusBar =
      [self.ownerController.view hasSubviewWithViewClass:[MWMAddPlaceNavigationBar class]];
  BOOL const isNightMode = [UIColor isNightMode];
  BOOL const isSomethingUnderStatusBar = isSearchUnderStatusBar || isNavigationUnderStatusBar ||
                                         isDirectionViewUnderStatusBar ||
                                         isMenuViewUnderStatusBar || isAddPlaceUnderStatusBar;

  setStatusBarBackgroundColor(isSomethingUnderStatusBar ? UIColor.clearColor
                                                        : [UIColor statusBarBackground]);
  return isSomethingUnderStatusBar || isNightMode ? UIStatusBarStyleLightContent
                                                  : UIStatusBarStyleDefault;
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
}

#pragma mark - MWMPlacePageViewManager

- (void)dismissPlacePage
{
  self.trafficButtonHidden = NO;
  [self.placePageManager dismiss];
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

- (void)actionDownloadMaps:(MWMMapDownloaderMode)mode
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

- (void)didFinishAddingPlace
{
  self.trafficButtonHidden = NO;
  self.menuState = MWMBottomMenuStateInactive;
}

- (void)addPlace:(BOOL)isBusiness hasPoint:(BOOL)hasPoint point:(m2::PointD const &)point
{
  self.trafficButtonHidden = YES;
  self.menuState = MWMBottomMenuStateHidden;
  MapViewController * ownerController = self.ownerController;
  [self.placePageManager dismiss];
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

#pragma mark - MWMSearchManagerObserver

- (void)onSearchManagerStateChanged
{
  auto state = [MWMSearchManager manager].state;
  if (!IPAD && state == MWMSearchManagerStateHidden)
    self.hidden = NO;
}

#pragma mark - Routing

- (void)onRoutePrepare
{
  auto nm = self.navigationManager;
  [nm onRoutePrepare];
  [nm onRoutePointsUpdated];
}

- (void)onRouteRebuild
{
  if (IPAD)
    self.searchManager.state = MWMSearchManagerStateHidden;

  [self.navigationManager onRoutePlanning];
}

- (void)onRouteReady
{
  self.searchManager.state = MWMSearchManagerStateHidden;
  [self.navigationManager onRouteReady];
}

- (void)onRouteStart
{
  self.hidden = NO;
  self.sideButtons.zoomHidden = self.zoomHidden;
  self.sideButtonsHidden = NO;
  self.disableStandbyOnRouteFollowing = YES;
  self.trafficButtonHidden = YES;
  [self.navigationManager onRouteStart];
}

- (void)onRouteStop
{
  self.searchManager.state = MWMSearchManagerStateHidden;
  self.sideButtons.zoomHidden = self.zoomHidden;
  [self.navigationManager onRouteStop];
  self.disableStandbyOnRouteFollowing = NO;
  self.trafficButtonHidden = NO;
}

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
    _navigationManager =
        [[MWMNavigationDashboardManager alloc] initWithParentView:self.ownerController.view];
  return _navigationManager;
}

- (MWMSearchManager *)searchManager
{
  if (!_searchManager)
  {
    _searchManager = [[MWMSearchManager alloc] init];
    [MWMSearchManager addObserver:self];
  }
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

- (MWMBottomMenuState)menuState
{
  MWMBottomMenuState const state = self.menuController.state;
  if (state != MWMBottomMenuStateHidden)
    return state;
  return _menuState;
}

#pragma mark - MWMFeatureHolder

- (id<MWMFeatureHolder>)featureHolder { return self.placePageManager; }

#pragma mark - MWMBookingInfoHolder
- (id<MWMBookingInfoHolder>)bookingInfoHolder { return self.placePageManager; }

@end
