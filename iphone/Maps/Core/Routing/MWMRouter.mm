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

m2::PointD getMercator(MWMRoutePoint * p)
{
  if (p.isMyPosition)
    return mercatorMWMRoutePoint(lastLocationPoint());
  return mercatorMWMRoutePoint(p);
}
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

+ (BOOL)hasRouteAltitude { return GetFramework().GetRoutingManager().HasRouteAltitude(); }
+ (BOOL)isTaxi
{
  return GetFramework().GetRoutingManager().GetRouter() == routing::RouterType::Taxi;
}
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

+ (BOOL)isRoutingActive { return GetFramework().GetRoutingManager().IsRoutingActive(); }

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
  [self doStop:NO];
  GetFramework().GetRoutingManager().SetRouter(coreRouterType(type));
}

- (MWMRouterType)type { return routerType(GetFramework().GetRoutingManager().GetRouter()); }

- (BOOL)arePointsValidForRouting
{
  return self.startPoint.isValid && self.finishPoint.isValid && self.startPoint != self.finishPoint;
}

- (void)applyRoutePoints
{
  auto & rm = GetFramework().GetRoutingManager();
  if (self.startPoint.isValid)
  {
    RouteMarkData pt;
    pt.m_pointType = RouteMarkType::Start;
    pt.m_isMyPosition = self.startPoint.isMyPosition;
    pt.m_position = getMercator(self.startPoint);
    rm.AddRoutePoint(std::move(pt));
  }
  if (self.finishPoint.isValid)
  {
    RouteMarkData pt;
    pt.m_pointType = RouteMarkType::Finish;
    pt.m_isMyPosition = self.finishPoint.isMyPosition;
    pt.m_position = getMercator(self.finishPoint);
    rm.AddRoutePoint(std::move(pt));
  }
}

- (void)removeRoutePoint:(RouteMarkType)type intermediateIndex:(int)intermediateIndex
{
  auto & rm = GetFramework().GetRoutingManager();
  rm.RemoveRoutePoint(type, intermediateIndex);
  auto points = rm.GetRoutePoints();
  if (points.empty())
  {
    // No more than 1 point exist.
    if (type == RouteMarkType::Start)
      self.startPoint = zeroRoutePoint();
    else if (type == RouteMarkType::Finish)
      self.finishPoint = zeroRoutePoint();
  }
  else
  {
    // At least 2 points exist, one of them may (or may not) be my position.
    self.startPoint = rm.IsMyPosition(RouteMarkType::Start) ?
                                      routePoint(points.front().m_position) :
                                      routePoint(points.front().m_position, nil);
    self.finishPoint = rm.IsMyPosition(RouteMarkType::Finish) ?
                                       routePoint(points.back().m_position) :
                                       routePoint(points.back().m_position, nil);
  }
}

- (void)swapPointsAndRebuild
{
  [Statistics logEvent:kStatEventName(kStatPointToPoint, kStatSwapRoutingPoints)];
  std::swap(_startPoint, _finishPoint);
  [self applyRoutePoints];
  [self rebuildWithBestRouter:NO];
}

- (void)removeStartPointAndRebuild:(int)intermediateIndex
{
  [self removeRoutePoint:RouteMarkType::Start intermediateIndex:intermediateIndex];
  [self rebuildWithBestRouter:NO];
}

- (void)removeFinishPointAndRebuild:(int)intermediateIndex
{
  [self removeRoutePoint:RouteMarkType::Finish intermediateIndex:intermediateIndex];
  [self rebuildWithBestRouter:NO];
}

- (void)addIntermediatePointAndRebuild:(MWMRoutePoint *)point intermediateIndex:(int)intermediateIndex
{
  RouteMarkData pt;
  pt.m_pointType = RouteMarkType::Intermediate;
  pt.m_position = getMercator(point);
  pt.m_intermediateIndex = intermediateIndex;
  pt.m_isMyPosition = static_cast<bool>(point.isMyPosition);
  GetFramework().GetRoutingManager().AddRoutePoint(std::move(pt));
  [self rebuildWithBestRouter:NO];
}

