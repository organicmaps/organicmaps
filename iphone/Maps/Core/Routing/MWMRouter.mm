#import "MWMRouter.h"
#import <Pushwoosh/PushNotificationManager.h>
#import "CLLocation+Mercator.h"
#import "MWMAlertViewController.h"
#import "MWMCoreRouterType.h"
#import "MWMFrameworkListener.h"
#import "MWMLocationHelpers.h"
#import "MWMLocationManager.h"
#import "MWMLocationObserver.h"
#import "MWMMapViewControlsManager.h"
#import "MWMNavigationDashboardManager.h"
#import "MWMRoutePoint.h"
#import "MWMRouterSavedState.h"
#import "MWMSearch.h"
#import "MWMSettings.h"
#import "MWMStorage.h"
#import "MWMTextToSpeech.h"
#import "MapViewController.h"
#import "MapsAppDelegate.h"
#import "Statistics.h"
#import "SwiftBridge.h"
#import "UIImage+RGBAData.h"

#include "Framework.h"

#include "platform/local_country_file_utils.hpp"
#include "platform/measurement_utils.hpp"

using namespace routing;

namespace
{
char const * kRenderAltitudeImagesQueueLabel = "mapsme.mwmrouter.renderAltitudeImagesQueue";

MWMRoutePoint * lastLocationPoint()
{
  CLLocation * lastLocation = [MWMLocationManager lastLocation];
  return lastLocation ? routePoint(lastLocation.mercator) : zeroRoutePoint();
}

bool isMarkerPoint(MWMRoutePoint * point) { return point.isValid && !point.isMyPosition; }
}  // namespace

@interface MWMRouter ()<MWMLocationObserver, MWMFrameworkRouteBuilderObserver>

@property(nonatomic, readwrite) MWMRoutePoint * startPoint;
@property(nonatomic, readwrite) MWMRoutePoint * finishPoint;

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
  auto router = [self router];
  if (![self isTaxi])
  {
    [router start];
    return;
  }

  auto taxiDataSource = [MWMNavigationDashboardManager manager].taxiDataSource;
  auto eventName = taxiDataSource.isTaxiInstalled ? kStatRoutingTaxiOrder : kStatRoutingTaxiInstall;
  auto const sLatLon = routePointLatLon(router.startPoint);
  auto const fLatLon = routePointLatLon(router.finishPoint);

  [Statistics logEvent:eventName
        withParameters:@{
          kStatProvider : kStatUber,
          kStatFromLocation : makeLocationEventValue(sLatLon.lat, sLatLon.lon),
          kStatToLocation : makeLocationEventValue(fLatLon.lat, fLatLon.lon)
        }
            atLocation:[MWMLocationManager lastLocation]];

  [[UIApplication sharedApplication] openURL:taxiDataSource.taxiURL];
}

+ (void)stopRouting
{
  [[self router] stop];
  [MWMNavigationDashboardManager manager].taxiDataSource = nil;
}

+ (BOOL)isRoutingActive { return GetFramework().IsRoutingActive(); }
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
  self.finishPoint = zeroRoutePoint();
}

- (void)setType:(MWMRouterType)type
{
  if (type == self.type)
    return;
  [self doStop];
  GetFramework().SetRouter(coreRouterType(type));
}

- (MWMRouterType)type { return routerType(GetFramework().GetRouter()); }
- (BOOL)arePointsValidForRouting
{
  return self.startPoint.isValid && self.finishPoint.isValid && self.startPoint != self.finishPoint;
}

- (void)swapPointsAndRebuild
{
  [Statistics logEvent:kStatEventName(kStatPointToPoint, kStatSwapRoutingPoints)];
  swap(_startPoint, _finishPoint);
  [self rebuildWithBestRouter:NO];
}

- (void)buildFromPoint:(MWMRoutePoint *)startPoint bestRouter:(BOOL)bestRouter
{
  self.startPoint = startPoint;
  [self rebuildWithBestRouter:bestRouter];
}

- (void)buildToPoint:(MWMRoutePoint *)finishPoint bestRouter:(BOOL)bestRouter
{
  if (!self.startPoint.isValid && !finishPoint.isMyPosition)
    self.startPoint = lastLocationPoint();
  self.finishPoint = finishPoint;
  [self rebuildWithBestRouter:bestRouter];
}

- (void)buildFromPoint:(MWMRoutePoint *)start
               toPoint:(MWMRoutePoint *)finish
            bestRouter:(BOOL)bestRouter
{
  self.startPoint = start;
  self.finishPoint = finish;
  [self rebuildWithBestRouter:bestRouter];
}

