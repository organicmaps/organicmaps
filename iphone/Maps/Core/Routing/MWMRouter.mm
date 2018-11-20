#import "MWMRouter.h"
#import <Crashlytics/Crashlytics.h>
#import "MWMAlertViewController+CPP.h"
#import "MWMCoreRouterType.h"
#import "MWMFrameworkListener.h"
#import "MWMLocationHelpers.h"
#import "MWMLocationManager.h"
#import "MWMLocationObserver.h"
#import "MWMMapViewControlsManager.h"
#import "MWMNavigationDashboardManager+Entity.h"
#import "MWMRoutePoint+CPP.h"
#import "MWMRouterRecommendation.h"
#import "MWMStorage.h"
#import "MapViewController.h"
#import "MapsAppDelegate.h"
#import "Statistics.h"
#import "SwiftBridge.h"
#import "UIImage+RGBAData.h"

#include "Framework.h"

#include "indexer/feature_altitude.hpp"

#include "platform/local_country_file_utils.hpp"

#include <cstdint>
#include <memory>
#include <vector>

using namespace routing;

@interface MWMRouter ()<MWMLocationObserver, MWMFrameworkRouteBuilderObserver>

@property(nonatomic) NSMutableDictionary<NSValue *, NSData *> * altitudeImagesData;
@property(nonatomic) NSString * altitudeElevation;
@property(nonatomic) dispatch_queue_t renderAltitudeImagesQueue;
@property(nonatomic) uint32_t taxiRoutePointTransactionId;
@property(nonatomic) uint32_t routeManagerTransactionId;
@property(nonatomic) BOOL canAutoAddLastLocation;
@property(nonatomic) BOOL isAPICall;

+ (MWMRouter *)router;

@end

namespace
{
char const * kRenderAltitudeImagesQueueLabel = "mapsme.mwmrouter.renderAltitudeImagesQueue";

void logPointEvent(MWMRoutePoint * point, NSString * eventType)
{
  if (point == nullptr)
    return;

  NSString * pointTypeStr = nil;
  switch (point.type)
  {
  case MWMRoutePointTypeStart: pointTypeStr = kStatRoutingPointTypeStart; break;
  case MWMRoutePointTypeIntermediate: pointTypeStr = kStatRoutingPointTypeIntermediate; break;
  case MWMRoutePointTypeFinish: pointTypeStr = kStatRoutingPointTypeFinish; break;
  }
  NSString * method = nil;
  if ([MWMRouter router].isAPICall)
    method = kStatRoutingPointMethodApi;
  else if ([MWMNavigationDashboardManager manager].state != MWMNavigationDashboardStateHidden)
    method = kStatRoutingPointMethodPlanning;
  else
    method = kStatRoutingPointMethodNoPlanning;
  [Statistics logEvent:eventType
        withParameters:@{
          kStatRoutingPointType: pointTypeStr,
          kStatRoutingPointValue:
              (point.isMyPosition ? kStatRoutingPointValueMyPosition : kStatRoutingPointValuePoint),
          kStatRoutingPointMethod: method,
          kStatMode: ([MWMRouter isOnRoute] ? kStatRoutingModeOnRoute : kStatRoutingModePlanning)
        }];
}
}  // namespace

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

+ (BOOL)hasRouteAltitude
{
  switch ([self type])
  {
  case MWMRouterTypeVehicle:
  case MWMRouterTypePublicTransport:
  case MWMRouterTypeTaxi: return NO;
  case MWMRouterTypePedestrian:
  case MWMRouterTypeBicycle: return GetFramework().GetRoutingManager().HasRouteAltitude();
  }
}

