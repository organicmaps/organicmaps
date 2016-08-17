#import "MWMRouter.h"
#import <Pushwoosh/PushNotificationManager.h>
#import "MWMAlertViewController.h"
#import "MWMFrameworkListener.h"
#import "MWMLocationHelpers.h"
#import "MWMLocationManager.h"
#import "MWMMapViewControlsManager.h"
#import "MWMNavigationDashboardManager.h"
#import "MWMRouterSavedState.h"
#import "MWMSearch.h"
#import "MWMStorage.h"
#import "MWMTextToSpeech.h"
#import "MapViewController.h"
#import "MapsAppDelegate.h"
#import "Statistics.h"

#include "Framework.h"

#include "platform/local_country_file_utils.hpp"

using namespace routing;

namespace
{
MWMRoutePoint lastLocationPoint()
{
  CLLocation * lastLocation = [MWMLocationManager lastLocation];
  return lastLocation ? MWMRoutePoint(lastLocation.mercator) : MWMRoutePoint::MWMRoutePointZero();
}

bool isMarkerPoint(MWMRoutePoint const & point) { return point.IsValid() && !point.IsMyPosition(); }
}  // namespace

@interface MWMRouter ()<MWMLocationObserver, MWMFrameworkRouteBuilderObserver>

@property(nonatomic, readwrite) MWMRoutePoint startPoint;
@property(nonatomic, readwrite) MWMRoutePoint finishPoint;

@end

@implementation MWMRouter

+ (MWMRouter *)router
{
  static MWMRouter * router;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    router = [[super alloc] initRouter];
  });
  return router;
}

- (instancetype)initRouter
{
  self = [super init];
  if (self)
  {
    [self resetPoints];
    [MWMLocationManager addObserver:self];
    [MWMFrameworkListener addObserver:self];
  }
  return self;
}

- (void)resetPoints
{
  self.startPoint = lastLocationPoint();
  self.finishPoint = MWMRoutePoint::MWMRoutePointZero();
}

- (void)setType:(RouterType)type
{
  if (type == self.type)
    return;
  [self doStop];
  GetFramework().SetRouter(type);
}

- (RouterType)type { return GetFramework().GetRouter(); }
- (BOOL)arePointsValidForRouting
{
  MWMRoutePoint const zeroPoint = MWMRoutePoint::MWMRoutePointZero();
  return self.startPoint != zeroPoint && self.finishPoint != zeroPoint &&
         self.startPoint != self.finishPoint;
}

- (void)swapPointsAndRebuild
{
  [Statistics logEvent:kStatEventName(kStatPointToPoint, kStatSwapRoutingPoints)];
  swap(_startPoint, _finishPoint);
  [self rebuildWithBestRouter:NO];
}

- (void)buildFromPoint:(MWMRoutePoint const &)startPoint bestRouter:(BOOL)bestRouter
{
  self.startPoint = startPoint;
  [self rebuildWithBestRouter:bestRouter];
  if (!self.finishPoint.IsValid())
    [[MWMMapViewControlsManager manager] onRoutePrepare];
}

- (void)buildToPoint:(MWMRoutePoint const &)finishPoint bestRouter:(BOOL)bestRouter
{
  if (!self.startPoint.IsValid() && !finishPoint.IsMyPosition())
    self.startPoint = lastLocationPoint();
  self.finishPoint = finishPoint;
  [self rebuildWithBestRouter:bestRouter];
}

- (void)buildFromPoint:(MWMRoutePoint const &)start
               toPoint:(MWMRoutePoint const &)finish
            bestRouter:(BOOL)bestRouter
{
  self.startPoint = start;
  self.finishPoint = finish;
  [self rebuildWithBestRouter:bestRouter];
}