- (void)removeIntermediatePointAndRebuild:(int)intermediateIndex
{
  [self removeRoutePoint:RouteMarkType::Intermediate intermediateIndex:intermediateIndex];
  [self rebuildWithBestRouter:NO];
}

- (void)buildFromPoint:(MWMRoutePoint *)startPoint bestRouter:(BOOL)bestRouter
{
  self.startPoint = startPoint;
  [self applyRoutePoints];
  [self rebuildWithBestRouter:bestRouter];
}

- (void)buildToPoint:(MWMRoutePoint *)finishPoint bestRouter:(BOOL)bestRouter
{
  self.finishPoint = finishPoint;
  [self applyRoutePoints];
  [self rebuildWithBestRouter:bestRouter];
}

- (void)buildFromPoint:(MWMRoutePoint *)start
               toPoint:(MWMRoutePoint *)finish
            bestRouter:(BOOL)bestRouter
{
  self.startPoint = start;
  self.finishPoint = finish;
  [self applyRoutePoints];
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
  }
  else if (self.finishPoint.isMyPosition)
  {
    [Statistics logEvent:kStatPointToPoint
          withParameters:@{kStatAction : kStatBuildRoute, kStatValue : kStatToMyPosition}];
  }
  else
  {
    [Statistics logEvent:kStatPointToPoint
          withParameters:@{kStatAction : kStatBuildRoute, kStatValue : kStatPointToPoint}];
    isP2P = true;
  }

  MWMMapViewControlsManager * mapViewControlsManager = [MWMMapViewControlsManager manager];
  [mapViewControlsManager onRoutePrepare];

  auto & rm = GetFramework().GetRoutingManager();
  auto points = rm.GetRoutePoints();
  if (points.size() < 2 || ![self arePointsValidForRouting])
  {
    rm.CloseRouting(false /* remove route points */);
    return;
  }
  
  // Taxi can't be used as best router.
  if (bestRouter && ![[self class] isTaxi])
    self.type = routerType(rm.GetBestRouter(points.front().m_position, points.back().m_position));

  rm.BuildRoute(0 /* timeoutSec */);
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
      GetFramework().GetRoutingManager().FollowRoute();
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
  [self doStop:YES];
  [[MWMMapViewControlsManager manager] onRouteStop];
}

- (void)doStop:(BOOL)removeRoutePoints
{
  // Don't save taxi routing type as default.
  if ([[self class] isTaxi])
    GetFramework().GetRoutingManager().SetRouter(routing::RouterType::Vehicle);

  [self clearAltitudeImagesData];
  GetFramework().GetRoutingManager().CloseRouting(removeRoutePoints);
  MapsAppDelegate * app = [MapsAppDelegate theApp];
  app.routingPlaneMode = MWMRoutingPlaneModeNone;
  [MWMRouterSavedState remove];
  [MWMThemeManager setAutoUpdates:NO];
  [app showAlertIfRequired];
}

- (void)updateFollowingInfo
{
  auto const & f = GetFramework();
  if (!f.GetRoutingManager().IsRoutingActive())
    return;
  location::FollowingInfo info;
  f.GetRoutingManager().GetRouteFollowingInfo(info);
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
      if (!GetFramework().GetRoutingManager().GenerateRouteAltitudeChart(
              width, height, imageRGBAData, minRouteAltitude, maxRouteAltitude, units))
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
  auto const & routingManager = GetFramework().GetRoutingManager();
  if (routingManager.IsRoutingActive())
  {
    MWMTextToSpeech * tts = [MWMTextToSpeech tts];
    if (routingManager.IsOnRoute() && tts.active)
      [tts playTurnNotifications];

    [self updateFollowingInfo];
  }
  else
  {
    MWMRouterSavedState * state = [MWMRouterSavedState state];
    if (state.forceStateChange == MWMRouterForceStateChange::Rebuild)
    {
      state.forceStateChange = MWMRouterForceStateChange::Start;
      self.type = routerType(GetFramework().GetRoutingManager().GetLastUsedRouter());
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
}

- (void)setFinishPoint:(MWMRoutePoint *)finishPoint
{
  if (_finishPoint == finishPoint)
    return;
  _finishPoint = finishPoint;
  if (finishPoint == self.startPoint)
    self.startPoint = zeroRoutePoint();
}

@end