+ (BOOL)isTaxi
{
  return GetFramework().GetRoutingManager().GetRouter() == routing::RouterType::Taxi;
}
+ (void)startRouting
{
  if (![self isTaxi])
  {
    [self start];
    return;
  }

  auto taxiDataSource = [MWMNavigationDashboardManager manager].taxiDataSource;
  auto & rm = GetFramework().GetRoutingManager();
  auto const routePoints = rm.GetRoutePoints();
  if (routePoints.size() >= 2)
  {
    auto eventName =
        taxiDataSource.isTaxiInstalled ? kStatRoutingTaxiOrder : kStatRoutingTaxiInstall;
    auto p1 = [[MWMRoutePoint alloc] initWithRouteMarkData:routePoints.front()];
    auto p2 = [[MWMRoutePoint alloc] initWithRouteMarkData:routePoints.back()];

    NSString * provider = nil;
    switch (taxiDataSource.type)
    {
    case MWMRoutePreviewTaxiCellTypeTaxi: provider = kStatUnknown; break;
    case MWMRoutePreviewTaxiCellTypeUber: provider = kStatUber; break;
    case MWMRoutePreviewTaxiCellTypeYandex: provider = kStatYandex; break;
    case MWMRoutePreviewTaxiCellTypeMaxim: provider = kStatMaxim; break;
    case MWMRoutePreviewTaxiCellTypeRutaxi: provider = kStatRutaxi; break;
    }

    [Statistics logEvent:eventName
          withParameters:@{
            kStatProvider : provider,
            kStatFromLocation : makeLocationEventValue(p1.latitude, p1.longitude),
            kStatToLocation : makeLocationEventValue(p2.latitude, p2.longitude)
          }
              atLocation:[MWMLocationManager lastLocation]];
  }
  else
  {
    auto err = [[NSError alloc] initWithDomain:kMapsmeErrorDomain
                                          code:5
                                      userInfo:@{
                                        @"Description" : @"Invalid number of taxi route points",
                                        @"Count" : @(routePoints.size())
                                      }];
    [[Crashlytics sharedInstance] recordError:err];
  }

  [taxiDataSource taxiURL:^(NSURL * url) {
    [UIApplication.sharedApplication openURL:url];
  }];
}

+ (void)stopRouting
{
  auto type = GetFramework().GetRoutingManager().GetRouter();
  [Statistics logEvent:kStatRoutingRouteFinish withParameters:@{
                                                                kStatMode : @(routing::ToString(type).c_str()),
                                                                kStatRoutingInterrupted : @([self isRouteFinished])
                                                                }];
  [self stop:YES];
}

+ (BOOL)isRoutingActive { return GetFramework().GetRoutingManager().IsRoutingActive(); }
+ (BOOL)isRouteBuilt { return GetFramework().GetRoutingManager().IsRouteBuilt(); }
+ (BOOL)isRouteFinished { return GetFramework().GetRoutingManager().IsRouteFinished(); }
+ (BOOL)isRouteRebuildingOnly { return GetFramework().GetRoutingManager().IsRouteRebuildingOnly(); }
+ (BOOL)isOnRoute { return GetFramework().GetRoutingManager().IsRoutingFollowing(); }
+ (NSArray<MWMRoutePoint *> *)points
{
  NSMutableArray<MWMRoutePoint *> * points = [@[] mutableCopy];
  auto const routePoints = GetFramework().GetRoutingManager().GetRoutePoints();
  for (auto const & routePoint : routePoints)
    [points addObject:[[MWMRoutePoint alloc] initWithRouteMarkData:routePoint]];
  return [points copy];
}

+ (NSInteger)pointsCount { return GetFramework().GetRoutingManager().GetRoutePointsCount(); }
+ (MWMRoutePoint *)startPoint
{
  auto const routePoints = GetFramework().GetRoutingManager().GetRoutePoints();
  if (routePoints.empty())
    return nil;
  auto const & routePoint = routePoints.front();
  if (routePoint.m_pointType == RouteMarkType::Start)
    return [[MWMRoutePoint alloc] initWithRouteMarkData:routePoint];
  return nil;
}

+ (MWMRoutePoint *)finishPoint
{
  auto const routePoints = GetFramework().GetRoutingManager().GetRoutePoints();
  if (routePoints.empty())
    return nil;
  auto const & routePoint = routePoints.back();
  if (routePoint.m_pointType == RouteMarkType::Finish)
    return [[MWMRoutePoint alloc] initWithRouteMarkData:routePoint];
  return nil;
}

