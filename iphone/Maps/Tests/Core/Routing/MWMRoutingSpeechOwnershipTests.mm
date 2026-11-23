#import <XCTest/XCTest.h>

#import "Bridging-Header.h"
#import "MWMFrameworkObservers.h"
#import "SwiftBridge.h"

#include <CoreApi/Framework.h>

#include "routing/routing_helpers.hpp"
#include "routing/ruler_router.hpp"

#import <objc/runtime.h>

#include <array>

@interface MWMLocationManager (SpeechTests) <CLLocationManagerDelegate>
+ (instancetype)manager;
@end

@interface MWMRouter (SpeechTests)
+ (id<MWMLocationObserver>)router;
@end

namespace
{
auto const kStart = mercator::FromLatLon(45.0, 19.0);
auto const kRight = mercator::GetSmPoint(kStart, 0, 16);
auto const kLeft = mercator::GetSmPoint(kRight, 65, 0);
auto const kFinish = mercator::GetSmPoint(kLeft, 0, 600);

// Two nearby driving turns and a long final leg, independent of downloaded maps.
class SpeechTestRouter final : public routing::RulerRouter
{
  routing::RouterResultCode CalculateRoute(routing::Checkpoints const & checkpoints, m2::PointD const &, bool, bool,
                                           routing::RouterDelegate const &, routing::RoutesResult & result) override
  {
    using namespace routing;
    using turns::CarDirection;
    std::array const points = {checkpoints.GetStart(), checkpoints.GetStart(), kRight, kLeft, checkpoints.GetFinish()};
    std::array const directions = {CarDirection::None, CarDirection::TurnRight, CarDirection::TurnLeft,
                                   CarDirection::ReachedYourDestination};
    std::vector<RouteSegment> segments;
    for (uint32_t i = 0; i < directions.size(); ++i)
      segments.emplace_back(Segment(kFakeNumMwmId, 0, i, true), turns::TurnItem(i + 1, directions[i]),
                            geometry::PointWithAltitude(points[i + 1], 0), RouteSegment::RoadNameInfo{});
    FillSegmentInfo({0, 2, 8, 70}, segments);
    Route route;
    route.SetSubroutes(
        std::vector<Route::SubrouteAttrs>{{{points.front(), 0}, {points.back(), 0}, 0, segments.size()}});
    route.SetRouteSegments(std::move(segments));
    route.SetGeometry(points.begin(), points.end());
    result.MakeFrom("speech-test-router", std::move(route));
    return RouterResultCode::NoError;
  }
};

CLLocation * LocationAt(m2::PointD const & point)
{
  auto const ll = mercator::ToLatLon(point);
  return [[CLLocation alloc] initWithLatitude:ll.m_lat longitude:ll.m_lon];
}

// The renderer-free test host needs the matched routing position before delivering the iOS callback.
CLLocation * MoveAlongFinalLeg(routing::RoutingSession & session, double metersToFinish)
{
  CLLocation * fix = LocationAt(mercator::GetSmPoint(kFinish, 0, -metersToFinish));
  location::GpsInfo gps;
  gps.m_latitude = fix.coordinate.latitude;
  gps.m_longitude = fix.coordinate.longitude;
  gps.m_horizontalAccuracy = fix.horizontalAccuracy;
  session.OnLocationPositionChanged(gps);
  return fix;
}

void Deliver(CLLocation * fix)
{
  [[MWMLocationManager manager] locationManager:[[CLLocationManager alloc] init] didUpdateLocations:@[fix]];
}
}  // namespace

@interface MWMRoutingSpeechOwnershipTests : XCTestCase <MWMFrameworkRouteBuilderObserver>
{
  MWMRouterType m_previousRouter;
  BOOL m_ttsEnabled;
  bool m_notificationsEnabled;
  std::string m_locale;
  IMP m_originalPlayback;
}
@property(nonatomic) NSMutableArray<NSString *> * spoken;
@property(nonatomic) XCTestExpectation * routeReady;
@end

@implementation MWMRoutingSpeechOwnershipTests
- (void)setUp
{
  [super setUp];
  [[MapViewController sharedController] loadViewIfNeeded];
  [MWMTextToSpeech tts];
  m_previousRouter = MWMRouter.type;
  auto & manager = GetFramework().GetRoutingManager();
  m_ttsEnabled = MWMTextToSpeech.isTTSEnabled;
  m_notificationsEnabled = manager.AreTurnNotificationsEnabled();
  m_locale = manager.GetTurnNotificationsLocale();
  [MWMTextToSpeech setTTSEnabled:YES];
  manager.SetTurnNotificationsLocale("en");
  manager.EnableTurnNotifications(true);

  self.spoken = [NSMutableArray array];
  auto spoken = self.spoken;
  Method const play = class_getInstanceMethod(MWMTextToSpeech.class, @selector(playTurnNotifications:));
  m_originalPlayback = method_setImplementation(
      play, imp_implementationWithBlock(^(id, NSArray * phrases) { [spoken addObjectsFromArray:phrases]; }));

  [MWMRouter stopRouting];
  [MWMRouter setType:MWMRouterTypeVehicle];
  [MWMRouter unsubscribeFromEvents];
  // New observers replay the last fix, including one left by another test.
  Deliver(LocationAt(kStart));
  manager.RoutingSession().SetRouter(std::make_unique<SpeechTestRouter>(), nullptr);
  RouteMarkData start;
  start.m_pointType = RouteMarkType::Start;
  start.m_position = kStart;
  manager.AddRoutePoint(std::move(start), false);
  RouteMarkData finish;
  finish.m_pointType = RouteMarkType::Finish;
  finish.m_position = kFinish;
  manager.AddRoutePoint(std::move(finish), false);
  [MWMFrameworkListener addObserver:self];
  self.routeReady = [self expectationWithDescription:@"Driving route preview"];
  manager.BuildRoute();
  [self waitForExpectations:@[self.routeReady] timeout:5];
  [MWMFrameworkListener removeObserver:self];
}

