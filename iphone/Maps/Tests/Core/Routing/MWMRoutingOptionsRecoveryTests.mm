#import <XCTest/XCTest.h>

#import "MWMAlertViewController.h"
#import "MWMDefaultAlert.h"
#import "MWMFrameworkListener.h"
#import "MWMFrameworkObservers.h"
#import "MWMMacros.h"
#import "MWMRouter.h"
#import "MWMRoutingManager.h"
#import "MWMRoutingOptions.h"
#import "MapViewController.h"

#include <CoreApi/Framework.h>

#include "routing/routing_options.hpp"
#include "routing/ruler_router.hpp"

#include <chrono>
#include <memory>
#include <semaphore>
#include <utility>

namespace
{
struct RequestGate
{
  std::binary_semaphore m_waitingToRead{0};
  std::binary_semaphore m_mayRead{0};
  std::binary_semaphore m_didRead{0};
  std::binary_semaphore m_mayFinish{0};
};

using namespace std::chrono_literals;

// Keep the session and platform callbacks real without requiring downloaded maps.
class AvoidanceTestRouter final : public routing::RulerRouter
{
public:
  AvoidanceTestRouter(routing::VehicleType vehicle, routing::RouterResultCode failure,
                      routing::RoutingOptions::Road road = routing::RoutingOptions::Ferry,
                      std::shared_ptr<RequestGate> const & gate = {})
    : m_vehicle(vehicle)
    , m_failure(failure)
    , m_road(road)
    , m_gate(gate)
  {}

private:
  routing::RouterResultCode CalculateRoute(routing::Checkpoints const & checkpoints, m2::PointD const & direction,
                                           bool adjust, bool alternatives, routing::RouterDelegate const & delegate,
                                           routing::RoutesResult & result) override
  {
    auto const gate = std::move(m_gate);
    if (gate)
    {
      gate->m_waitingToRead.release();
      if (!gate->m_mayRead.try_acquire_for(5s))
        return routing::RouterResultCode::InternalError;
    }
    auto const submittedOptions = delegate.GetRoutingOptions();
    auto const options = submittedOptions ? *submittedOptions : routing::RoutingOptions::LoadFromSettings(m_vehicle);
    bool const avoid = options.Has(m_road);
    if (gate)
    {
      gate->m_didRead.release();
      if (!gate->m_mayFinish.try_acquire_for(5s))
        return routing::RouterResultCode::InternalError;
    }
    if (avoid)
      return m_failure;
    return static_cast<routing::IRouter &>(m_ruler).CalculateRoute(checkpoints, direction, adjust, alternatives,
                                                                   delegate, result);
  }

  routing::VehicleType const m_vehicle;
  routing::RouterResultCode const m_failure;
  routing::RoutingOptions::Road const m_road;
  std::shared_ptr<RequestGate> m_gate;
  routing::RulerRouter m_ruler;
};
}  // namespace

@interface MWMRouter (RoutingOptionsRecoveryTests)
+ (id<MWMFrameworkRouteBuilderObserver>)router;
@end

@interface MWMDefaultAlert (RoutingOptionsRecoveryTests)
@property(nonatomic, readonly) UIButton * rightButton;
@end

@interface MWMRoutingOptionsRecoveryTests : XCTestCase <MWMFrameworkRouteBuilderObserver>
{
  MWMRouterType m_previousRouter;
  routing::RouterResultCode m_result;
  m2::PointD m_previousPosition;
  bool m_hadPosition;
}
@property(nonatomic) NSArray<MWMRoutingOptions *> * savedOptions;
@property(nonatomic) XCTestExpectation * completed;
@property(nonatomic) NSUInteger eventsCount;
@end

@implementation MWMRoutingOptionsRecoveryTests

- (void)setUp
{
  [super setUp];
  [[MapViewController sharedController] loadViewIfNeeded];
  m_previousRouter = MWMRouter.type;
  self.savedOptions = @[
    [[MWMRoutingOptions alloc] initWithRouterType:MWMRouterTypeVehicle],
    [[MWMRoutingOptions alloc] initWithRouterType:MWMRouterTypeBicycle],
    [[MWMRoutingOptions alloc] initWithRouterType:MWMRouterTypePedestrian]
  ];
  auto const & position = GetFramework().GetBookmarkManager().MyPositionMark();
  m_previousPosition = position.GetPivot();
  m_hadPosition = position.HasPosition();
  [MWMRouter stopRouting];
  [MWMRouter subscribeToEvents];
  [MWMFrameworkListener addObserver:self];
}

