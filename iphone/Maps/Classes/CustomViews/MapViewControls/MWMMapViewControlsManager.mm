#import "MWMMapViewControlsManager.h"
#import "MWMAddPlaceNavigationBar.h"
#import "MWMMapDownloadDialog.h"
#import "MWMMapViewControlsManager+AddPlace.h"
#import "MWMNetworkPolicy+UI.h"
#import "MWMPlacePageManager.h"
#import "MWMPlacePageProtocol.h"
#import "MWMSearchManager.h"
#import "MWMSideButtons.h"
#import "MWMTrafficButtonViewController.h"
#import "MapViewController.h"
#import "MapsAppDelegate.h"
#import "SwiftBridge.h"

#include <CoreApi/Framework.h>
#import <CoreApi/MWMFrameworkHelper.h>

#include "platform/local_country_file_utils.hpp"
#include "platform/platform.hpp"

#include "storage/storage_helpers.hpp"

#include "map/place_page_info.hpp"

namespace {
NSString *const kMapToCategorySelectorSegue = @"MapToCategorySelectorSegue";
}  // namespace

@interface MWMMapViewControlsManager () <BottomMenuDelegate,
                                         MWMSearchManagerObserver,
                                         MWMTutorialViewControllerDelegate>

@property(nonatomic) MWMSideButtons *sideButtons;
@property(nonatomic) MWMTrafficButtonViewController *trafficButton;
@property(nonatomic) UIButton *promoButton;
@property(nonatomic) UIViewController *menuController;
@property(nonatomic) id<MWMPlacePageProtocol> placePageManager;
@property(nonatomic) MWMNavigationDashboardManager *navigationManager;
@property(nonatomic) MWMSearchManager *searchManager;

@property(weak, nonatomic) MapViewController *ownerController;

@property(nonatomic) BOOL disableStandbyOnRouteFollowing;
@property(nonatomic) MWMTip tutorialType;
@property(nonatomic) MWMTutorialViewController *tutorialViewContoller;
@property(nonatomic) PromoDiscoveryCampaign *promoDiscoveryCampaign;

@end

@implementation MWMMapViewControlsManager

+ (MWMMapViewControlsManager *)manager {
  return [MapViewController sharedController].controlsManager;
}
- (instancetype)initWithParentController:(MapViewController *)controller {
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
  self.promoDiscoveryCampaign = [ABTestManager manager].promoDiscoveryCampaign;
  if (_promoDiscoveryCampaign.enabled) {
    [controller.controlsView addSubview:self.promoButton];
    self.promoButton.translatesAutoresizingMaskIntoConstraints = NO;
    [NSLayoutConstraint activateConstraints:@[
      [self.promoButton.centerXAnchor constraintEqualToAnchor:self.trafficButton.view.centerXAnchor],
      [self.promoButton.topAnchor constraintEqualToAnchor:self.sideButtons.view.topAnchor]
    ]];
    [Statistics logEvent:kStatMapSponsoredButtonShow withParameters:@{kStatTarget: kStatGuidesSubscription}];
  }
  return self;
}

- (UIStatusBarStyle)preferredStatusBarStyle {
  BOOL const isSearchUnderStatusBar = (self.searchManager.state != MWMSearchManagerStateHidden);
  BOOL const isNavigationUnderStatusBar = self.navigationManager.state != MWMNavigationDashboardStateHidden &&
                                          self.navigationManager.state != MWMNavigationDashboardStateNavigation;
  BOOL const isMenuViewUnderStatusBar = self.menuState == MWMBottomMenuStateActive;
  BOOL const isDirectionViewUnderStatusBar = !self.isDirectionViewHidden;
  BOOL const isAddPlaceUnderStatusBar =
    [self.ownerController.view hasSubviewWithViewClass:[MWMAddPlaceNavigationBar class]];
  BOOL const isNightMode = [UIColor isNightMode];
  BOOL const isSomethingUnderStatusBar = isSearchUnderStatusBar || isNavigationUnderStatusBar ||
                                         isDirectionViewUnderStatusBar || isMenuViewUnderStatusBar ||
                                         isAddPlaceUnderStatusBar;

  return isSomethingUnderStatusBar || isNightMode ? UIStatusBarStyleLightContent : UIStatusBarStyleDefault;
}