- (void)rebuildWithBestRouter:(BOOL)bestRouter
{
  if (self.startPoint.IsMyPosition())
  {
    [Statistics logEvent:kStatPointToPoint
          withParameters:@{kStatAction : kStatBuildRoute, kStatValue : kStatFromMyPosition}];
    self.startPoint = lastLocationPoint();
  }
  else if (self.finishPoint.IsMyPosition())
  {
    [Statistics logEvent:kStatPointToPoint
          withParameters:@{kStatAction : kStatBuildRoute, kStatValue : kStatToMyPosition}];
    self.finishPoint = lastLocationPoint();
  }
  else
  {
    [Statistics logEvent:kStatPointToPoint
          withParameters:@{kStatAction : kStatBuildRoute, kStatValue : kStatPointToPoint}];
    switch (self.type)
    {
    case RouterType::Vehicle:
      [[PushNotificationManager pushManager] setTags:@{ @"routing_p2p_vehicle_discovered" : @YES }];
      break;
    case RouterType::Pedestrian:
      [[PushNotificationManager pushManager] setTags:@{
        @"routing_p2p_pedestrian_discovered" : @YES
      }];
      break;
    case RouterType::Bicycle:
      [[PushNotificationManager pushManager] setTags:@{ @"routing_p2p_bicycle_discovered" : @YES }];
      break;
    }
  }

  if (![self arePointsValidForRouting])
    return;
  auto & f = GetFramework();
  auto const & startPoint = self.startPoint.Point();
  auto const & finishPoint = self.finishPoint.Point();
  if (bestRouter)
    self.type = GetFramework().GetBestRouter(startPoint, finishPoint);
  f.BuildRoute(startPoint, finishPoint, 0 /* timeoutSec */);
  f.SetRouteStartPoint(startPoint, isMarkerPoint(self.startPoint));
  f.SetRouteFinishPoint(finishPoint, isMarkerPoint(self.finishPoint));
  [[MWMMapViewControlsManager manager] onRouteRebuild];
  switch (self.type)
  {
  case RouterType::Vehicle:
    [[PushNotificationManager pushManager] setTags:@{ @"routing_vehicle_discovered" : @YES }];
    break;
  case RouterType::Pedestrian:
    [[PushNotificationManager pushManager] setTags:@{ @"routing_pedestrian_discovered" : @YES }];
    break;
  case RouterType::Bicycle:
    [[PushNotificationManager pushManager] setTags:@{ @"routing_bicycle_discovered" : @YES }];
    break;
  }
}

- (void)start
{
  if (self.startPoint.IsMyPosition())
    [Statistics logEvent:kStatEventName(kStatPointToPoint, kStatGo)
          withParameters:@{kStatValue : kStatFromMyPosition}];
  else if (self.finishPoint.IsMyPosition())
    [Statistics logEvent:kStatEventName(kStatPointToPoint, kStatGo)
          withParameters:@{kStatValue : kStatToMyPosition}];
  else
    [Statistics logEvent:kStatEventName(kStatPointToPoint, kStatGo)
          withParameters:@{kStatValue : kStatPointToPoint}];

  if (self.startPoint.IsMyPosition())
  {
    GetFramework().FollowRoute();
    [[MWMMapViewControlsManager manager] onRouteStart];
    MapsAppDelegate * app = [MapsAppDelegate theApp];
    app.routingPlaneMode = MWMRoutingPlaneModeNone;
    [MWMRouterSavedState store];
    [MapsAppDelegate changeMapStyleIfNedeed];
    [app startMapStyleChecker];
  }
  else
  {
    MWMAlertViewController * alertController = [MWMAlertViewController activeAlertController];
    CLLocation * lastLocation = [MWMLocationManager lastLocation];
    BOOL const needToRebuild = lastLocation &&
                               !location_helpers::isMyPositionPendingOrNoPosition() &&
                               !self.finishPoint.IsMyPosition();
    [alertController presentPoint2PointAlertWithOkBlock:^{
      [self buildFromPoint:lastLocationPoint() bestRouter:NO];
    }
                                          needToRebuild:needToRebuild];
  }
}

- (void)stop
{
  [Statistics logEvent:kStatEventName(kStatPointToPoint, kStatClose)];
  [MWMSearch clear];
  [self resetPoints];
  [self doStop];
  [[MWMMapViewControlsManager manager] onRouteStop];
}

- (void)doStop
{
  GetFramework().CloseRouting();
  MapsAppDelegate * app = [MapsAppDelegate theApp];
  app.routingPlaneMode = MWMRoutingPlaneModeNone;
  [MWMRouterSavedState remove];
  if ([MapsAppDelegate isAutoNightMode])
    [MapsAppDelegate resetToDefaultMapStyle];
  [app showAlertIfRequired];
}

- (void)updateFollowingInfo
{
  auto & f = GetFramework();
  if (!f.IsRoutingActive())
    return;
  location::FollowingInfo info;
  f.GetRouteFollowingInfo(info);
  if (info.IsValid())
    [[MWMNavigationDashboardManager manager] updateFollowingInfo:info];
}

- (void)showDisclaimer
{
  bool isDisclaimerApproved = false;
  UNUSED_VALUE(settings::Get("IsDisclaimerApproved", isDisclaimerApproved));
  if (isDisclaimerApproved)
    return;
  [[MWMAlertViewController activeAlertController] presentRoutingDisclaimerAlert];
  settings::Set("IsDisclaimerApproved", true);
}

#pragma mark - MWMLocationObserver

