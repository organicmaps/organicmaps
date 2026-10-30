#import <XCTest/XCTest.h>

#import "MWMFrameworkListener.h"
#import "MWMFrameworkObservers.h"
#import "MWMRouter.h"

#include <CoreApi/Framework.h>

#include <utility>

@interface MWMSynchronousRouteObserver : NSObject <MWMFrameworkRouteBuilderObserver>
@property(nonatomic) NSMutableArray<NSNumber *> * codes;
@property(nonatomic, copy) void (^onEvent)(routing::RouterResultCode);
@end

@implementation MWMSynchronousRouteObserver
- (void)processRouteBuilderEvent:(routing::RouterResultCode)code countries:(storage::CountriesSet const &)countries
{
  XCTAssertTrue(NSThread.isMainThread);
  [self.codes addObject:@(static_cast<int>(code))];
  if (self.onEvent)
    self.onEvent(code);
}
@end

@interface MWMFrameworkListenerTests : XCTestCase
{
  MWMRouterType m_previousRouter;
}
@property(nonatomic) MWMSynchronousRouteObserver * observer;
@end

@implementation MWMFrameworkListenerTests
- (void)setUp
{
  [super setUp];
  m_previousRouter = MWMRouter.type;
  [MWMRouter stopRouting];
  [MWMRouter unsubscribeFromEvents];
  [MWMRouter setType:MWMRouterTypeRuler];
  self.observer = [[MWMSynchronousRouteObserver alloc] init];
  self.observer.codes = [NSMutableArray array];
  [MWMFrameworkListener addObserver:self.observer];
}

- (void)tearDown
{
  [MWMFrameworkListener removeObserver:self.observer];
  self.observer.onEvent = nil;
  [MWMRouter stopRouting];
  [MWMRouter setType:m_previousRouter];
  [MWMRouter subscribeToEvents];
  self.observer = nil;
  [super tearDown];
}

- (void)testValidationErrorIsDeliveredBeforeBuildReturns
{
  auto & manager = GetFramework().GetRoutingManager();
  manager.BuildRoute();
  XCTAssertEqualObjects(self.observer.codes, (@[@(static_cast<int>(routing::RouterResultCode::Cancelled))]));
  XCTAssertFalse(manager.IsRoutingActive());
}

- (void)testRemovalTakesEffectBeforeTheNextBuild
{
  auto & manager = GetFramework().GetRoutingManager();
  manager.BuildRoute();
  XCTAssertEqual(self.observer.codes.count, 1);
  [MWMFrameworkListener removeObserver:self.observer];
  manager.BuildRoute();
  XCTAssertEqual(self.observer.codes.count, 1);
}

- (void)testObserverCanUnsubscribeDuringNotification
{
  __weak MWMSynchronousRouteObserver * observer = self.observer;
  self.observer.onEvent = ^(routing::RouterResultCode) { [MWMFrameworkListener removeObserver:observer]; };
  auto & manager = GetFramework().GetRoutingManager();
  manager.BuildRoute();
  XCTAssertEqual(self.observer.codes.count, 1);
  manager.BuildRoute();
  XCTAssertEqual(self.observer.codes.count, 1);
}

- (void)testValidationObserverCanStartANewBuild
{
  auto * manager = &GetFramework().GetRoutingManager();
  XCTestExpectation * built = [self expectationWithDescription:@"Replacement route built"];
  __block bool requested = false;
  self.observer.onEvent = ^(routing::RouterResultCode code) {
    if (!requested)
    {
      XCTAssertTrue(code == routing::RouterResultCode::Cancelled);
      XCTAssertFalse(manager->IsRoutingActive());
      requested = true;
      RouteMarkData start;
      start.m_pointType = RouteMarkType::Start;
      start.m_position = {1, 1};
      manager->AddRoutePoint(std::move(start), false);
      RouteMarkData finish;
      finish.m_pointType = RouteMarkType::Finish;
      finish.m_position = {2, 2};
      manager->AddRoutePoint(std::move(finish), false);
      manager->BuildRoute();
    }
    else if (code == routing::RouterResultCode::NoError)
    {
      [built fulfill];
    }
  };
  manager->BuildRoute();
  XCTAssertTrue(requested);
  XCTAssertTrue(manager->IsRouteBuilding());
  [self waitForExpectations:@[built] timeout:5];
  XCTAssertTrue(manager->IsRouteValid());
}

@end