#pragma mark - Layout

- (UIView *)anchorView {
  return self.tabBarController.view;
}

- (void)viewWillTransitionToSize:(CGSize)size
       withTransitionCoordinator:(id<UIViewControllerTransitionCoordinator>)coordinator {
  [self.trafficButton viewWillTransitionToSize:size withTransitionCoordinator:coordinator];
  [self.tabBarController viewWillTransitionToSize:size withTransitionCoordinator:coordinator];
  [self.guidesNavigationBar viewWillTransitionToSize:size withTransitionCoordinator:coordinator];
  [self.searchManager viewWillTransitionToSize:size withTransitionCoordinator:coordinator];
}

#pragma mark - MWMPlacePageViewManager

- (void)showPlacePageReview {
  [[MWMNetworkPolicy sharedPolicy] callOnlineApi:^(BOOL) {
    self.trafficButtonHidden = YES;
    [self.placePageManager showReview];
  }];
}

- (void)searchTextOnMap:(NSString *)text forInputLocale:(NSString *)locale {
  if (![self searchText:text forInputLocale:locale])
    return;

  self.searchManager.state = MWMSearchManagerStateMapSearch;
}

- (BOOL)searchText:(NSString *)text forInputLocale:(NSString *)locale {
  if (text.length == 0)
    return NO;

  self.searchManager.state = MWMSearchManagerStateTableSearch;
  [self.searchManager searchText:text forInputLocale:locale];
  return YES;
}

- (void)hideSearch {
  self.searchManager.state = MWMSearchManagerStateHidden;
}

#pragma mark - BottomMenuDelegate

- (void)actionDownloadMaps:(MWMMapDownloaderMode)mode {
  [self.ownerController openMapsDownloader:mode];
}

- (void)didFinishAddingPlace {
  self.trafficButtonHidden = NO;
  self.menuState = MWMBottomMenuStateInactive;
}

- (void)addPlace {
  GetPlatform().GetMarketingService().SendPushWooshTag(marketing::kEditorAddDiscovered);
  [self addPlace:NO hasPoint:NO point:m2::PointD()];
}

