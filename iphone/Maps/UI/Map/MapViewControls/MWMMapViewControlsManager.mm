#import "MWMMapViewControlsManager.h"
#import "MWMMapDownloadDialog.h"
#import "MWMMapViewControlsManager+AddPlace.h"
#import "MWMMapWidgetsHelper.h"
#import "MWMNetworkPolicy+UI.h"
#import "MWMPlacePageManager.h"
#import "MWMPlacePageProtocol.h"
#import "MWMSideButtons.h"
#import "MWMTrafficButtonViewController.h"
#import "MapViewController.h"
#import "MapsAppDelegate.h"
#import "SwiftBridge.h"

#import <CoreApi/MWMFrameworkHelper.h>

#include "platform/local_country_file_utils.hpp"
#include "platform/platform.hpp"

#include "base/assert.hpp"

#include "storage/storage_helpers.hpp"

#include "map/place_page_info.hpp"

namespace
{
NSString * const kMapToCategorySelectorSegue = @"MapToCategorySelectorSegue";
}  // namespace

@interface MWMMapViewControlsManager () <BottomMenuDelegate>

@property(nonatomic) MWMSideButtons * sideButtons;
@property(nonatomic) MWMTrafficButtonViewController * trafficButton;
@property(nonatomic) UIButton * promoButton;
@property(nonatomic) UIViewController * menuController;
@property(nonatomic) BOOL isDismissingMenu;
@property(nonatomic) NSURL * pendingMenuURL;
@property(nonatomic) id<MWMPlacePageProtocol> placePageManager;
@property(nonatomic) MWMNavigationDashboardManager * navigationManager;
@property(nonatomic) SearchOnMapManager * searchManager;

@property(weak, nonatomic) MapViewController * ownerController;

@property(nonatomic) BOOL disableStandbyOnRouteFollowing;
@property(nonatomic) BOOL isAddingPlace;

@end

@implementation MWMMapViewControlsManager

+ (MWMMapViewControlsManager *)manager
{
  return [MapViewController sharedController].controlsManager;
}

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
  self.isAddingPlace = NO;
  self.searchManager = controller.searchManager;
  return self;
}

- (UIStatusBarStyle)preferredStatusBarStyle
{
  MapViewController * ownerController = self.ownerController;
  BOOL const isMenuViewUnderStatusBar = self.menuState == MWMBottomMenuStateActive;
  BOOL const isDirectionViewUnderStatusBar = !self.isDirectionViewHidden;
  BOOL const isMapPointPickerUnderStatusBar =
      [ownerController.childViewControllers
          indexOfObjectPassingTest:^BOOL(UIViewController * controller, NSUInteger, BOOL *) {
            return [controller isKindOfClass:[MapPointPickerViewController class]];
          }] != NSNotFound;
  BOOL const isNightMode = ownerController.traitCollection.userInterfaceStyle == UIUserInterfaceStyleDark;
  BOOL const isSomethingUnderStatusBar =
      isDirectionViewUnderStatusBar || isMenuViewUnderStatusBar || isMapPointPickerUnderStatusBar;

  return isSomethingUnderStatusBar || isNightMode ? UIStatusBarStyleLightContent : UIStatusBarStyleDefault;
}

#pragma mark - Layout

- (UIView *)anchorView
{
  return self.tabBarController.view;
}

- (void)viewWillTransitionToSize:(CGSize)size
       withTransitionCoordinator:(id<UIViewControllerTransitionCoordinator>)coordinator
{
  [self.trafficButton viewWillTransitionToSize:size withTransitionCoordinator:coordinator];
  [self.trackRecordingButton viewWillTransitionToSize:size withTransitionCoordinator:coordinator];
  [self.tabBarController viewWillTransitionToSize:size withTransitionCoordinator:coordinator];
}

#pragma mark - MWMPlacePageViewManager

- (void)searchOnMap:(SearchQuery *)query
{
  if (![self search:query])
    return;

  [self.searchManager startSearchingWithIsRouting:NO];
}

