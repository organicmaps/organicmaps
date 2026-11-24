#import <XCTest/XCTest.h>

#import "MWMLocationManager.h"
#import "MWMLocationObserver.h"
#import "MWMRouter.h"
#import "MWMRoutingManager.h"
#import "MWMTextToSpeech.h"
#import "MapViewController.h"

#include <CoreApi/Framework.h>

#include "routing/route.hpp"
#include "routing/routing_helpers.hpp"
#include "routing/routing_options.hpp"

#include "geometry/mercator.hpp"

#include "base/scope_guard.hpp"

#import <objc/runtime.h>

#include <string>
#include <utility>
#include <vector>

@interface MWMLocationManager (SpeechOwnershipTests) <CLLocationManagerDelegate>
+ (instancetype)manager;
@end

static void DeliverStartFix()
{
  auto location = [[CLLocation alloc] initWithLatitude:45.0 longitude:19.0];
  [[MWMLocationManager manager] locationManager:[[CLLocationManager alloc] init] didUpdateLocations:@[location]];
}

@interface MWMRouter (SpeechOwnershipTests)
+ (MWMRouter *)router;
+ (void)enterTrackNavigation;
@end

@interface MWMSpeechLocationListener : NSObject <MWMRoutingManagerListener>
@property(nonatomic, copy) void (^onLocation)(void);
@end

@implementation MWMSpeechLocationListener
- (void)processRouteBuilderEventWithCode:(MWMRouterResultCode)code countries:(NSArray<NSString *> *)countries
{}
- (void)updateCameraInfo:(BOOL)isCameraOnRoute speedLimitMps:(double)limit
{}
- (void)didLocationUpdate
{
  if (self.onLocation)
    self.onLocation();
}
@end

@interface MWMRoutingSpeechOwnershipTests : XCTestCase
{
  bool m_notificationsEnabled;
  std::string m_locale;
}
@end

@implementation MWMRoutingSpeechOwnershipTests
- (void)setUp
{
  [super setUp];
  [[MapViewController sharedController] loadViewIfNeeded];
  auto & manager = GetFramework().GetRoutingManager();
  m_notificationsEnabled = manager.AreTurnNotificationsEnabled();
  m_locale = manager.GetTurnNotificationsLocale();
  manager.CloseRouting(true);
}

- (void)tearDown
{
  auto & manager = GetFramework().GetRoutingManager();
  manager.CloseRouting(true);
  manager.RoutingSession().SetRoutingSettings(routing::GetRoutingSettings(
      routing::RoutingOptions::GetVehicleType(manager.GetRouter()).value_or(routing::VehicleType::Transit)));
  manager.EnableTurnNotifications(m_notificationsEnabled);
  manager.SetTurnNotificationsLocale(m_locale);
  [super tearDown];
}

- (void)prepareApproachWithNotificationsEnabled:(BOOL)enabled
{
  auto & manager = GetFramework().GetRoutingManager();
  manager.RoutingSession().Reset();
  auto const entry = mercator::FromLatLon(45.0, 19.0);
  auto const finish = mercator::FromLatLon(45.01, 19.0);
  auto const start = mercator::FromLatLon(44.998, 19.0);
  double const approach = mercator::DistanceOnEarth(start, entry);
  double const total = approach + mercator::DistanceOnEarth(entry, finish);
  routing::Segment const fake(routing::kFakeNumMwmId, 0, 0, false);
  std::vector<routing::RouteSegment> segments;
  segments.emplace_back(fake, routing::turns::TurnItem(1, routing::turns::PedestrianDirection::None),
                        geometry::PointWithAltitude(entry, 100), routing::RouteSegment::RoadNameInfo{});
  segments.emplace_back(fake, routing::turns::TurnItem(2, routing::turns::PedestrianDirection::ReachedYourDestination),
                        geometry::PointWithAltitude(finish, 110), routing::RouteSegment::RoadNameInfo{});
  segments[0].SetDistancesAndTime(approach, (entry - start).Length(), approach);
  segments[1].SetDistancesAndTime(total, (entry - start).Length() + (finish - entry).Length(), total);
  routing::Route route;
  route.SetRouteSegments(std::move(segments));
  route.SetSubroutes(std::vector<routing::Route::SubrouteAttrs>{
      {geometry::PointWithAltitude(start, 100), geometry::PointWithAltitude(finish, 110), 0, 2}});
  std::vector<m2::PointD> const points = {start, entry, finish};
  route.SetGeometry(points.begin(), points.end());
  route.SetTrackStartDistanceMeters(approach);
  manager.RoutingSession().SetRoutingSettings(routing::GetRoutingSettings(routing::VehicleType::Pedestrian));
  manager.SetTurnNotificationsLocale("en");
  [MWMRouter enableTurnNotifications:enabled];
  manager.RoutingSession().AssignRouteForTesting(std::move(route), routing::RouterResultCode::NoError);
  XCTAssertTrue(manager.RoutingSession().EnableFollowMode());
}

