#import "MWMRouter.h"
#import "MWMAlertViewController+CPP.h"
#import "MWMCoreRouterType.h"
#import "MWMFrameworkListener.h"
#import "MWMFrameworkObservers.h"
#import "MWMLocationHelpers.h"
#import "MWMLocationObserver.h"
#import "MWMMapViewControlsManager.h"
#import "MWMNavigationDashboardManager+Entity.h"
#import "MWMRoutePoint+CPP.h"
#import "MWMRoutingManager.h"
#import "MWMStorage+UI.h"
#import "MapsAppDelegate.h"
#import "SwiftBridge.h"

#include <CoreApi/ElevationProfileData+Core.h>
#include <CoreApi/Framework.h>
#include <CoreApi/RouteElevationPreviewData.h>
#include <CoreApi/StringUtils+Core.h>
#include <CoreApi/TrackInfo+Core.h>

#include "routing/routing_options.hpp"

#include "kml/type_utils.hpp"
#include "map/routing_mark.hpp"
#include "platform/local_country_file_utils.hpp"
#include "platform/localization.hpp"

using namespace routing;

@interface MWMFollowedTrackInfo ()

- (instancetype)initWithCoreInfo:(FollowedTrackInfo const &)info;

@end

@implementation MWMFollowedTrackInfo

- (instancetype)initWithCoreInfo:(FollowedTrackInfo const &)info
{
  self = [super init];
  if (self)
  {
    _trackId = info.m_id;
    _name = @(info.m_name.c_str());
    _lengthMeters = info.m_lengthM;
    _isReversed = info.m_reversed;
    _finishPoint = [[MWMRoutePoint alloc] initWithPoint:info.m_finish
                                                  title:_name
                                               subtitle:@""
                                                   type:MWMRoutePointTypeFinish
                                      intermediateIndex:0];
  }
  return self;
}

@end

@interface MWMRouter () <MWMLocationObserver, MWMFrameworkRouteBuilderObserver>

@property(nonatomic) uint32_t routeManagerTransactionId;
@property(nonatomic) BOOL canAutoAddLastLocation;
@property(nonatomic) BOOL isAPICall;
@property(nonatomic) BOOL isRestoreProcessCompleted;
@property(strong, nonatomic) MWMRoutingOptions * routingOptions;

+ (MWMRouter *)router;

@end

@implementation MWMRouter

+ (MWMRouter *)router
{
  static MWMRouter * router;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{ router = [[self alloc] initRouter]; });
  return router;
}

+ (BOOL)hasRouteAltitude
{
  return GetFramework().GetRoutingManager().HasRouteAltitude();
}

+ (RouteElevationPreviewData *)routeElevationProfileData
{
  if (![self hasRouteAltitude])
    return nil;

  ElevationInfo elevationInfo;
  if (!GetFramework().GetRoutingManager().GetRouteElevationInfo(elevationInfo))
    return nil;

  auto const altitudesInfo = elevationInfo.CalculateAltitudesInfo(ElevationInfo::kDefThresholdMWM);
  // A zero vertical range collapses the chart's Y-axis transforms, so suppress the misleading flat preview.
  if (altitudesInfo.m_maxAltitude == altitudesInfo.m_minAltitude)
    return nil;

  TrackStatistics trackStatistics;
  trackStatistics.m_ascent = altitudesInfo.GetTotalAscent();
  trackStatistics.m_descent = altitudesInfo.GetTotalDescent();
  trackStatistics.m_maxElevation = altitudesInfo.m_maxAltitude;
  trackStatistics.m_minElevation = altitudesInfo.m_minAltitude;

  TrackInfo * trackInfo = [[TrackInfo alloc] initWithTrackStatistics:trackStatistics];
  ElevationProfileData * profileData = [[ElevationProfileData alloc] initWithTrackId:kml::kInvalidTrackId
                                                                       elevationInfo:elevationInfo];
  return [[RouteElevationPreviewData alloc] initWithTrackInfo:trackInfo elevationInfo:profileData];
}