- (void)rebuildWithBestRouter:(BOOL)bestRouter
{
  [self clearAltitudeImagesData];

  bool isP2P = false;
  if (self.startPoint.isMyPosition)
  {
    [Statistics logEvent:kStatPointToPoint
          withParameters:@{kStatAction : kStatBuildRoute, kStatValue : kStatFromMyPosition}];
    self.startPoint = lastLocationPoint();
  }
  else if (self.finishPoint.isMyPosition)
  {
    [Statistics logEvent:kStatPointToPoint
          withParameters:@{kStatAction : kStatBuildRoute, kStatValue : kStatToMyPosition}];
    self.finishPoint = lastLocationPoint();
  }
  else
  {
    [Statistics logEvent:kStatPointToPoint
          withParameters:@{kStatAction : kStatBuildRoute, kStatValue : kStatPointToPoint}];
    isP2P = true;
  }

  MWMMapViewControlsManager * mapViewControlsManager = [MWMMapViewControlsManager manager];
  [mapViewControlsManager onRoutePrepare];
  if (![self arePointsValidForRouting])
    return;
  auto & f = GetFramework();
  auto startPoint = mercatorMWMRoutePoint(self.startPoint);
  auto finishPoint = mercatorMWMRoutePoint(self.finishPoint);
  // Taxi can't be used as best router.
  if (bestRouter && ![[self class] isTaxi])
    self.type = routerType(GetFramework().GetBestRouter(startPoint, finishPoint));
  f.BuildRoute(startPoint, finishPoint, isP2P, 0 /* timeoutSec */);
  f.SetRouteStartPoint(startPoint, isMarkerPoint(self.startPoint));
  f.SetRouteFinishPoint(finishPoint, isMarkerPoint(self.finishPoint));
  [mapViewControlsManager onRouteRebuild];
}

- (void)start
{
  auto const doStart = ^{
    if (self.startPoint.isMyPosition)
      [Statistics logEvent:kStatEventName(kStatPointToPoint, kStatGo)
            withParameters:@{kStatValue : kStatFromMyPosition}];
    else if (self.finishPoint.isMyPosition)
      [Statistics logEvent:kStatEventName(kStatPointToPoint, kStatGo)
            withParameters:@{kStatValue : kStatToMyPosition}];
    else
      [Statistics logEvent:kStatEventName(kStatPointToPoint, kStatGo)
            withParameters:@{kStatValue : kStatPointToPoint}];

    if (self.startPoint.isMyPosition)
    {
      GetFramework().FollowRoute();
      [[MWMMapViewControlsManager manager] onRouteStart];
      MapsAppDelegate * app = [MapsAppDelegate theApp];
      app.routingPlaneMode = MWMRoutingPlaneModeNone;
      [MWMRouterSavedState store];
      [MWMThemeManager setAutoUpdates:YES];
    }
    else
    {
      MWMAlertViewController * alertController = [MWMAlertViewController activeAlertController];
      CLLocation * lastLocation = [MWMLocationManager lastLocation];
      BOOL const needToRebuild = lastLocation &&
                                 !location_helpers::isMyPositionPendingOrNoPosition() &&
                                 !self.finishPoint.isMyPosition;
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
  if ([[self class] isTaxi])
    GetFramework().SetRouter(routing::RouterType::Vehicle);

  [self clearAltitudeImagesData];
  GetFramework().CloseRouting();
  MapsAppDelegate * app = [MapsAppDelegate theApp];
  app.routingPlaneMode = MWMRoutingPlaneModeNone;
  [MWMRouterSavedState remove];
  [MWMThemeManager setAutoUpdates:NO];
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
    if (![[self class] hasRouteAltitude])
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
      self.type = routerType(GetFramework().GetLastUsedRouter());
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
    if (![[self class] isTaxi])
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
  if (![[self class] isTaxi])
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
            [[[self class] router] stop];
        }
        downloadBlock:^(storage::TCountriesVec const & downloadCountries, MWMVoidBlock onSuccess) {
          [MWMStorage downloadNodes:downloadCountries
                          onSuccess:onSuccess];
        }
        downloadCompleteBlock:^{
          [[[self class] router] rebuildWithBestRouter:NO];
        }];
  }
  else
  {
    [activeAlertController presentAlert:code];
  }
}

#pragma mark - Properties

- (void)setStartPoint:(MWMRoutePoint *)startPoint
{
  if (_startPoint == startPoint)
    return;
  _startPoint = startPoint;
  if (startPoint == self.finishPoint)
    self.finishPoint = zeroRoutePoint();
  [[MWMNavigationDashboardManager manager].routePreview reloadData];
}

- (void)setFinishPoint:(MWMRoutePoint *)finishPoint
{
  if (_finishPoint == finishPoint)
    return;
  _finishPoint = finishPoint;
  if (finishPoint == self.startPoint)
    self.startPoint = zeroRoutePoint();
  [[MWMNavigationDashboardManager manager].routePreview reloadData];
}

@end