- (void)testStartingAtTheEntryLeavesTheFirstTurnForGps
{
  [MWMTextToSpeech tts];
  [MWMRouter hideNavigationMapControls];
  auto & manager = GetFramework().GetRoutingManager();
  auto const start = mercator::FromLatLon(45.0, 19.0);
  auto const bend = mercator::FromLatLon(45.0018, 19.0);
  auto const finish = mercator::FromLatLon(45.0018, 19.01);
  double const toBend = mercator::DistanceOnEarth(start, bend);
  double const total = toBend + mercator::DistanceOnEarth(bend, finish);
  routing::Segment const fake(routing::kFakeNumMwmId, 0, 0, false);
  std::vector<routing::RouteSegment> segments;
  segments.emplace_back(fake, routing::turns::TurnItem(1, routing::turns::PedestrianDirection::TurnRight),
                        geometry::PointWithAltitude(bend, 100), routing::RouteSegment::RoadNameInfo{});
  segments.emplace_back(fake, routing::turns::TurnItem(2, routing::turns::PedestrianDirection::ReachedYourDestination),
                        geometry::PointWithAltitude(finish, 100), routing::RouteSegment::RoadNameInfo{});
  segments[0].SetDistancesAndTime(toBend, (bend - start).Length(), toBend);
  segments[1].SetDistancesAndTime(total, (bend - start).Length() + (finish - bend).Length(), total);
  routing::Route route;
  route.SetRouteSegments(std::move(segments));
  route.SetSubroutes(std::vector<routing::Route::SubrouteAttrs>{
      {geometry::PointWithAltitude(start, 100), geometry::PointWithAltitude(finish, 100), 0, 2}});
  std::vector<m2::PointD> const points = {start, bend, finish};
  route.SetGeometry(points.begin(), points.end());
  route.SetTrackStartDistanceMeters(0.0);
  manager.RoutingSession().SetRoutingSettings(routing::GetRoutingSettings(routing::VehicleType::Pedestrian));
  manager.SetTurnNotificationsLocale("en");
  manager.EnableTurnNotifications(true);
  manager.RoutingSession().AssignRouteForTesting(std::move(route), routing::RouterResultCode::NoError);
  DeliverStartFix();
  XCTAssertTrue(manager.RoutingSession().EnableFollowMode());

  [MWMRouter enterTrackNavigation];

  XCTAssertGreaterThan([MWMRouter turnNotifications].count, 0U,
                       @"Start must leave the first maneuver pending until a navigation GPS update");
  [MWMRouter hideNavigationMapControls];
}

- (void)observePlayback:(void (^)(NSMutableArray<NSString *> *))body
{
  BOOL const wasTTSEnabled = [MWMTextToSpeech isTTSEnabled];
  [MWMTextToSpeech setTTSEnabled:YES];
  Method const play = class_getInstanceMethod(MWMTextToSpeech.class, @selector(playTurnNotifications:));
  auto played = [NSMutableArray<NSString *> array];
  IMP const playReplacement = imp_implementationWithBlock(
      ^(id self, NSArray<NSString *> * notifications) { [played addObjectsFromArray:notifications]; });
  IMP const originalPlay = method_setImplementation(play, playReplacement);
  SCOPE_GUARD(restoreSpeech, [&]
  {
    method_setImplementation(play, originalPlay);
    imp_removeBlock(playReplacement);
    [MWMTextToSpeech setTTSEnabled:wasTTSEnabled];
  });
  body(played);
}

- (void)testTrackStartPlaysApproachOnceAndDoesNotDeferMutedApproach
{
  SCOPE_GUARD(hideControls, [] { [MWMRouter hideNavigationMapControls]; });
  [self observePlayback:^(NSMutableArray<NSString *> * played) {
    for (BOOL const muted : {NO, YES})
    {
      [played removeAllObjects];
      [MWMRouter hideNavigationMapControls];
      [self prepareApproachWithNotificationsEnabled:!muted];
      [MWMRouter enterTrackNavigation];
      [MWMRouter enterTrackNavigation];
      XCTAssertEqualObjects(played, muted ? @[] : @[@"Proceed to the track."]);
      [MWMRouter enableTurnNotifications:YES];
      XCTAssertEqual([MWMRouter turnNotifications].count, 0U);
    }
  }];
}