+ (void)setRouteElevationActivePointDistance:(double)distance
{
  auto & framework = GetFramework();
  if (framework.GetDrapeEngine() == nullptr)
    return;

  auto const point = framework.GetRoutingManager().GetRoutePointAtDistance(distance);
  if (!point)
    return;

  framework.GetDrapeEngine()->SelectObject(df::SelectionShape::ESelectedObject::OBJECT_TRACK, *point, FeatureID(),
                                           false /* isAnim */, false /* isGeometrySelectionAllowed */,
                                           true /* isSelectionShapeVisible */);
}

+ (void)resetRouteElevationActivePoint
{
  auto & framework = GetFramework();
  if (!framework.HasPlacePageInfo())
    framework.DeactivateMapSelectionCircle(false /* restoreViewport */);
}

+ (void)saveRouteAsTrack
{
  GetFramework().SaveRoute();
}

+ (void)startRouting
{
  [self start];
}

+ (void)stopRouting
{
  [self stop:YES];
}

#pragma mark - Track navigation

static MWMFollowTrackResult ToMWMFollowTrackResult(FollowTrackResult result)
{
  switch (result)
  {
  case FollowTrackResult::Ok: return MWMFollowTrackResultOk;
  case FollowTrackResult::NoTrack: return MWMFollowTrackResultNoTrack;
  case FollowTrackResult::DegenerateTrack: return MWMFollowTrackResultDegenerateTrack;
  case FollowTrackResult::UnsupportedProfile: return MWMFollowTrackResultUnsupportedProfile;
  case FollowTrackResult::NotInitialized: return MWMFollowTrackResultNotInitialized;
  case FollowTrackResult::Busy: return MWMFollowTrackResultBusy;
  case FollowTrackResult::InvalidSelection: return MWMFollowTrackResultInvalidSelection;
  case FollowTrackResult::NoRemainingGeometry: return MWMFollowTrackResultNoRemainingGeometry;
  }
}

// An accepted follow transaction has already started the route build: put the dashboard into the
// planning state and let the standard route builder events (ready/error/progress) drive it, as
// +rebuildWithBestRouter: does. A refused one changed nothing, so there is nothing to present.
static MWMFollowTrackResult OnFollowTrackResult(FollowTrackResult result)
{
  if (result == FollowTrackResult::Ok)
  {
    [[MWMCarPlayService shared] preservePhoneTrackSession];
    [[MWMMapViewControlsManager manager] onRouteRebuild];
  }
  return ToMWMFollowTrackResult(result);
}

+ (MWMFollowTrackResult)navigateTrack:(MWMTrackID)trackId
{
  // The core validates before any teardown: it leaves the current routing state untouched when
  // the track is missing or degenerate, and closes the previous session itself on success.
  auto const result = GetFramework().GetRoutingManager().NavigateTrack(trackId, false /* reverse */);
  // Only the UI side of +stopRouting remains for an interrupted navigation: the core teardown
  // has already run inside NavigateTrack, and a full stop would close the session it just built.
  if (result == FollowTrackResult::Ok)
    [self hideNavigationMapControls];
  return OnFollowTrackResult(result);
}

+ (MWMFollowTrackResult)setFollowTrackProfile:(MWMRouterType)type
{
  auto const result =
      OnFollowTrackResult(GetFramework().GetRoutingManager().SetFollowTrackProfile(coreRouterType(type)));
  // Refresh presentation from the preview's effective profile.
  if (result == MWMFollowTrackResultOk)
    [MWMThemeManager invalidate];
  return result;
}

+ (BOOL)isFollowingTrack
{
  return GetFramework().GetRoutingManager().IsFollowingTrack();
}

+ (MWMFollowedTrackInfo *)followedTrackInfo
{
  auto const info = GetFramework().GetRoutingManager().GetFollowedTrackInfo();
  return info ? [[MWMFollowedTrackInfo alloc] initWithCoreInfo:*info] : nil;
}