+ (void)enableAutoAddLastLocation:(BOOL)enable
{
  [MWMRouter router].canAutoAddLastLocation = enable;
}

+ (BOOL)canAddIntermediatePoint
{
  return GetFramework().GetRoutingManager().CouldAddIntermediatePoint() && ![MWMRouter isTaxi];
}

- (instancetype)initRouter
{
  self = [super init];
  if (self)
  {
    self.altitudeImagesData = [@{} mutableCopy];
    self.renderAltitudeImagesQueue =
        dispatch_queue_create(kRenderAltitudeImagesQueueLabel, DISPATCH_QUEUE_SERIAL);
    self.taxiRoutePointTransactionId = RoutingManager::InvalidRoutePointsTransactionId();
    self.routeManagerTransactionId = RoutingManager::InvalidRoutePointsTransactionId();
    [MWMLocationManager addObserver:self];
    [MWMFrameworkListener addObserver:self];
    _canAutoAddLastLocation = YES;
  }
  return self;
}

+ (void)setType:(MWMRouterType)type
{
  if (type == self.type)
    return;
  
  if (type == MWMRouterTypeTaxi)
    [self openTaxiTransaction];
  else
    [self cancelTaxiTransaction];

  [self doStop:NO];
  GetFramework().GetRoutingManager().SetRouter(coreRouterType(type));
}

+ (void)openTaxiTransaction
{
  auto & rm = GetFramework().GetRoutingManager();
  auto router = [MWMRouter router];
  router.taxiRoutePointTransactionId = rm.OpenRoutePointsTransaction();
  rm.RemoveIntermediateRoutePoints();
}

+ (void)cancelTaxiTransaction
{
  auto router = [MWMRouter router];
  if (router.taxiRoutePointTransactionId != RoutingManager::InvalidRoutePointsTransactionId())
  {
    GetFramework().GetRoutingManager().CancelRoutePointsTransaction(router.taxiRoutePointTransactionId);
    router.taxiRoutePointTransactionId = RoutingManager::InvalidRoutePointsTransactionId();
  }
}

+ (void)applyTaxiTransaction
{
  // We have to apply taxi transaction each time we add/remove points after switch to taxi mode.
  auto router = [MWMRouter router];
  if (router.taxiRoutePointTransactionId != RoutingManager::InvalidRoutePointsTransactionId())
  {
    GetFramework().GetRoutingManager().ApplyRoutePointsTransaction(router.taxiRoutePointTransactionId);
    router.taxiRoutePointTransactionId = RoutingManager::InvalidRoutePointsTransactionId();
  }
}

+ (MWMRouterType)type { return routerType(GetFramework().GetRoutingManager().GetRouter()); }
+ (void)disableFollowMode { GetFramework().GetRoutingManager().DisableFollowMode(); }
+ (void)enableTurnNotifications:(BOOL)active
{
  GetFramework().GetRoutingManager().EnableTurnNotifications(active);
}

+ (BOOL)areTurnNotificationsEnabled
{
  return GetFramework().GetRoutingManager().AreTurnNotificationsEnabled();
}

+ (void)setTurnNotificationsLocale:(NSString *)locale
{
  GetFramework().GetRoutingManager().SetTurnNotificationsLocale(locale.UTF8String);
}

+ (NSArray<NSString *> *)turnNotifications
{
  NSMutableArray<NSString *> * turnNotifications = [@[] mutableCopy];
  vector<string> notifications;
  GetFramework().GetRoutingManager().GenerateNotifications(notifications);

  for (auto const & text : notifications)
    [turnNotifications addObject:@(text.c_str())];
  return [turnNotifications copy];
}

+ (void)removePoint:(MWMRoutePoint *)point
{
  logPointEvent(point, kStatRoutingRemovePoint);
  [self applyTaxiTransaction];
  RouteMarkData pt = point.routeMarkData;
  GetFramework().GetRoutingManager().RemoveRoutePoint(pt.m_pointType, pt.m_intermediateIndex);
  [[MWMNavigationDashboardManager manager] onRoutePointsUpdated];
}

