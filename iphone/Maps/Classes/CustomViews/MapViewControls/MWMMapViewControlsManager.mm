#import "Common.h"
#import "EAGLView.h"
#import "MapsAppDelegate.h"
#import "MapViewController.h"
#import "MWMAddPlaceNavigationBar.h"
#import "MWMAlertViewController.h"
#import "MWMAPIBar.h"
#import "MWMAuthorizationCommon.h"
#import "MWMBottomMenuViewController.h"
#import "MWMButton.h"
#import "MWMFrameworkListener.h"
#import "MWMFrameworkObservers.h"
#import "MWMLocationHelpers.h"
#import "MWMMapViewControlsManager.h"
#import "MWMObjectsCategorySelectorController.h"
#import "MWMPlacePageEntity.h"
#import "MWMPlacePageViewManager.h"
#import "MWMPlacePageViewManagerDelegate.h"
#import "MWMRoutePreview.h"
#import "MWMSearchManager.h"
#import "MWMSearchView.h"
#import "MWMSideButtons.h"
#import "RouteState.h"
#import "Statistics.h"

#import "3party/Alohalytics/src/alohalytics_objc.h"

#include "Framework.h"

#include "platform/local_country_file_utils.hpp"

#include "storage/storage_helpers.hpp"

namespace
{
  NSString * const kMapToCategorySelectorSegue = @"MapToCategorySelectorSegue";
} // namespace

extern NSString * const kAlohalyticsTapEventKey;

@interface MWMMapViewControlsManager ()<
    MWMPlacePageViewManagerProtocol, MWMNavigationDashboardManagerProtocol,
    MWMSearchManagerProtocol, MWMSearchViewProtocol, MWMBottomMenuControllerProtocol,
    MWMRoutePreviewDataSource, MWMFrameworkRouteBuilderObserver>

@property (nonatomic) MWMSideButtons * sideButtons;
@property (nonatomic) MWMBottomMenuViewController * menuController;
@property (nonatomic) MWMPlacePageViewManager * placePageManager;
@property (nonatomic) MWMNavigationDashboardManager * navigationManager;
@property (nonatomic) MWMSearchManager * searchManager;

@property (weak, nonatomic) MapViewController * ownerController;

@property (nonatomic) BOOL disableStandbyOnRouteFollowing;
@property (nonatomic) MWMRoutePoint routeSource;
@property (nonatomic) MWMRoutePoint routeDestination;

@property (nonatomic) CGFloat topBound;
@property (nonatomic) CGFloat leftBound;

@end

@implementation MWMMapViewControlsManager

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
  [self configRoutePoints];
  [MWMFrameworkListener addObserver:self];
  return self;
}

- (void)configRoutePoints
{
  if ([Alohalytics isFirstSession])
  {
    self.routeSource = MWMRoutePoint::MWMRoutePointZero();
  }
  else
  {
    CLLocation * lastLocation = [MWMLocationManager lastLocation];
    self.routeSource =
        lastLocation ? MWMRoutePoint(lastLocation.mercator) : MWMRoutePoint::MWMRoutePointZero();
  }
  self.routeDestination = MWMRoutePoint::MWMRoutePointZero();
}

#pragma mark - My Position

- (void)processMyPositionStateModeEvent:(location::EMyPositionMode)mode
{
  [self.sideButtons processMyPositionStateModeEvent:mode];
}

#pragma mark - MWMFrameworkRouteBuilderObserver

- (void)processRouteBuilderEvent:(routing::IRouter::ResultCode)code
                       countries:(storage::TCountriesVec const &)absentCountries
{
  switch (code)
  {
    case routing::IRouter::ResultCode::NoError:
      [self.navigationManager setRouteBuilderProgress:100];
      self.searchHidden = YES;
      break;
    case routing::IRouter::Cancelled:
    case routing::IRouter::NeedMoreMaps:
      break;
    default:
      [self handleRoutingError];
      break;
  }
}

- (void)processRouteBuilderProgress:(CGFloat)progress
{
  [self.navigationManager setRouteBuilderProgress:progress];
}