+ (BOOL)isRoutingActive
{
  return GetFramework().GetRoutingManager().IsRoutingActive();
}
+ (BOOL)isRouteBuilt
{
  return GetFramework().GetRoutingManager().IsRouteBuilt();
}
+ (BOOL)isRouteFinished
{
  return GetFramework().GetRoutingManager().IsRouteFinished();
}
+ (BOOL)isRouteRebuildingOnly
{
  return GetFramework().GetRoutingManager().IsRouteRebuildingOnly();
}
+ (BOOL)isOnRoute
{
  return GetFramework().GetRoutingManager().IsRoutingFollowing();
}
+ (BOOL)isRouteValid
{
  return GetFramework().GetRoutingManager().IsRouteValid();
}

+ (BOOL)isSpeedCamLimitExceeded
{
  return GetFramework().GetRoutingManager().IsSpeedCamLimitExceeded();
}

+ (NSArray<MWMRoutePoint *> *)points
{
  NSMutableArray<MWMRoutePoint *> * points = [@[] mutableCopy];
  auto const routePoints = GetFramework().GetRoutingManager().GetRoutePoints();
  for (auto const & routePoint : routePoints)
    [points addObject:[[MWMRoutePoint alloc] initWithRouteMarkData:routePoint]];
  return [points copy];
}

+ (NSInteger)pointsCount
{
  return GetFramework().GetRoutingManager().GetRoutePointsCount();
}
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
  return GetFramework().GetRoutingManager().CouldAddIntermediatePoint();
}

+ (BOOL)isRoutePointsLimitReached
{
  // Unlike canAddIntermediatePoint, this also works before routing becomes active.
  return GetFramework().GetRoutingManager().GetRoutePointsCount() >= RoutePointsLayout::kMaxRoutePointsCount;
}

- (instancetype)initRouter
{
  self = [super init];
  if (self)
  {
    self.routeManagerTransactionId = RoutingManager::InvalidRoutePointsTransactionId();
    [MWMLocationManager addObserver:self];
    [MWMFrameworkListener addObserver:self];
    _canAutoAddLastLocation = YES;
    _routingOptions = [[MWMRoutingOptions alloc] initWithRouterType:MWMRouter.type];
    _isRestoreProcessCompleted = NO;
  }
  return self;
}

+ (void)subscribeToEvents
{
  [MWMFrameworkListener addObserver:[MWMRouter router]];
  [MWMLocationManager addObserver:[MWMRouter router]];
}

+ (void)unsubscribeFromEvents
{
  [MWMFrameworkListener removeObserver:[MWMRouter router]];
  [MWMLocationManager removeObserver:[MWMRouter router]];
}

+ (void)setType:(MWMRouterType)type
{
  if (type == self.type)
    return;

  [self doStop:NO];
  GetFramework().GetRoutingManager().SetRouter(coreRouterType(type));
}

+ (MWMRouterType)type
{
  return routerType(GetFramework().GetRoutingManager().GetRouter());
}

+ (MWMRouterType)renderType
{
  return routerType(GetFramework().GetRoutingManager().GetRenderRouterType());
}

+ (NSString *)stringFromRouterType:(MWMRouterType)type
{
  return ToNSString(ToString(coreRouterType(type)).c_str());
}

+ (void)disableFollowMode
{
  GetFramework().GetRoutingManager().DisableFollowMode();
}
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
  std::vector<std::string> notifications;
  auto announceStreets = [NSUserDefaults.standardUserDefaults boolForKey:@"UserDefaultsNeedToEnableStreetNamesTTS"];
  GetFramework().GetRoutingManager().GenerateNotifications(notifications, announceStreets);

  for (auto const & text : notifications)
    [turnNotifications addObject:@(text.c_str())];
  return [turnNotifications copy];
}

+ (void)removePointAndRebuild:(MWMRoutePoint *)point
{
  if (!point)
    return;
  if ([self pointsCount] == 1)
  {
    [self stopRouting];
    return;
  }
  RouteMarkData pt = point.routeMarkData;
  GetFramework().GetRoutingManager().RemoveRoutePoint(pt.m_pointType, pt.m_intermediateIndex);
  [[MWMNavigationDashboardManager sharedManager] onRoutePointsUpdated];
  [self rebuildWithBestRouter:NO];
}