+ (void)removePointAndRebuild:(MWMRoutePoint *)point
{
  if (!point)
    return;
  [self removePoint:point];
  [self rebuildWithBestRouter:NO];
}

+ (void)removePoints { GetFramework().GetRoutingManager().RemoveRoutePoints(); }
+ (void)addPoint:(MWMRoutePoint *)point
{
  if (!point)
  {
    NSAssert(NO, @"Point can not be nil");
    return;
  }
  logPointEvent(point, kStatRoutingAddPoint);
  [self applyTaxiTransaction];
  RouteMarkData pt = point.routeMarkData;
  GetFramework().GetRoutingManager().AddRoutePoint(std::move(pt));
  [[MWMNavigationDashboardManager manager] onRoutePointsUpdated];
}

+ (void)addPointAndRebuild:(MWMRoutePoint *)point
{
  if (!point)
    return;
  [self addPoint:point];
  [self rebuildWithBestRouter:NO];
}

+ (void)buildFromPoint:(MWMRoutePoint *)startPoint bestRouter:(BOOL)bestRouter
{
  if (!startPoint)
    return;
  [self addPoint:startPoint];
  [self rebuildWithBestRouter:bestRouter];
}

+ (void)buildToPoint:(MWMRoutePoint *)finishPoint bestRouter:(BOOL)bestRouter
{
  if (!finishPoint)
    return;
  [self addPoint:finishPoint];
  if (![self startPoint] && [MWMLocationManager lastLocation] &&
      [MWMRouter router].canAutoAddLastLocation)
  {
    [self addPoint:[[MWMRoutePoint alloc] initWithLastLocationAndType:MWMRoutePointTypeStart
                                                    intermediateIndex:0]];
  }
  if ([self startPoint] && [self finishPoint])
    [self rebuildWithBestRouter:bestRouter];
}

+ (void)buildApiRouteWithType:(MWMRouterType)type
                   startPoint:(MWMRoutePoint *)startPoint
                  finishPoint:(MWMRoutePoint *)finishPoint
{
  if (!startPoint || !finishPoint)
    return;

  [MWMRouter setType:type];

  auto router = [MWMRouter router];
  router.isAPICall = YES;
  [self addPoint:startPoint];
  [self addPoint:finishPoint];
  router.isAPICall = NO;

  [self rebuildWithBestRouter:NO];
}

+ (void)rebuildWithBestRouter:(BOOL)bestRouter
{
  [self clearAltitudeImagesData];

  auto & rm = GetFramework().GetRoutingManager();
  auto const & points = rm.GetRoutePoints();
  auto const pointsCount = points.size();
  if (pointsCount < 2)
  {
    [self doStop:NO];
    [[MWMMapViewControlsManager manager] onRoutePrepare];
    return;
  }
  if (pointsCount == 2)
  {
    if (points.front().m_isMyPosition)
    {
      [Statistics logEvent:kStatPointToPoint
            withParameters:@{kStatAction : kStatBuildRoute, kStatValue : kStatFromMyPosition}];
    }
    else if (points.back().m_isMyPosition)
    {
      [Statistics logEvent:kStatPointToPoint
            withParameters:@{kStatAction : kStatBuildRoute, kStatValue : kStatToMyPosition}];
    }
    else
    {
      [Statistics logEvent:kStatPointToPoint
            withParameters:@{kStatAction : kStatBuildRoute, kStatValue : kStatPointToPoint}];
    }
  }

  // Taxi can't be used as best router.
  if (bestRouter && ![[self class] isTaxi])
    self.type = routerType(rm.GetBestRouter(points.front().m_position, points.back().m_position));

  [[MWMMapViewControlsManager manager] onRouteRebuild];
  rm.BuildRoute(0 /* timeoutSec */);
}