- (void)tearDown
{
  [MWMFrameworkListener removeObserver:self];
  self.completed = nil;
  [MWMRouter stopRouting];
  // Switching through another mode restores the real router even when its type is unchanged.
  [MWMRouter setType:m_previousRouter == MWMRouterTypeRuler ? MWMRouterTypeVehicle : MWMRouterTypeRuler];
  [MWMRouter setType:m_previousRouter];
  for (MWMRoutingOptions * options in self.savedOptions)
    [options save];
  GetFramework().GetBookmarkManager().MyPositionMark().SetUserPosition(m_previousPosition, m_hadPosition);
  [super tearDown];
}

- (void)processRouteBuilderEvent:(routing::RouterResultCode)code countries:(storage::CountriesSet const &)countries
{
  m_result = code;
  ++self.eventsCount;
  [self.completed fulfill];
}

- (void)expectResult:(routing::RouterResultCode)code action:(void (^)(void))action
{
  self.completed = [self expectationWithDescription:@"Route result"];
  action();
  [self waitForExpectations:@[self.completed] timeout:5];
  self.completed = nil;
  XCTAssertTrue(m_result == code, @"%s", routing::ToString(m_result).c_str());
  [self closeRouteAlert];
}

- (void)closeRouteAlert
{
  MWMAlertViewController * alerts = MWMAlertViewController.activeAlertController;
  if (alerts.isAlertDisplayed)
  {
    XCTestExpectation * closed = [self expectationWithDescription:@"Route alert closed"];
    [alerts closeAlert:^{ [closed fulfill]; }];
    [self waitForExpectations:@[closed] timeout:2];
  }
}

- (void)prepareRoute:(MWMRouterType)type failure:(routing::RouterResultCode)failure
{
  [MWMRouter stopRouting];
  [MWMRouter setType:type];
  auto & manager = GetFramework().GetRoutingManager();
  auto const vehicle = *routing::RoutingOptions::GetVehicleType(manager.GetRouter());
  routing::RoutingOptions::SaveToSettings(vehicle, {});
  manager.RoutingSession().SetRouter(std::make_unique<AvoidanceTestRouter>(vehicle, failure), nullptr);
  RouteMarkData start;
  start.m_pointType = RouteMarkType::Start;
  start.m_position = {1, 1};
  manager.AddRoutePoint(std::move(start), false);
  RouteMarkData finish;
  finish.m_pointType = RouteMarkType::Finish;
  finish.m_position = {2, 2};
  manager.AddRoutePoint(std::move(finish), false);
  [self expectResult:routing::RouterResultCode::NoError action:^{ [MWMRouter rebuildWithBestRouter:NO]; }];
}

- (void)testEndpointAlertsOfferSettingsWithActiveAvoidance
{
  using routing::RouterResultCode;
  using routing::RoutingOptions;
  storage::CountriesSet const absentCountries;
  [MWMRouter setType:MWMRouterTypePedestrian];
  for (bool const avoidSteps : {false, true})
  {
    RoutingOptions::SaveToSettings(routing::VehicleType::Pedestrian,
                                   RoutingOptions(avoidSteps ? RoutingOptions::Steps : 0));
    for (auto const failure : {RouterResultCode::StartPointNotFound, RouterResultCode::EndPointNotFound,
                               RouterResultCode::IntermediatePointNotFound})
    {
      [MWMRouter.router processRouteBuilderEvent:failure countries:absentCountries];
      auto * alert = (MWMDefaultAlert *)MWMAlertViewController.activeAlertController.view.subviews.lastObject;
      XCTAssertEqualObjects(alert.rightButton.currentTitle, avoidSteps ? L(@"settings") : L(@"ok"));
      [self closeRouteAlert];
    }
  }
}

