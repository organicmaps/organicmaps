#import "MWMMapViewControlsManager.h"
#import "Common.h"
#import "EAGLView.h"
#import "MWMAPIBar.h"
#import "MWMAddPlaceNavigationBar.h"
#import "MWMAlertViewController.h"
#import "MWMAuthorizationCommon.h"
#import "MWMBottomMenuViewController.h"
#import "MWMButton.h"
#import "MWMFrameworkListener.h"
#import "MWMObjectsCategorySelectorController.h"
#import "MWMPlacePageEntity.h"
#import "MWMPlacePageViewManager.h"
#import "MWMRoutePreview.h"
#import "MWMRouter.h"
#import "MWMSearchManager.h"
#import "MWMSearchView.h"
#import "MWMSideButtons.h"
#import "MapViewController.h"
#import "MapsAppDelegate.h"
#import "Statistics.h"
#import "UIColor+MapsMeColor.h"

#import "3party/Alohalytics/src/alohalytics_objc.h"

#include "Framework.h"

#include "platform/local_country_file_utils.hpp"

#include "storage/storage_helpers.hpp"

namespace
{
NSString * const kMapToCategorySelectorSegue = @"MapToCategorySelectorSegue";
}  // namespace

extern NSString * const kAlohalyticsTapEventKey;

@interface MWMMapViewControlsManager ()<MWMNavigationDashboardManagerProtocol,
                                        MWMBottomMenuControllerProtocol>

@property(nonatomic) MWMSideButtons * sideButtons;
@property(nonatomic) MWMBottomMenuViewController * menuController;
@property(nonatomic) MWMPlacePageViewManager * placePageManager;
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
  self.menuState = MWMBottomMenuStateInactive;
  self.menuRestoreState = MWMBottomMenuStateInactive;
  return self;
}

- (UIStatusBarStyle)preferredStatusBarStyle
{
  MWMSearchManagerState const searchManagerState =
      _searchManager ? _searchManager.state : MWMSearchManagerStateHidden;
  BOOL const isNightMode = [UIColor isNightMode];
  BOOL const isLight = (searchManagerState == MWMSearchManagerStateMapSearch &&
                        self.navigationState != MWMNavigationDashboardStateNavigation) ||
                       (searchManagerState != MWMSearchManagerStateMapSearch &&
                        searchManagerState != MWMSearchManagerStateHidden) ||
                       self.navigationState == MWMNavigationDashboardStatePlanning ||
                       self.menuState == MWMBottomMenuStateActive || self.isDirectionViewShown ||
                       (isNightMode && self.navigationState != MWMNavigationDashboardStateHidden) ||
                       MapsAppDelegate.theApp.routingPlaneMode != MWMRoutingPlaneModeNone;
  return (isLight || (!isLight && isNightMode)) ? UIStatusBarStyleLightContent
                                                : UIStatusBarStyleDefault;
}

#pragma mark - My Position

- (void)processMyPositionStateModeEvent:(location::EMyPositionMode)mode
{
  [self.sideButtons processMyPositionStateModeEvent:mode];
}

#pragma mark - Layout

- (void)refreshLayout { [self.menuController refreshLayout]; }
- (void)mwm_refreshUI
{
  [self.sideButtons mwm_refreshUI];
  [self.navigationManager mwm_refreshUI];
  [self.searchManager mwm_refreshUI];
  [self.menuController mwm_refreshUI];
  [self.placePageManager mwm_refreshUI];
}

- (void)willRotateToInterfaceOrientation:(UIInterfaceOrientation)toInterfaceOrientation
                                duration:(NSTimeInterval)duration
{
  [self.menuController willRotateToInterfaceOrientation:toInterfaceOrientation duration:duration];
  // Workaround needs for setting correct left bound while landscape place page is open.
  self.navigationManager.leftBound = 0;
  [self.placePageManager willRotateToInterfaceOrientation:toInterfaceOrientation];
  [self.searchManager willRotateToInterfaceOrientation:toInterfaceOrientation duration:duration];
}