+ (void)start
{
  [self saveRoute];
  auto const doStart = ^{
    auto & rm = GetFramework().GetRoutingManager();
    auto const routePoints = rm.GetRoutePoints();
    if (routePoints.size() >= 2)
    {
      auto p1 = [[MWMRoutePoint alloc] initWithRouteMarkData:routePoints.front()];
      auto p2 = [[MWMRoutePoint alloc] initWithRouteMarkData:routePoints.back()];

      if (p1.isMyPosition)
        [Statistics logEvent:kStatEventName(kStatPointToPoint, kStatGo)
              withParameters:@{kStatValue : kStatFromMyPosition}];
      else if (p2.isMyPosition)
        [Statistics logEvent:kStatEventName(kStatPointToPoint, kStatGo)
              withParameters:@{kStatValue : kStatToMyPosition}];
      else
        [Statistics logEvent:kStatEventName(kStatPointToPoint, kStatGo)
              withParameters:@{kStatValue : kStatPointToPoint}];

      auto type = GetFramework().GetRoutingManager().GetRouter();
      [Statistics logEvent:kStatRoutingRouteStart
            withParameters:@{
                             kStatMode : @(routing::ToString(type).c_str()),
                             kStatTraffic : @(GetFramework().GetTrafficManager().IsEnabled())
                             }];

      if (p1.isMyPosition && [MWMLocationManager lastLocation])
      {
        rm.FollowRoute();
        [[MWMMapViewControlsManager manager] onRouteStart];
        [MWMThemeManager setAutoUpdates:YES];
      }
      else
      {
        MWMAlertViewController * alertController = [MWMAlertViewController activeAlertController];
        CLLocation * lastLocation = [MWMLocationManager lastLocation];
        BOOL const needToRebuild = lastLocation &&
                                   !location_helpers::isMyPositionPendingOrNoPosition() &&
                                   !p2.isMyPosition;
        [alertController presentPoint2PointAlertWithOkBlock:^{
          [self buildFromPoint:[[MWMRoutePoint alloc]
                                   initWithLastLocationAndType:MWMRoutePointTypeStart
                                             intermediateIndex:0]
                    bestRouter:NO];
        }
                                              needToRebuild:needToRebuild];
      }
    }
    else
    {
      auto err = [[NSError alloc] initWithDomain:kMapsmeErrorDomain
                                            code:5
                                        userInfo:@{
                                          @"Description" : @"Invalid number of route points",
                                          @"Count" : @(routePoints.size())
                                        }];
      [[Crashlytics sharedInstance] recordError:err];
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

+ (void)stop:(BOOL)removeRoutePoints
{
  [self doStop:removeRoutePoints];
  // Don't save taxi routing type as default.
  if ([MWMRouter isTaxi])
    GetFramework().GetRoutingManager().SetRouter(routing::RouterType::Vehicle);
  [[MWMMapViewControlsManager manager] onRouteStop];
  [MWMRouter router].canAutoAddLastLocation = YES;
}

+ (void)doStop:(BOOL)removeRoutePoints
{
  [self clearAltitudeImagesData];
  GetFramework().GetRoutingManager().CloseRouting(removeRoutePoints);
  if (removeRoutePoints)
    GetFramework().GetRoutingManager().DeleteSavedRoutePoints();
  [MWMThemeManager setAutoUpdates:NO];
  [[MapsAppDelegate theApp] showAlertIfRequired];
}

- (void)updateFollowingInfo
{
  if (![MWMRouter isRoutingActive])
    return;
  auto const & rm = GetFramework().GetRoutingManager();
  location::FollowingInfo info;
  rm.GetRouteFollowingInfo(info);
  auto navManager = [MWMNavigationDashboardManager manager];
  if (!info.IsValid())
    return;
  if ([MWMRouter type] == MWMRouterTypePublicTransport)
    [navManager updateTransitInfo:rm.GetTransitRouteInfo()];
  else
    [navManager updateFollowingInfo:info];
}

+ (void)routeAltitudeImageForSize:(CGSize)size completion:(MWMImageHeightBlock)block
{
  if (![self hasRouteAltitude])
    return;

  auto routePointDistanceM = std::make_shared<std::vector<double>>(std::vector<double>());
  auto altitudes = std::make_shared<feature::TAltitudes>(feature::TAltitudes());
  if (!GetFramework().GetRoutingManager().GetRouteAltitudesAndDistancesM(*routePointDistanceM, *altitudes))
    return;

  // Note. |routePointDistanceM| and |altitudes| should not be used in the method after line below.
  dispatch_async(self.router.renderAltitudeImagesQueue, [=] () {
    auto router = self.router;
    CGFloat const screenScale = [UIScreen mainScreen].scale;
    CGSize const scaledSize = {size.width * screenScale, size.height * screenScale};
    CHECK_GREATER_OR_EQUAL(scaledSize.width, 0.0, ());
    CHECK_GREATER_OR_EQUAL(scaledSize.height, 0.0, ());
    uint32_t const width = static_cast<uint32_t>(scaledSize.width);
    uint32_t const height = static_cast<uint32_t>(scaledSize.height);
    if (width == 0 || height == 0)
      return;

    NSValue * sizeValue = [NSValue valueWithCGSize:scaledSize];
    NSData * imageData = router.altitudeImagesData[sizeValue];
    if (!imageData)
    {
      vector<uint8_t> imageRGBAData;
      int32_t minRouteAltitude = 0;
      int32_t maxRouteAltitude = 0;
      measurement_utils::Units units = measurement_utils::Units::Metric;
 
      if(!GetFramework().GetRoutingManager().GenerateRouteAltitudeChart(width, height,
                                                                        *altitudes,
                                                                        *routePointDistanceM,
                                                                        imageRGBAData,
                                                                        minRouteAltitude,
                                                                        maxRouteAltitude, units))
      {
        return;
      }

      if (imageRGBAData.empty())
        return;
      imageData = [NSData dataWithBytes:imageRGBAData.data() length:imageRGBAData.size()];
      router.altitudeImagesData[sizeValue] = imageData;

      string heightString;
      measurement_utils::FormatDistance(maxRouteAltitude - minRouteAltitude, heightString);
      router.altitudeElevation = @(heightString.c_str());
    }

    dispatch_async(dispatch_get_main_queue(), ^{
      UIImage * altitudeImage = [UIImage imageWithRGBAData:imageData width:width height:height];
      if (altitudeImage)
        block(altitudeImage, router.altitudeElevation);
    });
  });
}

+ (void)clearAltitudeImagesData
{
  auto router = self.router;
  dispatch_async(router.renderAltitudeImagesQueue, ^{
    [router.altitudeImagesData removeAllObjects];
    router.altitudeElevation = nil;
  });
}

#pragma mark - MWMLocationObserver

- (void)onLocationUpdate:(location::GpsInfo const &)info
{
  if (![MWMRouter isRoutingActive])
    return;
  auto tts = [MWMTextToSpeech tts];
  if ([MWMRouter isOnRoute] && tts.active)
  {
    [tts playTurnNotifications];
    [tts playWarningSound];
  }

  [self updateFollowingInfo];
}

#pragma mark - MWMFrameworkRouteBuilderObserver

- (void)processRouteBuilderEvent:(routing::RouterResultCode)code
                       countries:(storage::TCountriesVec const &)absentCountries
{
  MWMMapViewControlsManager * mapViewControlsManager = [MWMMapViewControlsManager manager];
  switch (code)
  {
  case routing::RouterResultCode::NoError:
  {
    GetFramework().DeactivateMapSelection(true);

    auto startPoint = [MWMRouter startPoint];
    if (!startPoint || !startPoint.isMyPosition)
    {
      dispatch_async(dispatch_get_main_queue(), ^{
        [MWMRouter disableFollowMode];
      });
    }

    [mapViewControlsManager onRouteReady];
    [self updateFollowingInfo];
    break;
  }
  case routing::RouterResultCode::RouteFileNotExist:
  case routing::RouterResultCode::InconsistentMWMandRoute:
  case routing::RouterResultCode::NeedMoreMaps:
  case routing::RouterResultCode::FileTooOld:
  case routing::RouterResultCode::RouteNotFound:
    if ([MWMRouter isTaxi])
      return;
    [self presentDownloaderAlert:code countries:absentCountries];
    [[MWMNavigationDashboardManager manager] onRouteError:L(@"routing_planning_error")];
    break;
  case routing::RouterResultCode::Cancelled:
    [mapViewControlsManager onRoutePrepare];
    break;
  case routing::RouterResultCode::StartPointNotFound:
  case routing::RouterResultCode::EndPointNotFound:
  case routing::RouterResultCode::NoCurrentPosition:
  case routing::RouterResultCode::PointsInDifferentMWM:
  case routing::RouterResultCode::InternalError:
  case routing::RouterResultCode::IntermediatePointNotFound:
  case routing::RouterResultCode::TransitRouteNotFoundNoNetwork:
  case routing::RouterResultCode::TransitRouteNotFoundTooLongPedestrian:
  case routing::RouterResultCode::RouteNotFoundRedressRouteError:
    if ([MWMRouter isTaxi])
      return;
    [[MWMAlertViewController activeAlertController] presentAlert:code];
    [[MWMNavigationDashboardManager manager] onRouteError:L(@"routing_planning_error")];
    break;
  }
}

- (void)processRouteBuilderProgress:(CGFloat)progress
{
  if (![MWMRouter isTaxi])
    [[MWMNavigationDashboardManager manager] setRouteBuilderProgress:progress];
}

- (void)processRouteRecommendation:(MWMRouterRecommendation)recommendation
{
  switch (recommendation)
  {
  case MWMRouterRecommendationRebuildAfterPointsLoading:
    [MWMRouter
        addPointAndRebuild:[[MWMRoutePoint alloc] initWithLastLocationAndType:MWMRoutePointTypeStart
                                                            intermediateIndex:0]];
    break;
  }
}

#pragma mark - Alerts

- (void)presentDownloaderAlert:(routing::RouterResultCode)code
                     countries:(storage::TCountriesVec const &)countries
{
  MWMAlertViewController * activeAlertController = [MWMAlertViewController activeAlertController];
  if (platform::migrate::NeedMigrate())
  {
    [activeAlertController presentRoutingMigrationAlertWithOkBlock:^{
      [Statistics logEvent:kStatDownloaderMigrationDialogue
            withParameters:@{kStatFrom : kStatRouting}];
      [[MapViewController sharedController] openMigration];
    }];
  }
  else if (!countries.empty())
  {
    [activeAlertController presentDownloaderAlertWithCountries:countries
        code:code
        cancelBlock:^{
          if (code != routing::RouterResultCode::NeedMoreMaps)
            [MWMRouter stopRouting];
        }
        downloadBlock:^(storage::TCountriesVec const & downloadCountries, MWMVoidBlock onSuccess) {
          [MWMStorage downloadNodes:downloadCountries
                          onSuccess:onSuccess];
        }
        downloadCompleteBlock:^{
          [MWMRouter rebuildWithBestRouter:NO];
        }];
  }
  else
  {
    [activeAlertController presentAlert:code];
  }
}

#pragma mark - Save / Load route points

+ (void)saveRoute { GetFramework().GetRoutingManager().SaveRoutePoints(); }

+ (void)saveRouteIfNeeded
{
  if ([self isOnRoute])
    [self saveRoute];
}

+ (void)restoreRouteIfNeeded
{
  if ([MapsAppDelegate theApp].isDrapeEngineCreated)
  {
    auto & rm = GetFramework().GetRoutingManager();
    if ([self isRoutingActive] || ![self hasSavedRoute])
      return;
    rm.LoadRoutePoints([self](bool success)
    {
      if (success)
        [self rebuildWithBestRouter:YES];
    });
  }
  else
  {
    dispatch_async(dispatch_get_main_queue(), ^{
      [self restoreRouteIfNeeded];
    });
  }
}

+ (BOOL)hasSavedRoute
{
  return GetFramework().GetRoutingManager().HasSavedRoutePoints();
}

@end