- (void)testChangedAvoidanceRetriesFailedRoutes
{
  for (auto const type : {MWMRouterTypeVehicle, MWMRouterTypeBicycle, MWMRouterTypePedestrian})
  {
    for (auto const failure :
         {routing::RouterResultCode::StartPointNotFound, routing::RouterResultCode::EndPointNotFound,
          routing::RouterResultCode::IntermediatePointNotFound, routing::RouterResultCode::RouteNotFound})
    {
      [self prepareRoute:type failure:failure];
      [self expectResult:failure action:^{ [MWMRouter avoidRoadTypeAndRebuild:MWMRoadTypeFerry]; }];
      XCTAssertFalse(MWMRouter.isRouteValid);
      XCTAssertEqual(MWMRouter.pointsCount, 2);
      MWMRoutingOptions * options = [[MWMRoutingOptions alloc] initWithRouterType:type];
      options.avoidFerry = NO;
      [options save];
      [self expectResult:routing::RouterResultCode::NoError action:^{ [MWMRouter updateRoute]; }];
      XCTAssertTrue(MWMRouter.isRouteValid);
    }
  }
}

- (void)testChangedAvoidanceRetriesAfterMissingPosition
{
  [self prepareRoute:MWMRouterTypePedestrian failure:routing::RouterResultCode::StartPointNotFound];
  auto & framework = GetFramework();
  auto & manager = framework.GetRoutingManager();
  framework.GetBookmarkManager().MyPositionMark().SetUserPosition({1, 1}, false);
  RouteMarkData start = manager.GetRoutePoints().front();
  start.m_isMyPosition = true;
  manager.ReplaceRoutePoint(RouteMarkType::Start, 0, std::move(start));
  [self expectResult:routing::RouterResultCode::NoCurrentPosition
              action:^{ [MWMRouter avoidRoadTypeAndRebuild:MWMRoadTypeFerry]; }];
  XCTAssertFalse(MWMRouter.isRoutingActive);
  NSUInteger const events = self.eventsCount;
  [MWMRouter updateRoute];
  XCTAssertEqual(self.eventsCount, events);
  framework.GetBookmarkManager().MyPositionMark().SetUserPosition({1, 1}, true);
  MWMRoutingOptions * options = [[MWMRoutingOptions alloc] initWithRouterType:MWMRouterTypePedestrian];
  options.avoidFerry = NO;
  [options save];
  [self expectResult:routing::RouterResultCode::NoError action:^{ [MWMRouter updateRoute]; }];
  XCTAssertTrue(MWMRouter.isRouteValid);
}

- (void)testEditAfterWorkerReadIsAppliedToSuccessfulAndFailedPlans
{
  using routing::RouterResultCode;
  using routing::RoutingOptions;
  auto const vehicle = routing::VehicleType::Pedestrian;
  for (bool const startsAvoiding : {false, true})
  {
    [self prepareRoute:MWMRouterTypePedestrian failure:RouterResultCode::StartPointNotFound];
    RoutingOptions::SaveToSettings(vehicle, RoutingOptions(startsAvoiding ? RoutingOptions::Steps : 0));
    auto const gate = std::make_shared<RequestGate>();
    GetFramework().GetRoutingManager().RoutingSession().SetRouter(
        std::make_unique<AvoidanceTestRouter>(vehicle, RouterResultCode::StartPointNotFound, RoutingOptions::Steps,
                                              gate),
        nullptr);
    auto const first = startsAvoiding ? RouterResultCode::StartPointNotFound : RouterResultCode::NoError;
    auto const second = startsAvoiding ? RouterResultCode::NoError : RouterResultCode::StartPointNotFound;
    [self expectResult:first
                action:^{
                  [MWMRouter rebuildWithBestRouter:NO];
                  XCTAssertTrue(gate->m_waitingToRead.try_acquire_for(5s));
                  gate->m_mayRead.release();
                  XCTAssertTrue(gate->m_didRead.try_acquire_for(5s));
                  MWMRoutingOptions * options = [[MWMRoutingOptions alloc] initWithRouterType:MWMRouterTypePedestrian];
                  options.avoidSteps = !startsAvoiding;
                  [options save];
                  gate->m_mayFinish.release();
                }];
    [self expectResult:second action:^{ [MWMRouter updateRoute]; }];
    XCTAssertEqual(MWMRouter.isRouteValid, startsAvoiding);
  }
}