- (void)notifyProxy
{
  auto location = [[CLLocation alloc] initWithLatitude:44.998 longitude:19.0];
  [(id<MWMLocationObserver>)[MWMRoutingManager routingManager] onLocationUpdate:location];
}

- (void)testPassiveProxyLeavesPhoneSpeechPending
{
  auto proxy = [MWMRoutingManager routingManager];
  for (bool const withListener : {false, true})
  {
    [self prepareApproachWithNotificationsEnabled:YES];
    auto listener = [[MWMSpeechLocationListener alloc] init];
    __block NSUInteger updates = 0;
    listener.onLocation = ^{ ++updates; };
    if (withListener)
      [proxy addListener:listener];
    SCOPE_GUARD(removeListener, [&]
    {
      if (withListener)
        [proxy removeListener:listener];
    });

    [self notifyProxy];

    XCTAssertEqual(updates, withListener ? 1U : 0U);
    XCTAssertEqualObjects([MWMRouter turnNotifications], (@[@"Proceed to the track."]));
    XCTAssertEqual([MWMRouter turnNotifications].count, 0U);
  }
}

- (void)testPhonePresentationLeavesApproachForTheLocationBatch
{
  [self observePlayback:^(NSMutableArray<NSString *> * played) {
    [self prepareApproachWithNotificationsEnabled:YES];
    auto fix = [[CLLocation alloc] initWithLatitude:44.998 longitude:19.0];
    [(id<MWMLocationObserver>)[MWMRouter router] onLocationUpdate:fix];
    XCTAssertEqual(played.count, 0U);
    auto locations = [MWMLocationManager manager];
    auto source = [[CLLocationManager alloc] init];
    [locations locationManager:source didUpdateLocations:@[fix, fix]];
    [locations locationManager:source didUpdateLocations:@[fix]];
    XCTAssertEqualObjects(played, (@[@"Proceed to the track."]));
  }];
}

- (void)testMutedLocationUpdatePublishesCurrentSecondTurn
{
  auto & manager = GetFramework().GetRoutingManager();
  auto const start = mercator::FromLatLon(45.0, 19.0);
  auto const right = mercator::GetSmPoint(start, 0, 16);
  auto const left = mercator::GetSmPoint(right, 65, 0);
  auto const finish = mercator::GetSmPoint(left, 0, 600);
  using routing::turns::CarDirection;
  std::vector<m2::PointD> const points = {start, start, right, left, finish};
  std::vector<CarDirection> const turns = {CarDirection::None, CarDirection::TurnRight, CarDirection::TurnLeft,
                                           CarDirection::ReachedYourDestination};
  std::vector<routing::RouteSegment> segments;
  for (uint32_t i = 0; i < turns.size(); ++i)
    segments.emplace_back(routing::Segment(routing::kFakeNumMwmId, 0, i, true),
                          routing::turns::TurnItem(i + 1, turns[i]), geometry::PointWithAltitude(points[i + 1], 0),
                          routing::RouteSegment::RoadNameInfo{});
  routing::FillSegmentInfo({0, 2, 8, 70}, segments);
  routing::Route route;
  route.SetSubroutes(std::vector<routing::Route::SubrouteAttrs>{
      {geometry::PointWithAltitude(start, 0), geometry::PointWithAltitude(finish, 0), 0, segments.size()}});
  route.SetRouteSegments(std::move(segments));
  route.SetGeometry(points.begin(), points.end());
  auto & session = manager.RoutingSession();
  session.SetRoutingSettings(routing::GetRoutingSettings(routing::VehicleType::Car));
  manager.EnableTurnNotifications(false);
  session.AssignRouteForTesting(std::move(route), routing::RouterResultCode::NoError);
  XCTAssertTrue(session.EnableFollowMode());
  auto proxy = [MWMRoutingManager routingManager];
  auto listener = [[MWMSpeechLocationListener alloc] init];
  __block CarDirection observed = CarDirection::None;
  listener.onLocation = ^{
    routing::FollowingInfo info;
    GetFramework().GetRoutingManager().GetRouteFollowingInfo(info);
    observed = info.m_nextTurn;
  };
  [proxy addListener:listener];
  SCOPE_GUARD(removeListener, [&] { [proxy removeListener:listener]; });
  DeliverStartFix();
  XCTAssertTrue(observed == CarDirection::TurnLeft);
}
@end