- (void)addPlace:(BOOL)isBusiness hasPoint:(BOOL)hasPoint point:(m2::PointD const &)point {
  MapViewController *ownerController = self.ownerController;
  [ownerController dismissPlacePage];

  self.searchManager.state = MWMSearchManagerStateHidden;
  self.menuState = MWMBottomMenuStateHidden;
  self.trafficButtonHidden = YES;

  [MWMAddPlaceNavigationBar showInSuperview:ownerController.view
    isBusiness:isBusiness
    applyPosition:hasPoint
    position:point
    doneBlock:^{
      auto &f = GetFramework();

      if (IsPointCoveredByDownloadedMaps(f.GetViewportCenter(), f.GetStorage(), f.GetCountryInfoGetter()))
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

- (void)setDisableStandbyOnRouteFollowing:(BOOL)disableStandbyOnRouteFollowing {
  if (_disableStandbyOnRouteFollowing == disableStandbyOnRouteFollowing)
    return;
  _disableStandbyOnRouteFollowing = disableStandbyOnRouteFollowing;
  if (disableStandbyOnRouteFollowing)
    [[MapsAppDelegate theApp] disableStandby];
  else
    [[MapsAppDelegate theApp] enableStandby];
}

#pragma mark - MWMSearchManagerObserver

- (void)onSearchManagerStateChanged {
  auto state = [MWMSearchManager manager].state;
  if (!IPAD && state == MWMSearchManagerStateHidden) {
    self.hidden = NO;
  } else if (state != MWMSearchManagerStateHidden) {
    [self hideGuidesNavigationBar];
  }
}

#pragma mark - Routing

- (void)onRoutePrepare {
  auto nm = self.navigationManager;
  [nm onRoutePrepare];
  [nm onRoutePointsUpdated];
  [self.ownerController.bookmarksCoordinator close];
  self.promoButton.hidden = YES;
}

- (void)onRouteRebuild {
  if (IPAD)
    self.searchManager.state = MWMSearchManagerStateHidden;

  [self.ownerController.bookmarksCoordinator close];
  [self.navigationManager onRoutePlanning];
  self.promoButton.hidden = YES;
}

- (void)onRouteReady:(BOOL)hasWarnings {
  self.searchManager.state = MWMSearchManagerStateHidden;
  [self.navigationManager onRouteReady:hasWarnings];
  self.promoButton.hidden = YES;
}

- (void)onRouteStart {
  self.hidden = NO;
  self.sideButtons.zoomHidden = self.zoomHidden;
  self.sideButtonsHidden = NO;
  self.disableStandbyOnRouteFollowing = YES;
  self.trafficButtonHidden = YES;
  [self.navigationManager onRouteStart];
  self.promoButton.hidden = YES;
}

- (void)onRouteStop {
  self.searchManager.state = MWMSearchManagerStateHidden;
  self.sideButtons.zoomHidden = self.zoomHidden;
  [self.navigationManager onRouteStop];
  self.disableStandbyOnRouteFollowing = NO;
  self.trafficButtonHidden = NO;
  self.promoButton.hidden = _promoDiscoveryCampaign.hasBeenActivated;
}

#pragma mark - Properties

- (UIButton *)promoButton {
  if (!_promoButton) {
    PromoCoordinator *coordinator = [[PromoCoordinator alloc] initWithViewController:self.ownerController
                                                                            campaign:_promoDiscoveryCampaign];
    _promoButton = [[PromoButton alloc] initWithCoordinator:coordinator];
  }
  return _promoButton;
}

- (MWMSideButtons *)sideButtons {
  if (!_sideButtons)
    _sideButtons = [[MWMSideButtons alloc] initWithParentView:self.ownerController.controlsView];
  return _sideButtons;
}

- (MWMTrafficButtonViewController *)trafficButton {
  if (!_trafficButton)
    _trafficButton = [[MWMTrafficButtonViewController alloc] init];
  return _trafficButton;
}

- (BottomTabBarViewController *)tabBarController {
  if (!_tabBarController) {
    _tabBarController = [BottomTabBarBuilder buildWithMapViewController:_ownerController controlsManager:self];
    [self.ownerController addChildViewController:_tabBarController];
    UIView *tabBarViewSuperView = self.ownerController.controlsView;
    [tabBarViewSuperView addSubview:_tabBarController.view];
  }

  return _tabBarController;
}

- (id<MWMPlacePageProtocol>)placePageManager {
  if (!_placePageManager)
    _placePageManager = [[MWMPlacePageManager alloc] init];
  return _placePageManager;
}

- (MWMNavigationDashboardManager *)navigationManager {
  if (!_navigationManager)
    _navigationManager = [[MWMNavigationDashboardManager alloc] initWithParentView:self.ownerController.controlsView];
  return _navigationManager;
}

- (MWMSearchManager *)searchManager {
  if (!_searchManager) {
    _searchManager = [[MWMSearchManager alloc] init];
    [MWMSearchManager addObserver:self];
  }
  return _searchManager;
}

@synthesize menuState = _menuState;

- (void)setHidden:(BOOL)hidden {
  if (_hidden == hidden)
    return;
  _hidden = hidden;
  self.sideButtonsHidden = _sideButtonsHidden;
  self.trafficButtonHidden = _trafficButtonHidden;
  self.menuState = _menuState;
}

- (void)setZoomHidden:(BOOL)zoomHidden {
  _zoomHidden = zoomHidden;
  self.sideButtons.zoomHidden = zoomHidden;
}

- (void)setSideButtonsHidden:(BOOL)sideButtonsHidden {
  _sideButtonsHidden = sideButtonsHidden;
  self.sideButtons.hidden = self.hidden || sideButtonsHidden;
}

- (void)setTrafficButtonHidden:(BOOL)trafficButtonHidden {
  BOOL const isNavigation = self.navigationManager.state == MWMNavigationDashboardStateNavigation;
  _trafficButtonHidden = isNavigation || trafficButtonHidden;
  self.trafficButton.hidden = self.hidden || _trafficButtonHidden;
}

- (void)showGuidesNavigationBar:(MWMMarkGroupID)categoryId {
  if (!_guidesNavigationBar) {
    MapViewController *parentViewController = self.ownerController;
    _guidesNavigationBar = [[GuidesNavigationBarViewController alloc] initWithCategoryId:categoryId];
    [parentViewController addChildViewController:_guidesNavigationBar];
    [parentViewController.controlsView addSubview:_guidesNavigationBar.view];
    [_guidesNavigationBar configLayout];
    _guidesNavigationBarHidden = NO;
    self.menuState = MWMBottomMenuStateHidden;
  }
}

- (void)hideGuidesNavigationBar {
  if (_guidesNavigationBar) {
    [_guidesNavigationBar removeFromParentViewController];
    [_guidesNavigationBar.view removeFromSuperview];
    _guidesNavigationBar = nil;
    _guidesNavigationBarHidden = YES;
    self.menuState = _menuRestoreState;
  }
}

- (void)setMenuState:(MWMBottomMenuState)menuState {
  _menuState = menuState;
  switch (_menuState) {
    case MWMBottomMenuStateActive:
      _tabBarController.isHidden = NO;
      if (_menuController == nil) {
        _menuController = [BottomMenuBuilder buildMenuWithMapViewController:_ownerController
                                                            controlsManager:self
                                                                   delegate:self];
        [_ownerController presentViewController:_menuController animated:YES completion:nil];
      }
      break;
    case MWMBottomMenuStateLayers:
      _tabBarController.isHidden = NO;
      if (_menuController == nil) {
        _menuController = [BottomMenuBuilder buildLayersWithMapViewController:_ownerController
                                                              controlsManager:self
                                                                     delegate:self];
        [_ownerController presentViewController:_menuController animated:YES completion:nil];
      }
      break;
    case MWMBottomMenuStateInactive:
      _tabBarController.isHidden = NO;
      if (_menuController != nil) {
        [_menuController dismissViewControllerAnimated:YES completion:nil];
        _menuController = nil;
      }
      break;
    case MWMBottomMenuStateHidden:
      _tabBarController.isHidden = YES;
      if (_menuController != nil) {
        [_menuController dismissViewControllerAnimated:YES completion:nil];
        _menuController = nil;
      }
      break;
    default:
      break;
  }
}

#pragma mark - MWMFeatureHolder

- (id<MWMFeatureHolder>)featureHolder {
  return self.placePageManager;
}

- (MWMTutorialViewController *)tutorialWithType:(MWMTip)tutorialType {
  MWMTutorialViewController *tutorial;
  switch (tutorialType) {
    case MWMTipSearch:
      tutorial = [MWMTutorialViewController tutorial:MWMTutorialTypeSearch
                                              target:self.tabBarController.searchButton
                                            delegate:self];
      break;
    case MWMTipDiscovery:
      tutorial = [MWMTutorialViewController tutorial:MWMTutorialTypeDiscovery
                                              target:self.tabBarController.discoveryButton
                                            delegate:self];
      break;
    case MWMTipBookmarks:
      tutorial = [MWMTutorialViewController tutorial:MWMTutorialTypeBookmarks
                                              target:self.tabBarController.bookmarksButton
                                            delegate:self];
      break;
    case MWMTipSubway:
      tutorial = [MWMTutorialViewController tutorial:MWMTutorialTypeSubway
                                              target:(UIControl *)self.trafficButton.view
                                            delegate:self];
      break;
    case MWMTipIsolines:
      tutorial = [MWMTutorialViewController tutorial:MWMTutorialTypeIsolines
                                              target:(UIControl *)self.trafficButton.view
                                            delegate:self];
      break;
    case MWMTipNone:
      tutorial = nil;
      break;
  }

  return tutorial;
}

- (void)showAdditionalViewsIfNeeded {
  auto ownerController = self.ownerController;

  if ([MWMRouter isRoutingActive] || [MWMRouter hasSavedRoute])
    return;

  if (self.searchManager.state != MWMSearchManagerStateHidden)
    return;

  if (self.menuState != MWMBottomMenuStateInactive)
    return;

  if (ownerController.navigationController.viewControllers.count > 1)
    return;

  if (DeepLinkHandler.shared.isLaunchedByDeeplink)
    return;

  if ([self showPromoBookingIfNeeded])
    return;

  [self showTutorialIfNeeded];
}

- (BOOL)showPromoBookingIfNeeded {
  PromoAfterBookingCampaign *afterBookingCampaign = [ABTestManager manager].promoAfterBookingCampaign;
  PromoAfterBookingData *afterBookingData = afterBookingCampaign.afterBookingData;
  if (!afterBookingData.enabled)
    return NO;

  MWMVoidBlock ok = ^{
    auto urlString = afterBookingData.promoUrl;
    auto url = [NSURL URLWithString:urlString];
    [MapViewController.sharedController openCatalogAbsoluteUrl:url animated:YES utm:MWMUTMBookingPromo];

    [self.ownerController dismissViewControllerAnimated:YES completion:nil];
  };
  MWMVoidBlock cancel = ^{
    [self.ownerController dismissViewControllerAnimated:YES completion:nil];
  };
  NSString *cityImageUrl = afterBookingData.pictureUrl;
  PromoAfterBookingViewController *alert;
  alert = [[PromoAfterBookingViewController alloc] initWithCityImageUrl:cityImageUrl okClosure:ok cancelClosure:cancel];
  [self.ownerController presentViewController:alert animated:YES completion:nil];
  [MWMEye promoAfterBookingShownWithCityId:afterBookingData.promoId];
  return YES;
}

- (BOOL)showTutorialIfNeeded {
  if (self.tutorialViewContoller != nil)
    return YES;

  auto ownerController = self.ownerController;

  if ([self.placePageManager isPPShown] || ownerController.downloadDialog.superview != nil) {
    return NO;
  }

  self.tutorialType = [MWMEye getTipType];
  self.tutorialViewContoller = [self tutorialWithType:self.tutorialType];
  if (!self.tutorialViewContoller)
    return NO;

  [self logTutorialEvent:kStatTipsTricksShow additionalOptions:nil];
  self.hidden = NO;
  [ownerController addChildViewController:self.tutorialViewContoller];
  self.tutorialViewContoller.view.frame = ownerController.view.bounds;
  self.tutorialViewContoller.view.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
  [ownerController.controlsView addSubview:self.tutorialViewContoller.view];
  [self.tutorialViewContoller didMoveToParentViewController:ownerController];

  return YES;
}

- (void)didPressCancel:(MWMTutorialViewController *)viewController {
  [self logTutorialEvent:kStatTipsTricksClose additionalOptions:@{kStatOption: kStatGotIt}];
  [MWMEye tipClickedWithType:self.tutorialType event:MWMTipEventGotIt];
  [self fadeOutTutorial:viewController];
}

- (void)didPressTarget:(MWMTutorialViewController *)viewController {
  [self logTutorialEvent:kStatTipsTricksClick additionalOptions:nil];
  [MWMEye tipClickedWithType:self.tutorialType event:MWMTipEventAction];
  [self fadeOutTutorial:viewController];
}

- (void)didPressOnScreen:(MWMTutorialViewController *)viewController {
  [self logTutorialEvent:kStatTipsTricksClose additionalOptions:@{kStatOption: kStatOffscreen}];
}

- (void)fadeOutTutorial:(MWMTutorialViewController *)viewController {
  [viewController fadeOutWithCompletion:^{
    [viewController willMoveToParentViewController:nil];
    [viewController.view removeFromSuperview];
    [viewController removeFromParentViewController];
  }];
  self.tutorialViewContoller = nil;
}

- (void)logTutorialEvent:(NSString *)eventName additionalOptions:(NSDictionary<NSString *, NSString *> *)options {
  MWMTip type = self.tutorialType;
  NSNumber *statTutorialType;
  switch (type) {
    case MWMTipSearch:
      statTutorialType = @1;
      break;
    case MWMTipDiscovery:
      statTutorialType = @2;
      break;
    case MWMTipBookmarks:
      statTutorialType = @0;
      break;
    case MWMTipSubway:
      statTutorialType = @3;
      break;
    case MWMTipIsolines:
      statTutorialType = @4;
      break;
    case MWMTipNone:
      return;
  }
  NSMutableDictionary *params = [NSMutableDictionary dictionaryWithObject:statTutorialType forKey:kStatType];
  if (options != nil) {
    [params addEntriesFromDictionary:options];
  }
  [Statistics logEvent:eventName withParameters:params];
}

@end
