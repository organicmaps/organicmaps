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
#import "MWMSettings.h"
#import "MWMStorage.h"
#import "MWMTextToSpeech.h"
#import "MapViewController.h"
#import "MapsAppDelegate.h"
#import "Statistics.h"
#import "UIImage+RGBAData.h"

#include "Framework.h"

#include "platform/local_country_file_utils.hpp"
#include "platform/measurement_utils.hpp"

using namespace routing;

namespace
{
char const * kRenderAltitudeImagesQueueLabel = "mapsme.mwmrouter.renderAltitudeImagesQueue";

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

@property(nonatomic) NSMutableDictionary<NSValue *, NSData *> * altitudeImagesData;
@property(nonatomic) NSString * altitudeElevation;
@property(nonatomic) dispatch_queue_t renderAltitudeImagesQueue;

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

+ (BOOL)hasRouteAltitude { return GetFramework().HasRouteAltitude(); }
+ (BOOL)isTaxi { return GetFramework().GetRouter() == routing::RouterType::Taxi; }
+ (void)startRouting
{
  if ([self isTaxi])
    [[UIApplication sharedApplication] openURL:[MWMNavigationDashboardManager manager].taxiDataSource.taxiURL];
  else
    [[self router] start];
}

- (instancetype)initRouter
{
  self = [super init];
  if (self)
  {
    self.altitudeImagesData = [@{} mutableCopy];
    self.renderAltitudeImagesQueue =
        dispatch_queue_create(kRenderAltitudeImagesQueueLabel, DISPATCH_QUEUE_SERIAL);
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
  [self clearAltitudeImagesData];
  // Taxi can't be used as best router.
  if ([MWMRouter isTaxi])
    bestRouter = NO;

  auto const setTags = ^(RouterType t, BOOL isP2P)
  {
    NSMutableString * tag = (isP2P ? @"routing_p2p_" : @"routing_").mutableCopy;
    switch (t)
    {
    case RouterType::Vehicle:
      [tag appendString:@"vehicle_discovered"];
      break;
    case RouterType::Pedestrian:
      [tag appendString:@"pedestrian_discovered"];
      break;
    case RouterType::Bicycle:
      [tag appendString:@"bicycle_discovered"];
      break;
    case RouterType::Taxi:
      [tag appendString:@"uber_discovered"];
      break;
    }
    [[PushNotificationManager pushManager] setTags:@{ tag : @YES }];
  };

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
    setTags(self.type, YES);
  }

  MWMMapViewControlsManager * mapViewControlsManager = [MWMMapViewControlsManager manager];
  [mapViewControlsManager onRoutePrepare];
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
  [mapViewControlsManager onRouteRebuild];
  setTags(self.type, NO);
}

- (void)start
{
  auto const doStart = ^{
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
  };

  if ([MWMSettings routingDisclaimerApproved])
  {
    doStart();
  }
  else
  {
    [[MWMAlertViewController activeAlertController] presentRoutingDisclaimerAlertWithOkBlock:^{
      doStart();
      [MWMSettings setRoutingDisclaimerApproved];
    }];
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
  // Don't save taxi routing type as default.
  if ([MWMRouter isTaxi])
    GetFramework().SetRouter(routing::RouterType::Vehicle);

  [self clearAltitudeImagesData];
  GetFramework().CloseRouting();
  MapsAppDelegate * app = [MapsAppDelegate theApp];
  app.routingPlaneMode = MWMRoutingPlaneModeNone;
  [MWMRouterSavedState remove];
  if ([MWMSettings autoNightModeEnabled])
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

- (void)routeAltitudeImageForSize:(CGSize)size completion:(MWMImageHeightBlock)block
{
  dispatch_async(self.renderAltitudeImagesQueue, ^{
    if (![MWMRouter hasRouteAltitude])
      return;
    CGFloat const screenScale = [UIScreen mainScreen].scale;
    CGSize const scaledSize = {.width = size.width * screenScale,
                               .height = size.height * screenScale};
    uint32_t const width = static_cast<uint32_t>(scaledSize.width);
    uint32_t const height = static_cast<uint32_t>(scaledSize.height);
    if (width == 0 || height == 0)
      return;

    NSValue * sizeValue = [NSValue valueWithCGSize:scaledSize];
    NSData * imageData = self.altitudeImagesData[sizeValue];
    if (!imageData)
    {
      vector<uint8_t> imageRGBAData;
      int32_t minRouteAltitude = 0;
      int32_t maxRouteAltitude = 0;
      measurement_utils::Units units = measurement_utils::Units::Metric;
      if (!GetFramework().GenerateRouteAltitudeChart(width, height, imageRGBAData,
                                                     minRouteAltitude, maxRouteAltitude, units))
      {
        return;
      }
      if (imageRGBAData.empty())
        return;
      imageData = [NSData dataWithBytes:imageRGBAData.data() length:imageRGBAData.size()];
      self.altitudeImagesData[sizeValue] = imageData;

      string heightString;
      measurement_utils::FormatDistance(maxRouteAltitude - minRouteAltitude, heightString);
      self.altitudeElevation = @(heightString.c_str());
    }

    dispatch_async(dispatch_get_main_queue(), ^{
      UIImage * altitudeImage = [UIImage imageWithRGBAData:imageData width:width height:height];
      if (altitudeImage)
        block(altitudeImage, self.altitudeElevation);
    });
  });
}

- (void)clearAltitudeImagesData
{
  dispatch_async(self.renderAltitudeImagesQueue, ^{
    [self.altitudeImagesData removeAllObjects];
    self.altitudeElevation = nil;
  });
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
  MWMMapViewControlsManager * mapViewControlsManager = [MWMMapViewControlsManager manager];
  switch (code)
  {
  case routing::IRouter::ResultCode::NoError:
  {
    auto & f = GetFramework();
    f.DeactivateMapSelection(true);
    if (state.forceStateChange == MWMRouterForceStateChange::Start)
      [self start];
    else
      [mapViewControlsManager onRouteReady];
    [self updateFollowingInfo];
    if (![MWMRouter isTaxi])
      [[MWMNavigationDashboardManager manager] setRouteBuilderProgress:100];

    mapViewControlsManager.searchHidden = YES;
    break;
  }
  case routing::IRouter::RouteFileNotExist:
  case routing::IRouter::InconsistentMWMandRoute:
  case routing::IRouter::NeedMoreMaps:
  case routing::IRouter::FileTooOld:
  case routing::IRouter::RouteNotFound:
    [self presentDownloaderAlert:code countries:absentCountries];
    [mapViewControlsManager onRouteError];
    break;
  case routing::IRouter::Cancelled: break;
  case routing::IRouter::StartPointNotFound:
  case routing::IRouter::EndPointNotFound:
  case routing::IRouter::NoCurrentPosition:
  case routing::IRouter::PointsInDifferentMWM:
  case routing::IRouter::InternalError:
    [[MWMAlertViewController activeAlertController] presentAlert:code];
    [mapViewControlsManager onRouteError];
    break;
  }
  state.forceStateChange = MWMRouterForceStateChange::None;
}

- (void)processRouteBuilderProgress:(CGFloat)progress
{
  if (![MWMRouter isTaxi])
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
