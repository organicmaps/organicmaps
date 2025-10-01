#import "MWMRoutingManager.h"
#import "MWMCoreRouterType.h"
#import "MWMCoreUnits.h"
#import "MWMFrameworkListener.h"
#import "MWMFrameworkObservers.h"
#import "MWMLocationManager.h"
#import "MWMLocationObserver.h"
#import "MWMRoutePoint+CPP.h"
#import "RouteInfo+Core.h"
#import "SwiftBridge.h"

#include <CoreApi/Framework.h>

@interface MWMRoutingManager () <MWMFrameworkRouteBuilderObserver, MWMLocationObserver>
@property(nonatomic, readonly) RoutingManager & rm;
@property(strong, nonatomic) NSHashTable<id<MWMRoutingManagerListener>> * listeners;
@end

@implementation MWMRoutingManager

+ (MWMRoutingManager *)routingManager
{
  static MWMRoutingManager * routingManager;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{ routingManager = [[self alloc] initManager]; });
  return routingManager;
}

- (instancetype)initManager
{
  self = [super init];
  if (self)
  {
    self.listeners = [NSHashTable<id<MWMRoutingManagerListener>> weakObjectsHashTable];
    [MWMFrameworkListener addObserver:self];
    [MWMLocationManager addObserver:self];
  }
  return self;
}

- (RoutingManager &)rm
{
  return GetFramework().GetRoutingManager();
}

- (routing::SpeedCameraManager &)scm
{
  return self.rm.GetSpeedCamManager();
}

- (MWMRoutePoint *)startPoint
{
  auto const routePoints = self.rm.GetRoutePoints();
  if (routePoints.empty())
    return nil;
  auto const & routePoint = routePoints.front();
  if (routePoint.m_pointType == RouteMarkType::Start)
    return [[MWMRoutePoint alloc] initWithRouteMarkData:routePoint];
  return nil;
}

- (MWMRoutePoint *)endPoint
{
  auto const routePoints = self.rm.GetRoutePoints();
  if (routePoints.empty())
    return nil;
  auto const & routePoint = routePoints.back();
  if (routePoint.m_pointType == RouteMarkType::Finish)
    return [[MWMRoutePoint alloc] initWithRouteMarkData:routePoint];
  return nil;
}

- (BOOL)isOnRoute
{
  return self.rm.IsRoutingFollowing();
}

- (BOOL)isRoutingActive
{
  return self.rm.IsRoutingActive();
}

- (BOOL)isRouteFinished
{
  return self.rm.IsRouteFinished();
}

- (MWMRouterType)type
{
  return routerType(self.rm.GetRouter());
}

- (void)addListener:(id<MWMRoutingManagerListener>)listener
{
  [self.listeners addObject:listener];
}

- (void)removeListener:(id<MWMRoutingManagerListener>)listener
{
  [self.listeners removeObject:listener];
}

- (void)stopRoutingAndRemoveRoutePoints:(BOOL)flag
{
  self.rm.CloseRouting(flag);
}

- (void)deleteSavedRoutePoints
{
  self.rm.DeleteSavedRoutePoints();
}

- (void)applyRouterType:(MWMRouterType)type
{
  self.rm.SetRouter(coreRouterType(type));
}

- (void)addRoutePoint:(MWMRoutePoint *)point
{
  RouteMarkData startPt = point.routeMarkData;
  self.rm.AddRoutePoint(std::move(startPt));
}

- (void)saveRoute
{
  self.rm.SaveRoutePoints();
}

- (void)buildRouteWithDidFailError:(NSError * __autoreleasing __nullable *)errorPtr
{
  auto const & points = self.rm.GetRoutePoints();
  auto const pointsCount = points.size();

  if (pointsCount > 1)
  {
    self.rm.BuildRoute();
  }
  else if (errorPtr)
  {
    if (pointsCount == 0)
    {
      *errorPtr = [NSError errorWithDomain:@"omaps.app.routing"
                                      code:MWMRouterResultCodeStartPointNotFound
                                  userInfo:nil];
    }
    else
    {
      auto const & routePoint = points.front();
      MWMRouterResultCode code;
      if (routePoint.m_pointType == RouteMarkType::Start)
        code = MWMRouterResultCodeEndPointNotFound;
      else
        code = MWMRouterResultCodeStartPointNotFound;
      *errorPtr = [NSError errorWithDomain:@"omaps.app.routing" code:code userInfo:nil];
    }
  }
}