- (void)onLocationUpdate:(location::GpsInfo const &)info
{
  auto & f = GetFramework();
  if (f.IsRoutingActive())
  {
    MWMTextToSpeech * tts = [MWMTextToSpeech tts];
    if (f.IsOnRoute() && tts.active)
      [tts playTurnNotifications];

    [self updateFollowingInfo];
  }
  else
  {
    MWMRouterSavedState * state = [MWMRouterSavedState state];
    if (state.forceStateChange == MWMRouterForceStateChange::Rebuild)
    {
      state.forceStateChange = MWMRouterForceStateChange::Start;
      self.type = GetFramework().GetLastUsedRouter();
      [self buildToPoint:state.restorePoint bestRouter:NO];
    }
  }
}

#pragma mark - MWMFrameworkRouteBuilderObserver

- (void)processRouteBuilderEvent:(routing::IRouter::ResultCode)code
                       countries:(storage::TCountriesVec const &)absentCountries
{
  MWMRouterSavedState * state = [MWMRouterSavedState state];
  switch (code)
  {
  case routing::IRouter::ResultCode::NoError:
  {
    auto & f = GetFramework();
    f.DeactivateMapSelection(true);
    if (state.forceStateChange == MWMRouterForceStateChange::Start)
      [self start];
    else
      [[MWMMapViewControlsManager manager] onRouteReady];
    [self updateFollowingInfo];
    [self showDisclaimer];
    [[MWMNavigationDashboardManager manager] setRouteBuilderProgress:100];
    [MWMMapViewControlsManager manager].searchHidden = YES;
    break;
  }
  case routing::IRouter::RouteFileNotExist:
  case routing::IRouter::InconsistentMWMandRoute:
  case routing::IRouter::NeedMoreMaps:
  case routing::IRouter::FileTooOld:
  case routing::IRouter::RouteNotFound:
    [self presentDownloaderAlert:code countries:absentCountries];
    [[MWMMapViewControlsManager manager] onRouteError];
    break;
  case routing::IRouter::Cancelled: break;
  case routing::IRouter::StartPointNotFound:
  case routing::IRouter::EndPointNotFound:
  case routing::IRouter::NoCurrentPosition:
  case routing::IRouter::PointsInDifferentMWM:
  case routing::IRouter::InternalError:
    [[MWMAlertViewController activeAlertController] presentAlert:code];
    [[MWMMapViewControlsManager manager] onRouteError];
    break;
  }
  state.forceStateChange = MWMRouterForceStateChange::None;
}

- (void)processRouteBuilderProgress:(CGFloat)progress
{
  [[MWMNavigationDashboardManager manager] setRouteBuilderProgress:progress];
}

#pragma mark - Alerts

- (void)presentDownloaderAlert:(routing::IRouter::ResultCode)code
                     countries:(storage::TCountriesVec const &)countries
{
  MWMAlertViewController * activeAlertController = [MWMAlertViewController activeAlertController];
  if (platform::migrate::NeedMigrate())
  {
    [activeAlertController presentRoutingMigrationAlertWithOkBlock:^{
      [Statistics logEvent:kStatDownloaderMigrationDialogue
            withParameters:@{kStatFrom : kStatRouting}];
      [[MapViewController controller] openMigration];
    }];
  }
  else if (!countries.empty())
  {
    [activeAlertController presentDownloaderAlertWithCountries:countries
        code:code
        cancelBlock:^{
          if (code != routing::IRouter::NeedMoreMaps)
            [[MWMRouter router] stop];
        }
        downloadBlock:^(storage::TCountriesVec const & downloadCountries, TMWMVoidBlock onSuccess) {
          [MWMStorage downloadNodes:downloadCountries
                    alertController:activeAlertController
                          onSuccess:onSuccess];
        }
        downloadCompleteBlock:^{
          [[MWMRouter router] rebuildWithBestRouter:NO];
        }];
  }
  else
  {
    [activeAlertController presentAlert:code];
  }
}

#pragma mark - Properties

- (void)setStartPoint:(MWMRoutePoint)startPoint
{
  if (_startPoint == startPoint)
    return;
  _startPoint = startPoint;
  if (startPoint == self.finishPoint)
    self.finishPoint = MWMRoutePoint::MWMRoutePointZero();
  [[MWMNavigationDashboardManager manager].routePreview reloadData];
}

- (void)setFinishPoint:(MWMRoutePoint)finishPoint
{
  if (_finishPoint == finishPoint)
    return;
  _finishPoint = finishPoint;
  if (finishPoint == self.startPoint)
    self.startPoint = MWMRoutePoint::MWMRoutePointZero();
  [[MWMNavigationDashboardManager manager].routePreview reloadData];
}

@end