- (void)viewWillTransitionToSize:(CGSize)size
       withTransitionCoordinator:(id<UIViewControllerTransitionCoordinator>)coordinator
{
  [self.menuController viewWillTransitionToSize:size withTransitionCoordinator:coordinator];
  // Workaround needs for setting correct left bound while landscape place page is open.
  self.navigationManager.leftBound = 0;
  [self.placePageManager viewWillTransitionToSize:size withTransitionCoordinator:coordinator];
  [self.searchManager viewWillTransitionToSize:size withTransitionCoordinator:coordinator];
}

#pragma mark - MWMPlacePageViewManager

- (void)dismissPlacePage { [self.placePageManager hidePlacePage]; }
- (void)showPlacePage:(place_page::Info const &)info { [self.placePageManager showPlacePage:info]; }
- (MWMAlertViewController *)alertController { return self.ownerController.alertController; }
- (void)searchViewDidEnterState:(MWMSearchManagerState)state
{
  if (state == MWMSearchManagerStateMapSearch)
  {
    [self.navigationManager setMapSearch];
  }
  MWMRoutingPlaneMode const m = MapsAppDelegate.theApp.routingPlaneMode;
  if (state == MWMSearchManagerStateHidden)
  {
    if (!IPAD || m == MWMRoutingPlaneModeNone)
    {
      self.hidden = NO;
      self.leftBound = self.topBound = 0.0;
    }
  }
  [self.ownerController setNeedsStatusBarAppearanceUpdate];
  if (!IPAD || (state != MWMSearchManagerStateDefault && state != MWMSearchManagerStateHidden))
    return;
  if (m == MWMRoutingPlaneModeSearchSource || m == MWMRoutingPlaneModeSearchDestination)
  {
    [UIView animateWithDuration:kDefaultAnimationDuration
                     animations:^{
                       if (state == MWMSearchManagerStateHidden)
                       {
                         self.navigationManager.routePreview.alpha = 1.;
                         MapsAppDelegate.theApp.routingPlaneMode = MWMRoutingPlaneModePlacePage;
                       }
                       else
                       {
                         self.navigationManager.routePreview.alpha = 0.;
                       }
                     }];
  }
  else if (m == MWMRoutingPlaneModePlacePage)
  {
    if (state == MWMSearchManagerStateHidden)
    {
      [UIView animateWithDuration:kDefaultAnimationDuration
                       animations:^{
                         self.navigationManager.routePreview.alpha = 1.;
                       }];
    }
    else
    {
      [UIView animateWithDuration:kDefaultAnimationDuration
          animations:^{
            self.navigationManager.routePreview.alpha = 0.;
          }
          completion:^(BOOL finished) {
            MapsAppDelegate.theApp.routingPlaneMode = MWMRoutingPlaneModeNone;
            self.navigationManager.routePreview.alpha = 1.;
            [self.navigationManager.routePreview removeFromSuperview];
            [[MWMRouter router] stop];
            self.navigationManager.state = MWMNavigationDashboardStateHidden;
            self.menuController.p2pButton.selected = NO;
          }];
    }
  }
}

- (void)searchFrameUpdated:(CGRect)frame
{
  CGSize const s = frame.size;
  self.leftBound = s.width;
  self.topBound = s.height;
}

- (void)searchText:(NSString *)text forInputLocale:(NSString *)locale
{
  if (text.length == 0)
    return;
  self.searchManager.state = MWMSearchManagerStateTableSearch;
  [self.searchManager searchText:text forInputLocale:locale];
}

#pragma mark - MWMBottomMenuControllerProtocol

- (void)actionDownloadMaps:(mwm::DownloaderMode)mode
{
  if (platform::migrate::NeedMigrate())
  {
    if (GetFramework().IsRoutingActive())
    {
      [Statistics logEvent:kStatDownloaderMigrationProhibitedDialogue
            withParameters:@{kStatFrom : kStatDownloader}];
      [self.alertController presentMigrationProhibitedAlert];
    }
    else
    {
      [Statistics logEvent:kStatDownloaderMigrationDialogue
            withParameters:@{kStatFrom : kStatDownloader}];
      [self.ownerController openMigration];
    }
  }
  else
  {
    [self.ownerController openMapsDownloader:mode];
  }
}

