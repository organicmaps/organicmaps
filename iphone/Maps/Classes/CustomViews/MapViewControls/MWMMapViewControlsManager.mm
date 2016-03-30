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
#import "MWMObjectsCategorySelectorController.h"
#import "MWMFrameworkListener.h"
#import "MWMFrameworkObservers.h"
#import "MWMMapViewControlsManager.h"
#import "MWMPlacePageEntity.h"
#import "MWMPlacePageViewManager.h"
#import "MWMPlacePageViewManagerDelegate.h"
#import "MWMRoutePreview.h"
#import "MWMSearchManager.h"
#import "MWMSearchView.h"
#import "MWMZoomButtons.h"
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

@property (nonatomic) MWMZoomButtons * zoomButtons;
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
  self.zoomButtons = [[MWMZoomButtons alloc] initWithParentView:controller.view];
  self.menuController = [[MWMBottomMenuViewController alloc] initWithParentController:controller delegate:self];
  self.placePageManager = [[MWMPlacePageViewManager alloc] initWithViewController:controller delegate:self];
  self.navigationManager = [[MWMNavigationDashboardManager alloc] initWithParentView:controller.view delegate:self];
  self.searchManager = [[MWMSearchManager alloc] initWithParentView:controller.view delegate:self];
  self.hidden = NO;
  self.zoomHidden = NO;
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
    LocationManager * m = MapsAppDelegate.theApp.locationManager;
    self.routeSource = m.lastLocationIsValid ? MWMRoutePoint(m.lastLocation.mercator)
                                             : MWMRoutePoint::MWMRoutePointZero();
  }
  self.routeDestination = MWMRoutePoint::MWMRoutePointZero();
}

#pragma mark - MWMFrameworkRouteBuilderObserver