+ (void)replacePointAndRebuild:(MWMRoutePoint *)point withPoint:(MWMRoutePoint *)newPoint
{
  if (!point)
  {
    NSAssert(NO, @"Source point can not be nil");
    return;
  }
  if (!newPoint)
  {
    NSAssert(NO, @"Target point can not be nil");
    return;
  }
  if (point.type == MWMRoutePointTypeStart)
  {
    [self buildFromPoint:newPoint bestRouter:NO];
    return;
  }
  if (point.type == MWMRoutePointTypeFinish)
  {
    // Destination setup can add the current location when this is the only route point.
    [self buildToPoint:newPoint bestRouter:NO];
    return;
  }

  auto & routingManager = GetFramework().GetRoutingManager();
  RouteMarkData pt = point.routeMarkData;
  RouteMarkData newPt = newPoint.routeMarkData;
  routingManager.ReplaceRoutePoint(pt.m_pointType, pt.m_intermediateIndex, std::move(newPt));
  [[MWMNavigationDashboardManager sharedManager] onRoutePointsUpdated];
  [self rebuildWithBestRouter:NO];
}

+ (void)reverseRoute
{
  if (!GetFramework().GetRoutingManager().ReverseRoutePoints())
    return;
  [self rebuildWithBestRouter:NO];
}

+ (void)removePoints
{
  GetFramework().GetRoutingManager().RemoveRoutePoints();
}

+ (void)addPoint:(MWMRoutePoint *)point
{
  if (!point)
  {
    NSAssert(NO, @"Point can not be nil");
    return;
  }

  RouteMarkData pt = point.routeMarkData;
  GetFramework().GetRoutingManager().AddRoutePoint(std::move(pt), RoutingOptions::LoadRouteOptimizationFromSettings());
  [[MWMNavigationDashboardManager sharedManager] onRoutePointsUpdated];
}

+ (void)continueRouteToPointAndRebuild:(MWMRoutePoint *)point
{
  if (!point)
  {
    NSAssert(NO, @"Point can not be nil");
    return;
  }

  RouteMarkData pt = point.routeMarkData;
  if (!GetFramework().GetRoutingManager().ContinueRouteToPoint(std::move(pt)))
    return;

  [[MWMNavigationDashboardManager sharedManager] onRoutePointsUpdated];
  [self rebuildWithBestRouter:NO];
}

+ (void)addPointAndRebuild:(MWMRoutePoint *)point
{
  if (!point)
    return;
  [self addPoint:point];
  [self rebuildWithBestRouter:NO];
}

// Route marks must never land inside a track-follow session: the follow has no editable points,
// so a mark would neither affect the followed route nor be torn down with it — a hybrid session
// with an orphan mark presented under the follow header. Leave the follow through the standard
// stop first, so the fresh plan is built and presented as a normal one.
+ (void)leaveTrackFollowIfNeeded
{
  if (![self isFollowingTrack])
    return;
  [self doStop:NO];
  [[MWMMapViewControlsManager manager] onRoutePrepare];
}

+ (void)buildFromPoint:(MWMRoutePoint *)startPoint bestRouter:(BOOL)bestRouter
{
  if (!startPoint)
    return;
  [self leaveTrackFollowIfNeeded];
  [self addPoint:startPoint];
  [self rebuildWithBestRouter:bestRouter];
}

+ (void)buildToPoint:(MWMRoutePoint *)finishPoint bestRouter:(BOOL)bestRouter
{
  if (!finishPoint)
    return;
  [self leaveTrackFollowIfNeeded];
  [self addPoint:finishPoint];
  if (![self startPoint] && [MWMLocationManager lastLocation] && [MWMRouter router].canAutoAddLastLocation)
    [self addPoint:[[MWMRoutePoint alloc] initWithLastLocationAndType:MWMRoutePointTypeStart intermediateIndex:0]];
  if ([self startPoint] && [self finishPoint])
    [self rebuildWithBestRouter:bestRouter];
}

