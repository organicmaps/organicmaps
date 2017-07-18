#import "MWMRouter.h"
#import <Crashlytics/Crashlytics.h>
#import <Pushwoosh/PushNotificationManager.h>
#import "CLLocation+Mercator.h"
#import "MWMAlertViewController.h"
#import "MWMConsts.h"
#import "MWMCoreRouterType.h"
#import "MWMFrameworkListener.h"
#import "MWMLocationHelpers.h"
#import "MWMLocationManager.h"
#import "MWMLocationObserver.h"
#import "MWMMapViewControlsManager.h"
#import "MWMNavigationDashboardManager.h"
#import "MWMRoutePoint+CPP.h"
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

#include "map/routing_manager.hpp"

using namespace routing;

namespace
{
char const * kRenderAltitudeImagesQueueLabel = "mapsme.mwmrouter.renderAltitudeImagesQueue";
}  // namespace

@interface MWMRouter ()<MWMLocationObserver, MWMFrameworkRouteBuilderObserver>

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
    [[UIApplication sharedApplication] openURL:url];
  }];
}

+ (void)stopRouting
{
  [self stop];
  [MWMNavigationDashboardManager manager].taxiDataSource = nil;
}

+ (BOOL)isRoutingActive { return GetFramework().GetRoutingManager().IsRoutingActive(); }
+ (BOOL)isRouteBuilt { return GetFramework().GetRoutingManager().IsRouteBuilt(); }
+ (BOOL)isRouteFinished { return GetFramework().GetRoutingManager().IsRouteFinished(); }
+ (BOOL)isRouteRebuildingOnly { return GetFramework().GetRoutingManager().IsRouteRebuildingOnly(); }
+ (BOOL)isOnRoute { return GetFramework().GetRoutingManager().IsOnRoute(); }
+ (NSArray<MWMRoutePoint *> *)points
{
  NSMutableArray<MWMRoutePoint *> * points = [@[] mutableCopy];
  auto const routePoints = GetFramework().GetRoutingManager().GetRoutePoints();
  for (auto const & routePoint : routePoints)
    [points addObject:[[MWMRoutePoint alloc] initWithRouteMarkData:routePoint]];
  return [points copy];
}

+ (NSInteger)pointsCount { return GetFramework().GetRoutingManager().GetRoutePoints().size(); }
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
    [MWMLocationManager addObserver:self];
    [MWMFrameworkListener addObserver:self];
  }
  return self;
}

+ (void)setType:(MWMRouterType)type
{
  if (type == self.type)
    return;
  // Now only car routing supports intermediate points.
  if (type != MWMRouterTypeVehicle)
    GetFramework().GetRoutingManager().RemoveIntermediateRoutePoints();
  [self doStop:NO];
  GetFramework().GetRoutingManager().SetRouter(coreRouterType(type));
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
  GetFramework().GetRoutingManager().GenerateTurnNotifications(notifications);
  for (auto const & text : notifications)
    [turnNotifications addObject:@(text.c_str())];
  return [turnNotifications copy];
}

+ (void)removePoint:(RouteMarkType)type intermediateIndex:(int8_t)intermediateIndex
{
  GetFramework().GetRoutingManager().RemoveRoutePoint(type, intermediateIndex);
  [[MWMMapViewControlsManager manager] onRoutePointsUpdated];
}

+ (void)addPoint:(MWMRoutePoint *)point intermediateIndex:(int8_t)intermediateIndex
{
  RouteMarkData pt = point.routeMarkData;
  pt.m_intermediateIndex = intermediateIndex;
  GetFramework().GetRoutingManager().AddRoutePoint(std::move(pt));
  [[MWMMapViewControlsManager manager] onRoutePointsUpdated];
}

+ (void)addPoint:(MWMRoutePoint *)point
{
  if (!point)
  {
    NSAssert(NO, @"Point can not be nil");
    return;
  }
  RouteMarkData pt = point.routeMarkData;
  GetFramework().GetRoutingManager().AddRoutePoint(std::move(pt));
  [[MWMMapViewControlsManager manager] onRoutePointsUpdated];
}

+ (void)removeStartPointAndRebuild:(int8_t)intermediateIndex
{
  [self removePoint:RouteMarkType::Start intermediateIndex:intermediateIndex];
  [self rebuildWithBestRouter:NO];
}