- (BOOL)search:(SearchQuery *)query
{
  if (query.text.length == 0)
    return NO;

  [self.searchManager startSearchingWithIsRouting:NO];
  [self.searchManager searchText:query];
  return YES;
}

#pragma mark - BottomMenu
- (void)actionDownloadMaps:(MWMMapDownloaderMode)mode
{
  [self.ownerController openMapsDownloader:mode];
}

- (void)didFinishAddingPlace
{
  self.isAddingPlace = NO;
  self.trafficButtonHidden = NO;
  self.menuState = self.closedMenuState;
}

- (void)addPlace
{
  [self addPlace:NO position:nullptr];
}

- (void)addPlace:(BOOL)isBusiness position:(m2::PointD const *)optionalPosition
{
  MapViewController * ownerController = self.ownerController;

  self.isAddingPlace = YES;
  [self.searchManager close];
  self.menuState = MWMBottomMenuStateHidden;
  self.trafficButtonHidden = YES;

  [ownerController dismissPlacePage];

  NSValue * initialMercatorPosition =
      optionalPosition ? [NSValue valueWithCGPoint:CGPointMake(optionalPosition->x, optionalPosition->y)] : nil;
  MapPointPickerViewController * picker =
      [[MapPointPickerViewController alloc] initWithTitle:L(@"editor_add_select_location")
                                                     hint:L(@"editor_focus_map_on_location")
                                             enableBounds:isBusiness
                                  initialMercatorPosition:initialMercatorPosition
                                     shouldChangeViewport:YES];
  picker.doneHandler = ^(CGPoint point) {
    if ([MWMFrameworkHelper canEditMapAtMercatorPoint:point])
    {
      // Keep using the picked position: by the time the user picks a category the viewport may drift
      // (location follow, layout changes) onto a different — possibly unloaded — MWM.
      m2::PointD const position(point.x, point.y);
      NSValue * sender = [NSValue valueWithBytes:&position objCType:@encode(m2::PointD)];
      [ownerController performSegueWithIdentifier:kMapToCategorySelectorSegue sender:sender];
    }
    else
      [ownerController.alertController presentIncorrectFeaturePositionAlertWithOkBlock:nil];

    [self didFinishAddingPlace];
  };
  picker.cancelHandler = ^{ [self didFinishAddingPlace]; };
  [picker presentIn:ownerController];
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

#pragma mark - Routing

- (void)onRoutePrepare
{
  auto nm = self.navigationManager;
  [nm onRoutePrepare];
  [nm onRoutePointsUpdated];
  [self.ownerController.bookmarksCoordinator close];
  self.promoButton.hidden = YES;
}

- (void)onRouteRebuild
{
  [self.ownerController.bookmarksCoordinator close];
  [self.navigationManager onRoutePlanning];
  self.promoButton.hidden = YES;
}

- (void)onRouteReady:(BOOL)hasWarnings
{
  [self.navigationManager onRouteReady:hasWarnings];
  self.promoButton.hidden = YES;
}

- (void)onRouteStart
{
  self.hidden = NO;
  self.sideButtons.zoomHidden = self.zoomHidden;
  self.sideButtonsHidden = NO;
  self.disableStandbyOnRouteFollowing = YES;
  self.trafficButtonHidden = YES;
  if (TrackRecordingManager.shared.isActive)
    [self setTrackRecordingButtonState:TrackRecordingButtonStateHidden];
  [self.navigationManager onRouteStart];
  self.promoButton.hidden = YES;
}

- (void)onRouteStop
{
  self.sideButtons.zoomHidden = self.zoomHidden;
  [self.navigationManager onRouteStop];
  self.disableStandbyOnRouteFollowing = NO;
  self.trafficButtonHidden = NO;
  // Must run after `onRouteStop` has flipped the dashboard state from Navigation to Closed,
  // otherwise `setTrackRecordingButtonState:` would keep the widget hidden.
  if (TrackRecordingManager.shared.isActive &&
      ![TrackRecordingManager.shared contains:MapViewController.sharedController])
    [self setTrackRecordingButtonState:TrackRecordingButtonStateVisible];
  self.promoButton.hidden = YES;
}

#pragma mark - Properties

- (MWMSideButtons *)sideButtons
{
  if (!_sideButtons)
    _sideButtons = [[MWMSideButtons alloc] initWithParentView:self.ownerController.controlsView];
  return _sideButtons;
}

- (MWMTrafficButtonViewController *)trafficButton
{
  if (!_trafficButton)
    _trafficButton = [[MWMTrafficButtonViewController alloc] init];
  return _trafficButton;
}

- (BottomTabBarViewController *)tabBarController
{
  if (!_tabBarController)
  {
    MapViewController * ownerController = _ownerController;
    _tabBarController = [BottomTabBarBuilder buildWithMapViewController:ownerController controlsManager:self];
    [ownerController addChildViewController:_tabBarController];
    UIView * tabBarViewSuperView = ownerController.controlsView;
    [tabBarViewSuperView addSubview:_tabBarController.view];
  }

  return _tabBarController;
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
    _navigationManager = [[MWMNavigationDashboardManager alloc] initWithParentViewController:self.ownerController];
  return _navigationManager;
}

@synthesize menuState = _menuState;

- (void)setHidden:(BOOL)hidden
{
  if (_hidden == hidden)
    return;
  // Do not hide the controls view during the place adding process.
  if (!_isAddingPlace)
    _hidden = hidden;
  self.sideButtonsHidden = _sideButtonsHidden;
  self.trafficButtonHidden = _trafficButtonHidden;
  self.menuState = self.closedMenuState;
}

- (MWMBottomMenuState)closedMenuState
{
  return self.hidden || self.isAddingPlace ? MWMBottomMenuStateHidden : MWMBottomMenuStateInactive;
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

- (void)setTrackRecordingButtonState:(TrackRecordingButtonState)state
{
  BOOL const isNavigation = self.navigationManager.state == MWMNavigationDashboardStateNavigation;
  if (state == TrackRecordingButtonStateVisible && isNavigation)
    state = TrackRecordingButtonStateHidden;
  if ((state == TrackRecordingButtonStateHidden || state == TrackRecordingButtonStateClosed) && !_trackRecordingButton)
    return;
  if (!_trackRecordingButton)
    _trackRecordingButton = [[TrackRecordingButtonViewController alloc] init];
  [self.trackRecordingButton setState:state completion:^{ [MWMMapWidgetsHelper updateLayoutForAvailableArea]; }];
  if (state == TrackRecordingButtonStateClosed)
    _trackRecordingButton = nil;
}

- (void)setMenuState:(MWMBottomMenuState)menuState
{
  [self setMenuState:menuState animated:YES];
}

- (void)setMenuState:(MWMBottomMenuState)menuState animated:(BOOL)animated
{
  MapViewController * ownerController = _ownerController;
  __weak MapViewController * weakOwnerController = ownerController;
  if (menuState == MWMBottomMenuStateActive || menuState == MWMBottomMenuStateLayers)
  {
    if (_isDismissingMenu)
      return;
    if (_menuState == MWMBottomMenuStateLayers && menuState == MWMBottomMenuStateActive)
    {
      [self replaceLayersWithMenuForURL:nil];
      return;
    }
    // An unrelated modal (such as Share) owns the presenter. UIKit would reject another presentation.
    if (_menuController == nil && (ownerController.presentedViewController != nil ||
                                   ownerController.viewIfLoaded.window == nil || ownerController.controlsView.isHidden))
    {
      if (_menuState == MWMBottomMenuStateActive)
        [self setMenuState:self.closedMenuState animated:NO];
      return;
    }
  }

  _menuState = menuState;
  switch (_menuState)
  {
  case MWMBottomMenuStateActive:
    _tabBarController.isHidden = NO;
    if (_menuController == nil)
    {
      _menuController = [BottomMenuBuilder buildMenuWithMapViewController:ownerController
                                                          controlsManager:self
                                                                 delegate:self];
      [ownerController presentViewController:_menuController animated:YES completion:nil];
    }
    break;
  case MWMBottomMenuStateLayers:
    _tabBarController.isHidden = NO;
    if (_menuController == nil)
    {
      _menuController = [BottomMenuBuilder buildLayersWithMapViewController:ownerController
                                                            controlsManager:self
                                                                   delegate:self];
      [ownerController presentViewController:_menuController animated:YES completion:nil];
    }
    break;
  case MWMBottomMenuStateInactive:
  case MWMBottomMenuStateHidden:
  {
    _tabBarController.isHidden = menuState == MWMBottomMenuStateHidden;
    if (_pendingMenuURL != nil && [DeepLinkHandler.shared.url isEqual:_pendingMenuURL])
      (void)[DeepLinkHandler.shared getInAppFeatureHighlightData];
    _pendingMenuURL = nil;
    [self dismissBottomMenuAnimated:animated
                         completion:^{ [weakOwnerController refreshMapAccessibilityAfterMenuDismissal]; }];
    break;
  }
  default: break;
  }
}

- (void)replaceLayersWithMenuForURL:(NSURL *)url
{
  _menuState = MWMBottomMenuStateActive;
  _pendingMenuURL = url;
  [self dismissBottomMenuAnimated:NO
                       completion:^{
                         NSURL * pendingURL = self->_pendingMenuURL;
                         self->_pendingMenuURL = nil;
                         if (self->_menuState != MWMBottomMenuStateActive)
                           return;
                         if (pendingURL != nil && ![DeepLinkHandler.shared.url isEqual:pendingURL])
                         {
                           [self setMenuState:self.closedMenuState animated:NO];
                           return;
                         }
                         self.menuState = MWMBottomMenuStateActive;
                         if (pendingURL != nil && self->_menuState != MWMBottomMenuStateActive)
                           (void)[DeepLinkHandler.shared getInAppFeatureHighlightData];
                       }];
}

- (void)dismissBottomMenuAnimated:(BOOL)animated completion:(void (^)(void))completion
{
  if (_isDismissingMenu)
    return;

  UIViewController * menuController = _menuController;
  UIViewController * presenter = menuController.presentingViewController;
  if (presenter == nil)
  {
    _menuController = nil;
    if (completion)
      completion();
    return;
  }

  // Keep the outgoing sheet identifiable until UIKit finishes, so other actions can cancel a replacement.
  _isDismissingMenu = YES;
  [presenter dismissViewControllerAnimated:animated
                                completion:^{
                                  ASSERT(self->_menuController == menuController, ("Menu changed during dismissal"));
                                  self->_menuController = nil;
                                  self->_isDismissingMenu = NO;
                                  if (completion)
                                    completion();
                                }];
}

- (BOOL)dismissBottomMenuIfPresentedAnimated:(BOOL)animated
{
  if (!_isDismissingMenu && (_menuController == nil || _menuController != _ownerController.presentedViewController))
    return NO;

  [self setMenuState:self.closedMenuState animated:animated];
  return YES;
}

- (void)openMenuFromDeepLink
{
  switch (_menuState)
  {
  case MWMBottomMenuStateLayers: [self replaceLayersWithMenuForURL:DeepLinkHandler.shared.url]; break;
  case MWMBottomMenuStateInactive:
    self.menuState = MWMBottomMenuStateActive;
    if (_menuState != MWMBottomMenuStateActive)
      (void)[DeepLinkHandler.shared getInAppFeatureHighlightData];
    break;
  case MWMBottomMenuStateActive:
    if (_isDismissingMenu)
      _pendingMenuURL = DeepLinkHandler.shared.url;
    else if (_menuController != nil && _menuController == _ownerController.presentedViewController)
      [(BottomMenuViewController *)_menuController highlightRequestedItem];
    else
      (void)[DeepLinkHandler.shared getInAppFeatureHighlightData];
    break;
  case MWMBottomMenuStateHidden:
    // The controls are hidden in the full screen mode or while adding a place. Keep them hidden and drop the
    // highlight so the next menu opening skips it.
    (void)[DeepLinkHandler.shared getInAppFeatureHighlightData];
    break;
  }
}

#pragma mark - MWMFeatureHolder

- (id<MWMFeatureHolder>)featureHolder
{
  return self.placePageManager;
}

@end