+ (void)buildApiRouteWithType:(MWMRouterType)type
                   startPoint:(MWMRoutePoint *)startPoint
                  finishPoint:(MWMRoutePoint *)finishPoint
{
  if (!startPoint || !finishPoint)
    return;

  [self leaveTrackFollowIfNeeded];
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
  auto & rm = GetFramework().GetRoutingManager();
  auto const & points = rm.GetRoutePoints();
  auto const pointsCount = points.size();
  if (pointsCount < 2)
  {
    [self doStop:NO];
    [[MWMMapViewControlsManager manager] onRoutePrepare];
    return;
  }
  if (bestRouter)
    self.type = routerType(rm.GetBestRouter(points.front().m_position, points.back().m_position));

  [[MWMMapViewControlsManager manager] onRouteRebuild];
  rm.BuildRoute();
}

+ (void)start
{
  auto const doStart = ^{
    auto & rm = GetFramework().GetRoutingManager();
    if (rm.IsFollowingTrack())
    {
      // A followed track has no route marks. With a GPS fix the route already starts from the
      // user's position (or an instant re-projection handles the rest); without one keep the
      // standard "navigation is only possible from the current location" alert.
      if ([MWMLocationManager lastLocation])
      {
        [[MWMRoutingManager routingManager] startRoute];
        if (rm.IsRoutingFollowing())
          [[MWMMapViewControlsManager manager] onRouteStart];
        else if (rm.IsRouteBuilding())
          [[MWMMapViewControlsManager manager] onRouteRebuild];
      }
      else
      {
        [[MWMAlertViewController activeAlertController] presentPoint2PointAlertWithOkBlock:^{} needToRebuild:NO];
      }
      return;
    }
    auto const routePoints = rm.GetRoutePoints();
    if (routePoints.size() >= 2)
    {
      auto p1 = [[MWMRoutePoint alloc] initWithRouteMarkData:routePoints.front()];
      auto p2 = [[MWMRoutePoint alloc] initWithRouteMarkData:routePoints.back()];

      CLLocation * lastLocation = [MWMLocationManager lastLocation];
      if (p1.isMyPosition && lastLocation)
      {
        [[MWMRoutingManager routingManager] startRoute];
        [[MWMMapViewControlsManager manager] onRouteStart];
      }
      else
      {
        // The route is not followed here, and only following saves the points, so save them for
        // restoreRouteIfNeeded.
        [self saveRoute];

        BOOL const needToRebuild = lastLocation && [MWMLocationManager isStarted] && !p2.isMyPosition;

        [[MWMAlertViewController activeAlertController]
            presentPoint2PointAlertWithOkBlock:^{
              [self buildFromPoint:[[MWMRoutePoint alloc] initWithLastLocationAndType:MWMRoutePointTypeStart
                                                                    intermediateIndex:0]
                        bestRouter:NO];
            }
                                 needToRebuild:needToRebuild];
      }
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
  // End transit previews even when quick actions stop without an active route.
  GetFramework().HideRouteTransitIfNeeded();
  GetFramework().DeactivateMapSelection();
  [self doStop:removeRoutePoints];
  [self hideNavigationMapControls];
  [MWMRouter router].canAutoAddLastLocation = YES;
}

+ (void)doStop:(BOOL)removeRoutePoints
{
  [[MWMRoutingManager routingManager] stopRoutingAndRemoveRoutePoints:removeRoutePoints];
  if (removeRoutePoints)
    [[MWMRoutingManager routingManager] deleteSavedRoutePoints];
}

- (void)updateFollowingInfo
{
  if (![MWMRouter isRoutingActive])
    return;
  auto const & rm = GetFramework().GetRoutingManager();
  routing::FollowingInfo info;
  rm.GetRouteFollowingInfo(info);
  if (!info.IsValid())
    return;
  auto navManager = [MWMNavigationDashboardManager sharedManager];
  // The render type: a followed track is presented as its follow profile, never as transit.
  auto const type = [MWMRouter renderType];
  if (type == MWMRouterTypePublicTransport)
    [navManager updateTransitInfo:rm.GetTransitRouteInfo()];
  else
    [navManager updateFollowingInfo:info routePoints:[MWMRouter points] type:type];
}

#pragma mark - MWMLocationObserver

- (void)onLocationUpdate:(CLLocation *)location
{
  if (![MWMRouter isRoutingActive])
    return;
  auto tts = [MWMTextToSpeech tts];
  NSArray<NSString *> * turnNotifications = [MWMRouter turnNotifications];
  if ([MWMRouter isOnRoute] && tts.active)
  {
    [tts playTurnNotifications:turnNotifications];
    [tts playWarningSound];
  }

  [self updateFollowingInfo];
}

#pragma mark - MWMFrameworkRouteBuilderObserver

- (void)onRouteReady:(BOOL)hasWarnings
{
  self.routingOptions = [[MWMRoutingOptions alloc] initWithRouterType:MWMRouter.type];

  // A follow-track build has no route marks (the start point is always nil), but core following
  // must stay enabled: disabling it turns the session into RouteNoFollowing, which
  // EnableFollowMode() rejects — on every follow build (initial and each reroute) this would
  // kill starting navigation and the mid-navigation camera follow.
  dispatch_async(dispatch_get_main_queue(), ^{
    // Inspect the current session when this runs: another route may have replaced the one
    // whose ready callback queued this block.
    if ([MWMRouter isFollowingTrack])
      return;
    auto startPoint = [MWMRouter startPoint];
    if (!startPoint || !startPoint.isMyPosition)
      [MWMRouter disableFollowMode];
  });

  [[MWMMapViewControlsManager manager] onRouteReady:hasWarnings];
  [self updateFollowingInfo];
}

- (void)processRouteBuilderEvent:(routing::RouterResultCode)code
                       countries:(storage::CountriesSet const &)absentCountries
{
  MWMMapViewControlsManager * mapViewControlsManager = [MWMMapViewControlsManager manager];
  switch (code)
  {
  case routing::RouterResultCode::NoError: [self onRouteReady:NO]; break;
  case routing::RouterResultCode::HasWarnings: [self onRouteReady:YES]; break;
  case routing::RouterResultCode::NeedMoreMaps:
    self.routingOptions = [[MWMRoutingOptions alloc] initWithRouterType:MWMRouter.type];
    [self presentDownloaderAlert:code countries:absentCountries];
    // NeedMoreMaps can arrive after a valid route is already built. In that case
    // the user may decline extra maps and still navigate along the current route.
    if (![MWMRouter isRouteValid])
      [[MWMNavigationDashboardManager sharedManager] onRouteError:L(@"routing_planning_error")];
    break;
  case routing::RouterResultCode::FileTooOld:
  case routing::RouterResultCode::RouteNotFound:
    self.routingOptions = [[MWMRoutingOptions alloc] initWithRouterType:MWMRouter.type];
    [self presentDownloaderAlert:code countries:absentCountries];
    [[MWMNavigationDashboardManager sharedManager] onRouteError:L(@"routing_planning_error")];
    break;
  case routing::RouterResultCode::Cancelled: [mapViewControlsManager onRoutePrepare]; break;
  case routing::RouterResultCode::StartPointNotFound:
  case routing::RouterResultCode::EndPointNotFound:
  case routing::RouterResultCode::NoCurrentPosition:
  case routing::RouterResultCode::PointsInDifferentMWM:
  case routing::RouterResultCode::InternalError:
  case routing::RouterResultCode::IntermediatePointNotFound:
  case routing::RouterResultCode::TransitRouteNotFoundNoNetwork:
  case routing::RouterResultCode::TransitRouteNotFoundTooLongPedestrian:
  case routing::RouterResultCode::RouteNotFoundRedressRouteError:
    [[MWMAlertViewController activeAlertController] presentAlert:code];
    [[MWMNavigationDashboardManager sharedManager] onRouteError:L(@"routing_planning_error")];
    break;
  }
}

- (void)processRouteBuilderProgress:(CGFloat)progress
{
  [[MWMNavigationDashboardManager sharedManager] setRouteBuilderProgress:progress];
}

- (void)processRouteRecommendation:(MWMRouterRecommendation)recommendation
{
  switch (recommendation)
  {
  case MWMRouterRecommendationRebuildAfterPointsLoading:
    [MWMRouter addPointAndRebuild:[[MWMRoutePoint alloc] initWithLastLocationAndType:MWMRoutePointTypeStart
                                                                   intermediateIndex:0]];
    break;
  }
}

#pragma mark - Alerts

- (void)presentDownloaderAlert:(routing::RouterResultCode)code countries:(storage::CountriesSet const &)countries
{
  MWMAlertViewController * activeAlertController = [MWMAlertViewController activeAlertController];
  if (!countries.empty())
  {
    [activeAlertController presentDownloaderAlertWithCountries:countries
        code:code
        cancelBlock:^{
          if (code != routing::RouterResultCode::NeedMoreMaps)
            [MWMRouter stopRouting];
        }
        downloadBlock:^(storage::CountriesVec const & downloadCountries, MWMVoidBlock onSuccess) {
          NSMutableArray * array = [NSMutableArray arrayWithCapacity:downloadCountries.size()];
          for (auto const & cid : downloadCountries)
            [array addObject:@(cid.c_str())];
          [[MWMStorage sharedStorage] downloadNodes:array onSuccess:onSuccess];
        }
        downloadCompleteBlock:^{ [MWMRouter rebuildWithBestRouter:NO]; }];
  }
  else if ([MWMRouter hasActiveDrivingOptions])
  {
    [activeAlertController
        presentDefaultAlertWithTitle:L(@"unable_to_calc_alert_title")
                             message:L(@"unable_to_calc_alert_subtitle")
                    rightButtonTitle:L(@"settings")
                     leftButtonTitle:L(@"cancel")
                   rightButtonAction:^{ [[MapViewController sharedController] openDrivingOptions]; }];
  }
  else
  {
    [activeAlertController presentAlert:code];
  }
}

#pragma mark - Save / Load route points

+ (void)saveRoute
{
  GetFramework().GetRoutingManager().SaveRoutePoints();
}

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
    {
      self.router.isRestoreProcessCompleted = YES;
      return;
    }
    rm.LoadRoutePoints([self](bool success)
    {
      if (success)
        [self rebuildWithBestRouter:YES];
      self.router.isRestoreProcessCompleted = YES;
    });
  }
  else
  {
    dispatch_async(dispatch_get_main_queue(), ^{ [self restoreRouteIfNeeded]; });
  }
}