- (void)closeInfoScreens
{
  if (IPAD)
  {
    if (!self.searchHidden)
      self.searchManager.state = MWMSearchManagerStateHidden;
    else if (MapsAppDelegate.theApp.routingPlaneMode != MWMRoutingPlaneModeNone)
      [[MWMRouter router] stop];
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
  self.menuState = MWMBottomMenuStateInactive;
  static_cast<EAGLView *>(self.ownerController.view).widgetsManager.fullScreen = NO;
}

- (void)addPlace:(BOOL)isBusiness hasPoint:(BOOL)hasPoint point:(m2::PointD const &)point
{
  self.menuState = MWMBottomMenuStateHidden;
  static_cast<EAGLView *>(self.ownerController.view).widgetsManager.fullScreen = YES;
  [self.placePageManager dismissPlacePage];
  self.searchManager.state = MWMSearchManagerStateHidden;

  [MWMAddPlaceNavigationBar showInSuperview:self.ownerController.view
      isBusiness:isBusiness
      applyPosition:hasPoint
      position:point
      doneBlock:^{
        auto & f = GetFramework();

        if (IsPointCoveredByDownloadedMaps(f.GetViewportCenter(), f.GetStorage(),
                                           f.GetCountryInfoGetter()))
          [self.ownerController performSegueWithIdentifier:kMapToCategorySelectorSegue sender:nil];
        else
          [self.ownerController.alertController presentIncorrectFeauturePositionAlert];

        [self didFinishAddingPlace];
      }
      cancelBlock:^{
        [self didFinishAddingPlace];
      }];
}

- (void)dragPlacePage:(CGRect)frame
{
  if (IPAD)
    return;
  CGSize const ownerViewSize = self.ownerController.view.size;
  if (ownerViewSize.width > ownerViewSize.height)
  {
    CGFloat const leftBound = frame.origin.x + frame.size.width;
    self.menuController.leftBound = leftBound;
    [MWMNavigationDashboardManager manager].leftBound = leftBound;
  }
  else
  {
    [self.sideButtons setBottomBound:frame.origin.y];
  }
}

- (void)addPlacePageViews:(NSArray *)views
{
  UIView * ownerView = self.ownerController.view;
  for (UIView * view in views)
    [ownerView addSubview:view];
  [ownerView bringSubviewToFront:self.searchManager.view];
  if (IPAD)
  {
    [ownerView bringSubviewToFront:self.menuController.view];
    [ownerView bringSubviewToFront:self.navigationManager.routePreview];
  }
}

#pragma mark - MWMNavigationDashboardManager

- (void)routePreviewDidChangeFrame:(CGRect)newFrame
{
  if (!IPAD)
  {
    CGFloat const bound = newFrame.origin.y + newFrame.size.height;
    self.placePageManager.topBound = self.sideButtons.topBound = bound;
    return;
  }

  CGFloat const bound = newFrame.origin.x + newFrame.size.width;
  if (self.searchManager.state == MWMSearchManagerStateHidden)
  {
    self.placePageManager.leftBound = self.menuController.leftBound =
        newFrame.origin.x + newFrame.size.width;
  }
  else
  {
    [UIView animateWithDuration:kDefaultAnimationDuration
        animations:^{
          self.searchManager.view.alpha = bound > 0 ? 0. : 1.;
        }
        completion:^(BOOL finished) {
          self.searchManager.state = MWMSearchManagerStateHidden;
        }];
  }
}

- (void)navigationDashBoardDidUpdate
{
  if (IPAD)
  {
    [self.placePageManager setTopBound:self.topBound + self.navigationManager.leftHeight];
  }
  else
  {
    CGFloat const topBound = self.topBound + self.navigationManager.rightHeight;
    [self.sideButtons setTopBound:topBound];
    BOOL const skipNavDashboardHeight =
        self.navigationManager.state == MWMNavigationDashboardStateNavigation;
    [self.placePageManager setTopBound:skipNavDashboardHeight ? self.topBound : topBound];
  }
}

- (void)didStartEditingRoutePoint:(BOOL)isSource
{
  [Statistics logEvent:kStatEventName(kStatPointToPoint, kStatSearch)
        withParameters:@{kStatValue : (isSource ? kStatSource : kStatDestination)}];
  MapsAppDelegate.theApp.routingPlaneMode =
      isSource ? MWMRoutingPlaneModeSearchSource : MWMRoutingPlaneModeSearchDestination;
  self.searchManager.state = MWMSearchManagerStateDefault;
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

#pragma mark - Routing

- (void)onRoutePrepare
{
  self.navigationManager.state = MWMNavigationDashboardStatePrepare;
  MapsAppDelegate.theApp.routingPlaneMode = MWMRoutingPlaneModePlacePage;
  self.menuController.p2pButton.selected = YES;
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
  MapsAppDelegate.theApp.routingPlaneMode = MWMRoutingPlaneModePlacePage;
}

- (void)onRouteReady
{
  if (![MWMRouter router].startPoint.IsMyPosition())
  {
    dispatch_async(dispatch_get_main_queue(), ^{
      GetFramework().DisableFollowMode();
      [self.navigationManager updateDashboard];
    });
  }
  self.navigationManager.state = MWMNavigationDashboardStateReady;
}

- (void)onRouteStart
{
  self.hidden = NO;
  self.sideButtons.zoomHidden = self.zoomHidden;
  self.sideButtonsHidden = NO;
  self.disableStandbyOnRouteFollowing = YES;
  self.navigationManager.state = MWMNavigationDashboardStateNavigation;
}

- (void)onRouteStop
{
  self.sideButtons.zoomHidden = self.zoomHidden;
  self.navigationManager.state = MWMNavigationDashboardStateHidden;
  self.disableStandbyOnRouteFollowing = NO;
  self.menuState = MWMBottomMenuStateInactive;
  [self navigationDashBoardDidUpdate];
}

#pragma mark - Properties

- (MWMSideButtons *)sideButtons
{
  if (!_sideButtons)
    _sideButtons = [[MWMSideButtons alloc] initWithParentView:self.ownerController.view];
  return _sideButtons;
}

- (MWMBottomMenuViewController *)menuController
{
  if (!_menuController)
    _menuController =
        [[MWMBottomMenuViewController alloc] initWithParentController:self.ownerController
                                                             delegate:self];
  return _menuController;
}

- (MWMPlacePageViewManager *)placePageManager
{
  if (!_placePageManager)
    _placePageManager =
        [[MWMPlacePageViewManager alloc] initWithViewController:self.ownerController];
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
    _searchManager = [[MWMSearchManager alloc] initWithParentView:self.ownerController.view];
  return _searchManager;
}

@synthesize menuState = _menuState;

- (void)setHidden:(BOOL)hidden
{
  if (_hidden == hidden)
    return;
  _hidden = hidden;
  self.sideButtonsHidden = _sideButtonsHidden;
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

- (MWMNavigationDashboardState)navigationState { return self.navigationManager.state; }
- (MWMPlacePageEntity *)placePageEntity { return self.placePageManager.entity; }
- (BOOL)isDirectionViewShown
{
  return _placePageManager ? _placePageManager.isDirectionViewShown : NO;
}

- (void)setTopBound:(CGFloat)topBound
{
  if (IPAD)
    return;
  _topBound = self.placePageManager.topBound = self.sideButtons.topBound =
      self.navigationManager.topBound = topBound;
}

- (void)setLeftBound:(CGFloat)leftBound
{
  if (!IPAD)
    return;
  MWMRoutingPlaneMode const m = MapsAppDelegate.theApp.routingPlaneMode;
  if (m != MWMRoutingPlaneModeNone)
    return;
  _leftBound = self.placePageManager.leftBound = self.navigationManager.leftBound =
      self.menuController.leftBound = leftBound;
}

- (BOOL)searchHidden { return self.searchManager.state == MWMSearchManagerStateHidden; }
- (void)setSearchHidden:(BOOL)searchHidden
{
  self.searchManager.state =
      searchHidden ? MWMSearchManagerStateHidden : MWMSearchManagerStateDefault;
}

@end