+ (void)removeFinishPointAndRebuild:(int8_t)intermediateIndex
{
  [self removePoint:RouteMarkType::Finish intermediateIndex:intermediateIndex];
  [self rebuildWithBestRouter:NO];
}

+ (void)addIntermediatePointAndRebuild:(MWMRoutePoint *)point
                     intermediateIndex:(int8_t)intermediateIndex
{
  if (!point)
    return;
  [self addPoint:point intermediateIndex:intermediateIndex];
  [self rebuildWithBestRouter:NO];
}

+ (void)removeIntermediatePointAndRebuild:(int8_t)intermediateIndex
{
  [self removePoint:RouteMarkType::Intermediate intermediateIndex:intermediateIndex];
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
  if (![self startPoint] && [MWMLocationManager lastLocation])
    [self addPoint:[[MWMRoutePoint alloc] initWithLastLocationAndType:MWMRoutePointTypeStart]];
  [self rebuildWithBestRouter:bestRouter];
}

+ (void)buildFromPoint:(MWMRoutePoint *)startPoint
               toPoint:(MWMRoutePoint *)finishPoint
            bestRouter:(BOOL)bestRouter
{
  if (!startPoint || !finishPoint)
    return;

  [self addPoint:startPoint];
  [self addPoint:finishPoint];

  [self rebuildWithBestRouter:bestRouter];
}

+ (void)rebuildWithBestRouter:(BOOL)bestRouter
{
  [self clearAltitudeImagesData];

  auto & rm = GetFramework().GetRoutingManager();
  auto points = rm.GetRoutePoints();
  if (points.size() == 2)
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
                                   initWithLastLocationAndType:MWMRoutePointTypeStart]
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

+ (void)stop
{
  [Statistics logEvent:kStatEventName(kStatPointToPoint, kStatClose)];
  [MWMSearch clear];
  [self doStop:YES];
  [[MWMMapViewControlsManager manager] onRouteStop];
}

+ (void)doStop:(BOOL)removeRoutePoints
{
  // Don't save taxi routing type as default.
  if ([[self class] isTaxi])
    GetFramework().GetRoutingManager().SetRouter(routing::RouterType::Vehicle);

  [self clearAltitudeImagesData];
  GetFramework().GetRoutingManager().CloseRouting(removeRoutePoints);
  [MWMThemeManager setAutoUpdates:NO];
  [MapsAppDelegate.theApp showAlertIfRequired];
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

+ (void)routeAltitudeImageForSize:(CGSize)size completion:(MWMImageHeightBlock)block
{
  auto router = self.router;
  dispatch_async(router.renderAltitudeImagesQueue, ^{
    auto router = self.router;
    if (![self hasRouteAltitude])
      return;
    CGFloat const screenScale = [UIScreen mainScreen].scale;
    CGSize const scaledSize = {.width = size.width * screenScale,
                               .height = size.height * screenScale};
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
      if (!GetFramework().GetRoutingManager().GenerateRouteAltitudeChart(
              width, height, imageRGBAData, minRouteAltitude, maxRouteAltitude, units))
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
  auto const & routingManager = GetFramework().GetRoutingManager();
  if (!routingManager.IsRoutingActive())
    return;
  auto tts = [MWMTextToSpeech tts];
  if (routingManager.IsOnRoute() && tts.active)
    [tts playTurnNotifications];

  [self updateFollowingInfo];
}

#pragma mark - MWMFrameworkRouteBuilderObserver

- (void)processRouteBuilderEvent:(routing::IRouter::ResultCode)code
                       countries:(storage::TCountriesVec const &)absentCountries
{
  MWMMapViewControlsManager * mapViewControlsManager = [MWMMapViewControlsManager manager];
  switch (code)
  {
  case routing::IRouter::ResultCode::NoError:
  {
    auto & f = GetFramework();
    f.DeactivateMapSelection(true);
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
  case routing::IRouter::Cancelled:
    [mapViewControlsManager onRoutePrepare];
    break;
  case routing::IRouter::StartPointNotFound:
  case routing::IRouter::EndPointNotFound:
  case routing::IRouter::NoCurrentPosition:
  case routing::IRouter::PointsInDifferentMWM:
  case routing::IRouter::InternalError:
  case routing::IRouter::IntermediatePointNotFound: 
    [[MWMAlertViewController activeAlertController] presentAlert:code];
    [mapViewControlsManager onRouteError];
    break;
  }
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
            [MWMRouter stop];
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

@end