#pragma mark - Layout

- (void)refreshLayout
{
  [self.menuController refreshLayout];
}

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
  [self.placePageManager willRotateToInterfaceOrientation:toInterfaceOrientation];
  [self.navigationManager willRotateToInterfaceOrientation:toInterfaceOrientation];
  [self.searchManager willRotateToInterfaceOrientation:toInterfaceOrientation duration:duration];
  [self refreshHelperPanels:UIInterfaceOrientationIsLandscape(toInterfaceOrientation)];
}

- (void)viewWillTransitionToSize:(CGSize)size
       withTransitionCoordinator:(id<UIViewControllerTransitionCoordinator>)coordinator
{
  [self.menuController viewWillTransitionToSize:size withTransitionCoordinator:coordinator];
  [self.placePageManager viewWillTransitionToSize:size withTransitionCoordinator:coordinator];
  [self.navigationManager viewWillTransitionToSize:size withTransitionCoordinator:coordinator];
  [self.searchManager viewWillTransitionToSize:size withTransitionCoordinator:coordinator];
  [self refreshHelperPanels:size.height < size.width];
}

- (void)refreshHelperPanels:(BOOL)isLandscape
{
  if (!self.placePageManager.hasPlacePage)
    return;
  if (isLandscape)
    [self.navigationManager hideHelperPanels];
  else
    [self.navigationManager showHelperPanels];
}

#pragma mark - MWMPlacePageViewManager

- (void)dismissPlacePage
{
  [self.placePageManager hidePlacePage];
}

- (void)showPlacePage:(place_page::Info const &)info
{
  [self.placePageManager showPlacePage:info];
  [self refreshHelperPanels:UIInterfaceOrientationIsLandscape(self.ownerController.interfaceOrientation)];
}

- (void)apiBack
{
  [self.ownerController.apiBar back];
}

#pragma mark - MWMSearchManagerProtocol

- (MWMAlertViewController *)alertController
{
  return self.ownerController.alertController;
}