- (void)testWorkerUsesSubmittedOptionsWhenEditIsRevertedBeforeTheResult
{
  using routing::RouterResultCode;
  using routing::RoutingOptions;
  auto const vehicle = routing::VehicleType::Pedestrian;
  [self prepareRoute:MWMRouterTypePedestrian failure:RouterResultCode::StartPointNotFound];
  auto const gate = std::make_shared<RequestGate>();
  GetFramework().GetRoutingManager().RoutingSession().SetRouter(
      std::make_unique<AvoidanceTestRouter>(vehicle, RouterResultCode::StartPointNotFound, RoutingOptions::Steps, gate),
      nullptr);
  [self expectResult:RouterResultCode::NoError
              action:^{
                [MWMRouter rebuildWithBestRouter:NO];
                XCTAssertTrue(gate->m_waitingToRead.try_acquire_for(5s));
                MWMRoutingOptions * options = [[MWMRoutingOptions alloc] initWithRouterType:MWMRouterTypePedestrian];
                options.avoidSteps = YES;
                [options save];
                gate->m_mayRead.release();
                XCTAssertTrue(gate->m_didRead.try_acquire_for(5s));
                options.avoidSteps = NO;
                [options save];
                gate->m_mayFinish.release();
              }];
  [MWMRouter updateRoute];
  XCTAssertTrue(MWMRouter.isRouteValid);
}

- (void)testPreviewBuiltThroughRoutingManagerCanRecoverFromAnInFlightEdit
{
  using routing::RouterResultCode;
  using routing::RoutingOptions;
  auto const vehicle = routing::VehicleType::Car;
  [self prepareRoute:MWMRouterTypeVehicle failure:RouterResultCode::StartPointNotFound];
  RoutingOptions::SaveToSettings(vehicle, RoutingOptions(RoutingOptions::Ferry));
  auto const gate = std::make_shared<RequestGate>();
  GetFramework().GetRoutingManager().RoutingSession().SetRouter(
      std::make_unique<AvoidanceTestRouter>(vehicle, RouterResultCode::StartPointNotFound, RoutingOptions::Ferry, gate),
      nullptr);
  [self expectResult:RouterResultCode::StartPointNotFound
              action:^{
                NSError * error = nil;
                [[MWMRoutingManager routingManager] buildRouteWithDidFailError:&error];
                XCTAssertNil(error);
                XCTAssertTrue(gate->m_waitingToRead.try_acquire_for(5s));
                gate->m_mayRead.release();
                XCTAssertTrue(gate->m_didRead.try_acquire_for(5s));
                MWMRoutingOptions * options = [[MWMRoutingOptions alloc] initWithRouterType:MWMRouterTypeVehicle];
                options.avoidFerry = NO;
                [options save];
                gate->m_mayFinish.release();
              }];
  [self expectResult:RouterResultCode::NoError action:^{ [MWMRouter updateRoute]; }];
  XCTAssertTrue(MWMRouter.isRouteValid);
}

- (void)testChangedAvoidanceDoesNotRestartNavigationOrAnEmptyPlan
{
  [self prepareRoute:MWMRouterTypePedestrian failure:routing::RouterResultCode::StartPointNotFound];
  auto & manager = GetFramework().GetRoutingManager();
  GetFramework().GetBookmarkManager().MyPositionMark().SetUserPosition({1, 1}, true);
  RouteMarkData start = manager.GetRoutePoints().front();
  start.m_isMyPosition = true;
  manager.ReplaceRoutePoint(RouteMarkType::Start, 0, std::move(start));
  [self expectResult:routing::RouterResultCode::NoError action:^{ [MWMRouter rebuildWithBestRouter:NO]; }];
  XCTAssertTrue(manager.RoutingSession().EnableFollowMode());
  MWMRoutingOptions * options = [[MWMRoutingOptions alloc] initWithRouterType:MWMRouterTypePedestrian];
  options.avoidFerry = YES;
  [options save];
  [MWMRouter updateRoute];
  XCTAssertTrue(MWMRouter.isOnRoute);
  [MWMRouter stopRouting];
  NSUInteger const events = self.eventsCount;
  [MWMRouter updateRoute];
  XCTAssertEqual(self.eventsCount, events);
}

@end