+ (BOOL)isRestoreProcessCompleted
{
  return self.router.isRestoreProcessCompleted;
}

+ (BOOL)hasSavedRoute
{
  return GetFramework().GetRoutingManager().HasSavedRoutePoints();
}

+ (void)updateRoute
{
  MWMRoutingOptions * newOptions = [[MWMRoutingOptions alloc] initWithRouterType:MWMRouter.type];
  // Driving options don't affect a followed track; skipping the rebuild also protects the
  // follow session from being torn down by the marks-based build path.
  if ((self.isRoutingActive && !self.isOnRoute && ![self isFollowingTrack]) &&
      ![newOptions isEqual:[self router].routingOptions])
    [self rebuildWithBestRouter:YES];
}

+ (BOOL)hasActiveDrivingOptions
{
  return [[MWMRoutingOptions alloc] initWithRouterType:self.type].hasOptions;
}

+ (void)avoidRoadTypeAndRebuild:(MWMRoadType)type
{
  MWMRoutingOptions * options = [[MWMRoutingOptions alloc] initWithRouterType:MWMRouter.type];
  switch (type)
  {
  case MWMRoadTypeToll: options.avoidToll = YES; break;
  case MWMRoadTypeDirty: options.avoidDirty = YES; break;
  case MWMRoadTypeFerry: options.avoidFerry = YES; break;
  case MWMRoadTypeMotorway: options.avoidMotorway = YES; break;
  }
  [options save];
  [self rebuildWithBestRouter:YES];
}

+ (void)optimizeRoutePointsAndRebuild
{
  // The core keeps the approved order while following or in Ruler mode.
  if ([self isRoutingActive] && GetFramework().GetRoutingManager().OptimizeRoutePoints())
    [self rebuildWithBestRouter:NO];
}

+ (void)showNavigationMapControls
{
  [[MWMMapViewControlsManager manager] onRouteStart];
}

+ (void)hideNavigationMapControls
{
  [[MWMMapViewControlsManager manager] onRouteStop];
}

@end