- (void)startRoute
{
  [self saveRoute];
  self.rm.FollowRoute();
}

- (MWMSpeedCameraManagerMode)speedCameraMode
{
  auto const mode = self.scm.GetMode();
  switch (mode)
  {
  case routing::SpeedCameraManagerMode::Auto: return MWMSpeedCameraManagerModeAuto;
  case routing::SpeedCameraManagerMode::Always: return MWMSpeedCameraManagerModeAlways;
  default: return MWMSpeedCameraManagerModeNever;
  }
}

- (void)setSpeedCameraMode:(MWMSpeedCameraManagerMode)mode
{
  switch (mode)
  {
  case MWMSpeedCameraManagerModeAuto: self.scm.SetMode(routing::SpeedCameraManagerMode::Auto); break;
  case MWMSpeedCameraManagerModeAlways: self.scm.SetMode(routing::SpeedCameraManagerMode::Always); break;
  default: self.scm.SetMode(routing::SpeedCameraManagerMode::Never);
  }
}

- (void)setOnNewTurnCallback:(MWMVoidBlock)callback
{
  self.rm.RoutingSession().SetOnNewTurnCallback([callback] { callback(); });
}

- (void)resetOnNewTurnCallback
{
  self.rm.RoutingSession().SetOnNewTurnCallback(nullptr);
}

- (RouteInfo *)routeInfoForCarPlay:(BOOL)isCarPlay
{
  if (!self.isRoutingActive)
    return nil;
  routing::FollowingInfo info;
  self.rm.GetRouteFollowingInfo(info);
  if (!info.IsValid())
    return nil;
  auto const routeInfo = [[RouteInfo alloc] initWithFollowingInfo:info
                                                      routePoints:[MWMRouter points]
                                                             type:self.type
                                                        isCarPlay:isCarPlay];
  return routeInfo;
}

#pragma mark - MWMFrameworkRouteBuilderObserver implementation

- (void)processRouteBuilderEvent:(routing::RouterResultCode)code
                       countries:(const storage::CountriesSet &)absentCountries
{
  NSArray<id<MWMRoutingManagerListener>> * objects = self.listeners.allObjects;
  MWMRouterResultCode objCCode = MWMRouterResultCode(code);
  NSMutableArray<NSString *> * objCAbsentCountries = [NSMutableArray new];
  std::for_each(absentCountries.begin(), absentCountries.end(), ^(std::string const & str) {
    id nsstr = [NSString stringWithUTF8String:str.c_str()];
    [objCAbsentCountries addObject:nsstr];
  });
  for (id<MWMRoutingManagerListener> object in objects)
    [object processRouteBuilderEventWithCode:objCCode countries:objCAbsentCountries];
}

- (void)speedCameraShowedUpOnRoute:(double)speedLimit
{
  NSArray<id<MWMRoutingManagerListener>> * objects = self.listeners.allObjects;
  for (id<MWMRoutingManagerListener> object in objects)
  {
    if (speedLimit == routing::SpeedCameraOnRoute::kNoSpeedInfo)
    {
      [object updateCameraInfo:YES speedLimitMps:-1];
    }
    else
    {
      auto const metersPerSecond = measurement_utils::KmphToMps(speedLimit);
      [object updateCameraInfo:YES speedLimitMps:metersPerSecond];
    }
  }
}

- (void)speedCameraLeftVisibleArea
{
  NSArray<id<MWMRoutingManagerListener>> * objects = self.listeners.allObjects;
  for (id<MWMRoutingManagerListener> object in objects)
    [object updateCameraInfo:NO speedLimitMps:-1];
}

#pragma mark - MWMLocationObserver implementation

- (void)onLocationUpdate:(CLLocation *)location
{
  NSMutableArray<NSString *> * turnNotifications = [NSMutableArray array];
  std::vector<std::string> notifications;
  auto announceStreets = [NSUserDefaults.standardUserDefaults boolForKey:@"UserDefaultsNeedToEnableStreetNamesTTS"];
  self.rm.GenerateNotifications(notifications, announceStreets);
  for (auto const & text : notifications)
    [turnNotifications addObject:@(text.c_str())];
  NSArray<id<MWMRoutingManagerListener>> * objects = self.listeners.allObjects;
  for (id<MWMRoutingManagerListener> object in objects)
    [object didLocationUpdate:turnNotifications];
}

@end