- (void)processRouteBuilderEvent:(routing::IRouter::ResultCode)code
                       countries:(storage::TCountriesVec const &)absentCountries
{
  switch (code)
  {
    case routing::IRouter::ResultCode::NoError:
    {
      [self.navigationManager setRouteBuilderProgress:100];
      self.searchHidden = YES;
      break;
    }
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
  [self.zoomButtons mwm_refreshUI];
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

- (void)actionDownloadMaps
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
    [self.ownerController openMapsDownloader];
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

- (void)addPlace
{
  self.menuState = MWMBottomMenuStateHidden;
  static_cast<EAGLView *>(self.ownerController.view).widgetsManager.fullScreen = YES;
  [self.placePageManager dismissPlacePage];
  self.searchManager.state = MWMSearchManagerStateHidden;

  [MWMAddPlaceNavigationBar showInSuperview:self.ownerController.view doneBlock:^
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

- (void)didFinishAddingPlace
{
  self.menuState = MWMBottomMenuStateInactive;
  static_cast<EAGLView *>(self.ownerController.view).widgetsManager.fullScreen = NO;
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
    [self.zoomButtons setBottomBound:frame.origin.y];
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
  auto & f = GetFramework();
  m2::PointD const myPosition = [MapsAppDelegate theApp].locationManager.lastLocation.mercator;
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
  self.routeSource = from;
  if ((from.IsMyPosition() && self.routeDestination.IsMyPosition()) || from == self.routeDestination)
    self.routeDestination = MWMRoutePoint::MWMRoutePointZero();

  if (IPAD)
    self.searchManager.state = MWMSearchManagerStateHidden;
  [self buildRoute];
  
  GetFramework().SetRouteStartPoint(from.Point(), from != MWMRoutePoint::MWMRoutePointZero() && !from.IsMyPosition());
}

- (void)buildRouteTo:(MWMRoutePoint const &)to
{
  self.routeDestination = to;
  if ((to.IsMyPosition() && self.routeSource.IsMyPosition()) || to == self.routeSource)
    self.routeSource = MWMRoutePoint::MWMRoutePointZero();

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
    self.placePageManager.topBound = self.zoomButtons.topBound = bound;
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

- (void)setupRoutingDashboard:(location::FollowingInfo const &)info
{
  [self.navigationManager setupDashboard:info];
  if (self.menuController.state == MWMBottomMenuStateText)
    [self.menuController setStreetName:@(info.m_sourceName.c_str())];
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

  LocationManager * locMgr = [MapsAppDelegate theApp].locationManager;
  if (!locMgr.lastLocationIsValid && self.routeSource.IsMyPosition())
  {
    MWMAlertViewController * alert =
        [[MWMAlertViewController alloc] initWithViewController:self.ownerController];
    [alert presentLocationAlert];
    return;
  }

  m2::PointD const locationPoint = locMgr.lastLocation.mercator;
  if (self.routeSource.IsMyPosition())
  {
    [Statistics
              logEvent:kStatPointToPoint
        withParameters:@{kStatAction : kStatBuildRoute, kStatValue : kStatFromMyPosition}];
    self.routeSource = MWMRoutePoint(locationPoint);
    [locMgr start:self.navigationManager];
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
  self.menuState = MWMBottomMenuStatePlanning;
  GetFramework().BuildRoute(self.routeSource.Point(), self.routeDestination.Point(), 0 /* timeoutSec */);
  [self.navigationManager setRouteBuilderProgress:0.];
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
    [self.zoomButtons setTopBound:topBound];
  [self.placePageManager setTopBound:topBound];
}

- (BOOL)didStartFollowing
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
    LocationManager * manager = MapsAppDelegate.theApp.locationManager;
    BOOL const needToRebuild = manager.lastLocationIsValid && !manager.isLocationModeUnknownOrPending && !isDestinationMyPosition;
    m2::PointD const locationPoint = manager.lastLocation.mercator;
    [controller presentPoint2PointAlertWithOkBlock:^
    {
      self.routeSource = MWMRoutePoint(locationPoint);
      [self buildRoute];
    } needToRebuild:needToRebuild];
    return NO;
  }
  self.hidden = NO;
  self.zoomHidden = NO;
  GetFramework().FollowRoute();
  self.disableStandbyOnRouteFollowing = YES;
  [self.menuController setStreetName:@""];
  MapsAppDelegate * app = MapsAppDelegate.theApp;
  app.routingPlaneMode = MWMRoutingPlaneModeNone;
  [RouteState save];
  if ([MapsAppDelegate isAutoNightMode])
  {
    [MapsAppDelegate changeMapStyleIfNedeed];
    [app startMapStyleChecker];
  }
  return YES;
}

- (void)didCancelRouting
{
  [Statistics logEvent:kStatEventName(kStatPointToPoint, kStatClose)];
  [[MapsAppDelegate theApp].locationManager stop:self.navigationManager];
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
  LocationManager * m = MapsAppDelegate.theApp.locationManager;
  self.routeSource = m.lastLocationIsValid ? MWMRoutePoint(m.lastLocation.mercator) :
                                             MWMRoutePoint::MWMRoutePointZero();
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
  self.menuState = MWMBottomMenuStateGo;
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

- (void)routingNavigation
{
  if (![self didStartFollowing])
    return;
  self.navigationManager.state = MWMNavigationDashboardStateNavigation;
  MapsAppDelegate.theApp.routingPlaneMode = MWMRoutingPlaneModeNone;
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

@synthesize menuState = _menuState;

- (void)setHidden:(BOOL)hidden
{
  if (_hidden == hidden)
    return;
  _hidden = hidden;
  self.zoomHidden = _zoomHidden;
  self.menuState = _menuState;
  EAGLView * glView = (EAGLView *)self.ownerController.view;
  glView.widgetsManager.fullScreen = hidden;
}

- (void)setZoomHidden:(BOOL)zoomHidden
{
  _zoomHidden = zoomHidden;
  self.zoomButtons.hidden = self.hidden || zoomHidden;
}

- (void)setMenuState:(MWMBottomMenuState)menuState
{
  _menuState = menuState;
  MWMBottomMenuState const state = self.hidden ? MWMBottomMenuStateHidden : menuState;
  switch (state)
  {
    case MWMBottomMenuStateInactive:
      [self.menuController setInactive];
      break;
    case MWMBottomMenuStatePlanning:
      [self.menuController setPlanning];
      break;
    case MWMBottomMenuStateGo:
      [self.menuController setGo];
      break;
    default:
      self.menuController.state = state;
      break;
  }
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
  _topBound = self.placePageManager.topBound = self.zoomButtons.topBound = self.navigationManager.topBound = topBound;
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