- (void)searchViewDidEnterState:(MWMSearchManagerState)state
{
  if (state == MWMSearchManagerStateHidden)
  {
    if (!IPAD || MapsAppDelegate.theApp.routingPlaneMode == MWMRoutingPlaneModeNone)
    {
      self.hidden = NO;
      self.leftBound = self.topBound = 0.0;
    }
  }
  [self.ownerController setNeedsStatusBarAppearanceUpdate];
  if (!IPAD || (state != MWMSearchManagerStateDefault && state != MWMSearchManagerStateHidden))
    return;
  MWMRoutingPlaneMode const m = MapsAppDelegate.theApp.routingPlaneMode;
  if (m == MWMRoutingPlaneModeSearchSource || m == MWMRoutingPlaneModeSearchDestination)
  {
    [UIView animateWithDuration:kDefaultAnimationDuration animations:^
    {
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
      return;
    [UIView animateWithDuration:kDefaultAnimationDuration animations:^
    {
      self.navigationManager.routePreview.alpha = 0.;
    }
    completion:^(BOOL finished)
    {
      MapsAppDelegate.theApp.routingPlaneMode = MWMRoutingPlaneModeNone;
      self.navigationManager.routePreview.alpha = 1.;
      [self.navigationManager.routePreview removeFromSuperview];
      [self didCancelRouting];
      self.navigationManager.state = MWMNavigationDashboardStateHidden;
      self.menuController.p2pButton.selected = NO;
    }];
  }
}

#pragma mark - MWMSearchViewProtocol

- (void)searchFrameUpdated:(CGRect)frame
{
  CGSize const s = frame.size;
  self.leftBound = s.width;
  self.topBound = s.height;
}

#pragma mark - MWMSearchManagerProtocol & MWMBottomMenuControllerProtocol

- (void)actionDownloadMaps:(mwm::DownloaderMode)mode
{
  if (platform::migrate::NeedMigrate())
  {
    if (GetFramework().IsRoutingActive())
    {
      [Statistics logEvent:kStatDownloaderMigrationProhibitedDialogue withParameters:@{kStatFrom : kStatDownloader}];
      [self.alertController presentMigrationProhibitedAlert];
    }
    else
    {
      [Statistics logEvent:kStatDownloaderMigrationDialogue withParameters:@{kStatFrom : kStatDownloader}];
      [self.ownerController openMigration];
    }
  }
  else
  {
    [self.ownerController openMapsDownloader:mode];
  }
}

#pragma mark - MWMBottomMenuControllerProtocol

- (void)closeInfoScreens
{
  if (IPAD)
  {
    if (!self.searchHidden)
      self.searchManager.state = MWMSearchManagerStateHidden;
    else if (MapsAppDelegate.theApp.routingPlaneMode != MWMRoutingPlaneModeNone)
     [self routingHidden];
  }
  else
  {
    CGSize const ownerViewSize = self.ownerController.view.size;
    if (ownerViewSize.width > ownerViewSize.height)
      [self dismissPlacePage];
  }
}

#pragma mark - MWMBottomMenuControllerProtocol

- (void)didFinishAddingPlace
{
  self.menuState = MWMBottomMenuStateInactive;
  static_cast<EAGLView *>(self.ownerController.view).widgetsManager.fullScreen = NO;
}

#pragma mark - MWMBottomMenuControllerProtocol && MWMPlacePageViewManagerProtocol

- (void)addPlace:(BOOL)isBusiness hasPoint:(BOOL)hasPoint point:(m2::PointD const &)point
{
  self.menuState = MWMBottomMenuStateHidden;
  static_cast<EAGLView *>(self.ownerController.view).widgetsManager.fullScreen = YES;
  [self.placePageManager dismissPlacePage];
  self.searchManager.state = MWMSearchManagerStateHidden;

  [MWMAddPlaceNavigationBar showInSuperview:self.ownerController.view
                                 isBusiness:isBusiness applyPosition:hasPoint position:point doneBlock:^
  {
    auto & f = GetFramework();

    if (IsPointCoveredByDownloadedMaps(f.GetViewportCenter(), f.Storage(), f.CountryInfoGetter()))
      [self.ownerController performSegueWithIdentifier:kMapToCategorySelectorSegue sender:nil];
    else
      [self.ownerController.alertController presentIncorrectFeauturePositionAlert];

    [self didFinishAddingPlace];
  }
  cancelBlock:^
  {
    [self didFinishAddingPlace];
  }];
}

#pragma mark - MWMPlacePageViewManagerDelegate

- (void)dragPlacePage:(CGRect)frame
{
  if (IPAD)
    return;
  CGSize const ownerViewSize = self.ownerController.view.size;
  if (ownerViewSize.width > ownerViewSize.height)
    self.menuController.leftBound = frame.origin.x + frame.size.width;
  else
    [self.sideButtons setBottomBound:frame.origin.y];
}

- (void)placePageDidClose
{
  if (UIInterfaceOrientationIsLandscape(self.ownerController.interfaceOrientation))
    [self.navigationManager showHelperPanels];
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

- (void)updateStatusBarStyle
{
  [self.ownerController updateStatusBarStyle];
}

- (void)setupBestRouter
{
  auto & f = GetFramework();
  if (self.isPossibleToBuildRoute)
    f.SetRouter(f.GetBestRouter(self.routeSource.Point(), self.routeDestination.Point()));
}

- (void)restoreRouteTo:(m2::PointD const &)to
{
  CLLocation * lastLocation = [MWMLocationManager lastLocation];
  if (!lastLocation)
    return;
  auto & f = GetFramework();
  m2::PointD const myPosition = lastLocation.mercator;
  f.SetRouter(f.GetBestRouter(myPosition, to));
  self.routeSource = MWMRoutePoint(myPosition);
  self.routeDestination = {to, @"Destination"};
  f.BuildRoute(myPosition, to, 0 /* timeoutSec */);
}

- (void)buildRouteFrom:(MWMRoutePoint const &)from to:(MWMRoutePoint const &)to
{
  self.menuController.p2pButton.selected = YES;
  self.navigationManager.routePreview.extendButton.selected = NO;
  MapsAppDelegate.theApp.routingPlaneMode = MWMRoutingPlaneModePlacePage;
  if (from == MWMRoutePoint::MWMRoutePointZero())
  {
    self.navigationManager.state = MWMNavigationDashboardStatePrepare;
    [self buildRouteTo:to];
    return;
  }
  self.routeSource = from;
  self.routeDestination = to;
  [self setupBestRouter];
  [self buildRoute];

  auto & f = GetFramework();
  f.SetRouteStartPoint(from.Point(), !from.IsMyPosition());
  f.SetRouteFinishPoint(to.Point(), to != MWMRoutePoint::MWMRoutePointZero());
}

- (void)buildRouteFrom:(MWMRoutePoint const &)from
{
  if (self.navigationManager.state != MWMNavigationDashboardStatePrepare)
  {
    MapsAppDelegate.theApp.routingPlaneMode = MWMRoutingPlaneModePlacePage;
    self.navigationManager.state = MWMNavigationDashboardStatePrepare;
  }

  self.routeSource = from;
  if ((from.IsMyPosition() && self.routeDestination.IsMyPosition()) || from == self.routeDestination)
  {
    GetFramework().CloseRouting();
    self.routeDestination = MWMRoutePoint::MWMRoutePointZero();
  }

  if (IPAD)
    self.searchManager.state = MWMSearchManagerStateHidden;
  [self buildRoute];
  
  GetFramework().SetRouteStartPoint(from.Point(), from != MWMRoutePoint::MWMRoutePointZero() && !from.IsMyPosition());
}

- (void)buildRouteTo:(MWMRoutePoint const &)to
{
  self.routeDestination = to;
  if ((to.IsMyPosition() && self.routeSource.IsMyPosition()) || to == self.routeSource)
  {
    GetFramework().CloseRouting();
    self.routeSource = MWMRoutePoint::MWMRoutePointZero();
  }

  if (IPAD)
    self.searchManager.state = MWMSearchManagerStateHidden;
  [self buildRoute];
  
  GetFramework().SetRouteFinishPoint(to.Point(), to != MWMRoutePoint::MWMRoutePointZero());
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
    self.placePageManager.leftBound = self.menuController.leftBound = newFrame.origin.x + newFrame.size.width;
  }
  else
  {
    [UIView animateWithDuration:kDefaultAnimationDuration animations:^
    {
      self.searchManager.view.alpha = bound > 0 ? 0. : 1.;
    }
    completion:^(BOOL finished)
    {
      self.searchManager.state = MWMSearchManagerStateHidden;
    }];
  }
}

- (void)updateFollowingInfo:(location::FollowingInfo const &)info
{
  [self.navigationManager updateFollowingInfo:info];
}

- (void)handleRoutingError
{
  self.navigationManager.state = MWMNavigationDashboardStateError;
  MapsAppDelegate.theApp.routingPlaneMode = MWMRoutingPlaneModePlacePage;
}

- (void)buildRoute
{
  [self.navigationManager.routePreview reloadData];
  if (!self.isPossibleToBuildRoute)
    return;

  CLLocation * lastLocation = [MWMLocationManager lastLocation];
  if (!lastLocation && self.routeSource.IsMyPosition())
  {
    MWMAlertViewController * alert =
        [[MWMAlertViewController alloc] initWithViewController:self.ownerController];
    [alert presentLocationAlert];
    return;
  }

  m2::PointD const locationPoint = lastLocation.mercator;
  if (self.routeSource.IsMyPosition())
  {
    [Statistics
              logEvent:kStatPointToPoint
        withParameters:@{kStatAction : kStatBuildRoute, kStatValue : kStatFromMyPosition}];
    self.routeSource = MWMRoutePoint(locationPoint);
    [MWMLocationManager addObserver:self.navigationManager];
  }
  else if (self.routeDestination.IsMyPosition())
  {
    [Statistics
              logEvent:kStatPointToPoint
        withParameters:@{kStatAction : kStatBuildRoute, kStatValue : kStatToMyPosition}];
    self.routeDestination = MWMRoutePoint(locationPoint);
  }
  else
  {
    [Statistics
              logEvent:kStatPointToPoint
        withParameters:@{kStatAction : kStatBuildRoute, kStatValue : kStatPointToPoint}];
  }

  self.navigationManager.state = MWMNavigationDashboardStatePlanning;
  GetFramework().BuildRoute(self.routeSource.Point(), self.routeDestination.Point(), 0 /* timeoutSec */);
}

- (BOOL)isPossibleToBuildRoute
{
  MWMRoutePoint const zeroPoint = MWMRoutePoint::MWMRoutePointZero();
  return self.routeSource != zeroPoint && self.routeDestination != zeroPoint;
}

- (void)navigationDashBoardDidUpdate
{
  CGFloat const topBound = self.topBound + self.navigationManager.height;
  if (!IPAD)
    [self.sideButtons setTopBound:topBound];
  [self.placePageManager setTopBound:topBound];
}

- (BOOL)didStartRouting
{
  BOOL const isSourceMyPosition = self.routeSource.IsMyPosition();
  BOOL const isDestinationMyPosition = self.routeDestination.IsMyPosition();
  if (isSourceMyPosition)
    [Statistics logEvent:kStatEventName(kStatPointToPoint, kStatGo)
                     withParameters:@{kStatValue : kStatFromMyPosition}];
  else if (isDestinationMyPosition)
    [Statistics logEvent:kStatEventName(kStatPointToPoint, kStatGo)
                     withParameters:@{kStatValue : kStatToMyPosition}];
  else
    [Statistics logEvent:kStatEventName(kStatPointToPoint, kStatGo)
                     withParameters:@{kStatValue : kStatPointToPoint}];

  if (!isSourceMyPosition)
  {
    MWMAlertViewController * controller = [[MWMAlertViewController alloc] initWithViewController:self.ownerController];
    CLLocation * lastLocation = [MWMLocationManager lastLocation];
    BOOL const needToRebuild = lastLocation && !location_helpers::isMyPositionPendingOrNoPosition() && !isDestinationMyPosition;
    m2::PointD const locationPoint = lastLocation.mercator;
    [controller presentPoint2PointAlertWithOkBlock:^
    {
      self.routeSource = MWMRoutePoint(locationPoint);
      [self buildRoute];
    } needToRebuild:needToRebuild];
    return NO;
  }
  self.hidden = NO;
  self.sideButtonsHidden = NO;
  GetFramework().FollowRoute();
  self.disableStandbyOnRouteFollowing = YES;
  MapsAppDelegate * app = MapsAppDelegate.theApp;
  app.routingPlaneMode = MWMRoutingPlaneModeNone;
  [RouteState save];
  [MapsAppDelegate changeMapStyleIfNedeed];
  [app startMapStyleChecker];
  return YES;
}

- (void)didCancelRouting
{
  [Statistics logEvent:kStatEventName(kStatPointToPoint, kStatClose)];
  [MWMLocationManager removeObserver:self.navigationManager];
  self.navigationManager.state = MWMNavigationDashboardStateHidden;
  self.disableStandbyOnRouteFollowing = NO;
  [MapsAppDelegate theApp].routingPlaneMode = MWMRoutingPlaneModeNone;
  [RouteState remove];
  self.menuState = MWMBottomMenuStateInactive;
  [self resetRoutingPoint];
  [self navigationDashBoardDidUpdate];
  if ([MapsAppDelegate isAutoNightMode])
    [MapsAppDelegate resetToDefaultMapStyle];
  GetFramework().CloseRouting();
  [MapsAppDelegate.theApp showAlertIfRequired];
}

- (void)swapPointsAndRebuildRouteIfPossible
{
  [Statistics logEvent:kStatEventName(kStatPointToPoint, kStatSwapRoutingPoints)];
  swap(_routeSource, _routeDestination);
  [self buildRoute];

  auto & f = GetFramework();
  f.SetRouteStartPoint(self.routeSource.Point(),
                       self.routeSource != MWMRoutePoint::MWMRoutePointZero() && !self.routeSource.IsMyPosition());

  f.SetRouteFinishPoint(self.routeDestination.Point(),
                        self.routeDestination != MWMRoutePoint::MWMRoutePointZero());
}

- (void)didStartEditingRoutePoint:(BOOL)isSource
{
  [Statistics logEvent:kStatEventName(kStatPointToPoint, kStatSearch)
                   withParameters:@{kStatValue : (isSource ? kStatSource : kStatDestination)}];
  MapsAppDelegate.theApp.routingPlaneMode = isSource ? MWMRoutingPlaneModeSearchSource : MWMRoutingPlaneModeSearchDestination;
  self.searchManager.state = MWMSearchManagerStateDefault;
}

- (void)resetRoutingPoint
{
  CLLocation * lastLocation = [MWMLocationManager lastLocation];
  self.routeSource =
      lastLocation ? MWMRoutePoint(lastLocation.mercator) : MWMRoutePoint::MWMRoutePointZero();
  self.routeDestination = MWMRoutePoint::MWMRoutePointZero();
}

- (void)routingReady
{
  if (!self.routeSource.IsMyPosition())
  {
    dispatch_async(dispatch_get_main_queue(), ^
    {
      GetFramework().DisableFollowMode();
      [self.navigationManager updateDashboard];
    });
  }
  self.navigationManager.state = MWMNavigationDashboardStateReady;
}

- (void)routingHidden
{
  [self didCancelRouting];
}

- (void)routingPrepare
{
  self.navigationManager.state = MWMNavigationDashboardStatePrepare;
  MapsAppDelegate.theApp.routingPlaneMode = MWMRoutingPlaneModePlacePage;
}

- (void)startNavigation
{
  if (![self didStartRouting])
    return;
  self.navigationManager.state = MWMNavigationDashboardStateNavigation;
  MapsAppDelegate.theApp.routingPlaneMode = MWMRoutingPlaneModeNone;
}

- (void)stopNavigation
{
  [self didCancelRouting];
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

#pragma mark - MWMRoutePreviewDataSource

- (NSString *)source
{
  return self.routeSource.Name();
}

- (NSString *)destination
{
  return self.routeDestination.Name();
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
        [[MWMPlacePageViewManager alloc] initWithViewController:self.ownerController delegate:self];
  return _placePageManager;
}

- (MWMNavigationDashboardManager *)navigationManager
{
  if (!_navigationManager)
    _navigationManager =
        [[MWMNavigationDashboardManager alloc] initWithParentView:self.ownerController.view
                                                      infoDisplay:self.menuController
                                                         delegate:self];
  return _navigationManager;
}

- (MWMSearchManager *)searchManager
{
  if (!_searchManager)
    _searchManager =
        [[MWMSearchManager alloc] initWithParentView:self.ownerController.view delegate:self];
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

- (MWMNavigationDashboardState)navigationState
{
  return self.navigationManager.state;
}

- (MWMPlacePageEntity *)placePageEntity
{
  return self.placePageManager.entity;
}

- (BOOL)isDirectionViewShown
{
  return self.placePageManager.isDirectionViewShown;
}

- (void)setTopBound:(CGFloat)topBound
{
  if (IPAD)
    return;
  _topBound = self.placePageManager.topBound = self.sideButtons.topBound = self.navigationManager.topBound = topBound;
}

- (void)setLeftBound:(CGFloat)leftBound
{
  if (!IPAD)
    return;
  MWMRoutingPlaneMode const m = MapsAppDelegate.theApp.routingPlaneMode;
  if (m != MWMRoutingPlaneModeNone)
    return;
  _leftBound = self.placePageManager.leftBound = self.navigationManager.leftBound = self.menuController.leftBound = leftBound;
}

- (BOOL)searchHidden
{
  return self.searchManager.state == MWMSearchManagerStateHidden;
}

- (void)setSearchHidden:(BOOL)searchHidden
{
  self.searchManager.state = searchHidden ? MWMSearchManagerStateHidden : MWMSearchManagerStateDefault;
}

@end