- (void)processRouteBuilderEvent:(routing::RouterResultCode)code countries:(storage::CountriesSet const &)countries
{
  XCTAssertTrue(code == routing::RouterResultCode::NoError);
  [self.routeReady fulfill];
}

- (void)tearDown
{
  [MWMRouter stopRouting];
  // Reinstall the normal vehicle router even when the previous profile was also driving.
  [MWMRouter setType:MWMRouterTypeRuler];
  [MWMRouter setType:m_previousRouter];
  [MWMRouter subscribeToEvents];
  [MWMTextToSpeech setTTSEnabled:m_ttsEnabled];
  auto & manager = GetFramework().GetRoutingManager();
  manager.EnableTurnNotifications(m_notificationsEnabled);
  manager.SetTurnNotificationsLocale(m_locale);
  Method const play = class_getInstanceMethod(MWMTextToSpeech.class, @selector(playTurnNotifications:));
  imp_removeBlock(method_setImplementation(play, m_originalPlayback));
  [super tearDown];
}

- (void)testPhonePreviewPreservesFirstSpokenTurn
{
  auto car = [[MWMSpeechTestCarPlayPresentation alloc] initWithActive:NO hasTrip:YES];
  [MWMRouter subscribeToEvents];
  XCTestExpectation * subscribed = [self expectationWithDescription:@"Phone location observer registered"];
  dispatch_async(dispatch_get_main_queue(), ^{ [subscribed fulfill]; });
  [self waitForExpectations:@[subscribed] timeout:5];

  CLLocation * start = LocationAt(kStart);
  Deliver(start);
  Deliver(start);
  XCTAssertEqual(self.spoken.count, 0U);
  XCTAssertTrue(GetFramework().GetRoutingManager().RoutingSession().EnableFollowMode());
  Deliver(start);
  XCTAssertEqual(self.spoken.count, 0U);
  Deliver(start);
  XCTAssertEqualObjects(self.spoken.firstObject, @"Turn right.");
  XCTAssertFalse(car.showsNextTurn);
}

- (void)testDashboardReceivesSpeechBeforeATripExists
{
  auto dashboard = [[MWMSpeechTestCarPlayPresentation alloc] initWithActive:YES hasTrip:NO];
  XCTAssertTrue(GetFramework().GetRoutingManager().RoutingSession().EnableFollowMode());
  CLLocation * start = LocationAt(kStart);
  Deliver(start);
  Deliver(start);
  XCTAssertEqualObjects(self.spoken.firstObject, @"Turn right.");
  XCTAssertFalse(dashboard.showsNextTurn);
}

- (void)testMutedCarPlayReceivesTheCurrentSecondTurn
{
  auto car = [[MWMSpeechTestCarPlayPresentation alloc] initWithActive:YES hasTrip:YES];
  [MWMTextToSpeech tts].active = NO;
  XCTAssertTrue(GetFramework().GetRoutingManager().RoutingSession().EnableFollowMode());
  Deliver(LocationAt(kStart));
  XCTAssertTrue(car.showsNextTurn);
  XCTAssertEqual(self.spoken.count, 0U);
}

- (void)testCarPlayGetsArrivalBeforeFinishingDespiteALatePhoneCallback
{
  auto car = [[MWMSpeechTestCarPlayPresentation alloc] initWithActive:YES hasTrip:YES];
  auto & session = GetFramework().GetRoutingManager().RoutingSession();
  XCTAssertTrue(session.EnableFollowMode());
  Deliver(MoveAlongFinalLeg(session, 60));
  CLLocation * arrival = MoveAlongFinalLeg(session, 8);
  XCTAssertTrue(session.IsFinished());

  [MWMSpeechTestCarPlayPresentation whileCarOwnsRouting:^{ [[MWMRouter router] onLocationUpdate:arrival]; }];
  XCTAssertTrue(session.IsFinished());
  Deliver(arrival);
  XCTAssertEqualObjects(self.spoken.lastObject, @"You have arrived.");
  XCTAssertTrue(car.finished);
  XCTAssertFalse(session.IsActive());
}
@end
