#import <XCTest/XCTest.h>

#import "MWMFrameworkListener.h"
#import "MWMFrameworkObservers.h"
#import "MWMRouter.h"

#include <CoreApi/Framework.h>

#include <atomic>
#include <chrono>
#include <memory>
#include <thread>

@interface MWMRouter (Testing)
+ (instancetype)router;
@property(nonatomic) BOOL startNavigationAfterBuild;
@end

namespace
{
class GatedRouter final : public routing::IRouter
{
public:
  explicit GatedRouter(std::shared_ptr<std::atomic<bool>> release) : m_release(std::move(release)) {}
  std::string GetName() const override { return "gated test router"; }
  void ClearState() override {}
  void SetGuides(routing::GuidesTracks &&) override {}
  bool FindClosestProjectionToRoad(m2::PointD const &, m2::PointD const &, double, routing::EdgeProj &) override
  {
    return false;
  }
  routing::RouterResultCode CalculateRoute(routing::Checkpoints const &, m2::PointD const &, bool, bool,
                                           routing::RouterDelegate const & delegate, routing::RoutesResult &) override
  {
    // Keep the current build pending while its predecessor's queued UI events are delivered.
    while (!m_release->load() && !delegate.IsCancelled())
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    return routing::RouterResultCode::Cancelled;
  }

private:
  std::shared_ptr<std::atomic<bool>> m_release;
};
}  // namespace

@interface MWMRoutingEventObserver : NSObject <MWMFrameworkRouteBuilderObserver>
@property(nonatomic) NSMutableArray<NSNumber *> * codes;
@end

@implementation MWMRoutingEventObserver
- (instancetype)init
{
  self = [super init];
  if (self)
    _codes = [NSMutableArray array];
  return self;
}
- (void)processRouteBuilderEvent:(routing::RouterResultCode)code countries:(storage::CountriesSet const &)countries
{
  [self.codes addObject:@(static_cast<int>(code))];
}
@end

@interface MWMFrameworkListenerTests : XCTestCase
{
  MWMRouterType m_previousRouter;
  m2::PointD m_previousPosition;
  bool m_hadPosition;
  std::shared_ptr<std::atomic<bool>> m_release;
}
@property(nonatomic) MWMRoutingEventObserver * observer;
@end

@implementation MWMFrameworkListenerTests
- (void)setUp
{
  [super setUp];
  m_previousRouter = [MWMRouter type];
  auto const & position = GetFramework().GetBookmarkManager().MyPositionMark();
  m_previousPosition = position.GetPivot();
  m_hadPosition = position.HasPosition();
  [MWMRouter stopRouting];
  [MWMRouter setType:MWMRouterTypeVehicle];
  [MWMRouter subscribeToEvents];
  self.observer = [[MWMRoutingEventObserver alloc] init];
  [MWMFrameworkListener addObserver:self.observer];
  [self drainNotifications];
  [self.observer.codes removeAllObjects];
}

- (void)tearDown
{
  if (m_release)
    m_release->store(true);
  [MWMRouter stopRouting];
  [MWMFrameworkListener removeObserver:self.observer];
  // Restore a native router even when the saved type matches the gated router's type.
  [MWMRouter setType:MWMRouterTypeRuler];
  [MWMRouter setType:m_previousRouter];
  GetFramework().GetBookmarkManager().MyPositionMark().SetUserPosition(m_previousPosition, m_hadPosition);
  [self drainNotifications];
  self.observer = nil;
  m_release.reset();
  [super tearDown];
}

- (void)drainNotifications
{
  XCTestExpectation * delivered = [self expectationWithDescription:@"Queued routing notifications delivered"];
  dispatch_async(dispatch_get_main_queue(), ^{ [delivered fulfill]; });
  [self waitForExpectations:@[delivered] timeout:2];
}

- (void)buildInvalidPreview
{
  auto const type = GetFramework().ParseAndSetApiURL("om://v2/dir?origin=1,1&destination=1,1");
  XCTAssertTrue(type == url_scheme::ParsedMapApi::UrlType::Route);
  [MWMRouter buildApiRouteWithType:MWMRouterTypeVehicle startRouteNavigation:NO];
}

- (void)holdRouteBuild
{
  m_release = std::make_shared<std::atomic<bool>>(false);
  GetFramework().GetRoutingManager().RoutingSession().SetRouter(std::make_unique<GatedRouter>(m_release), nullptr);
  GetFramework().GetBookmarkManager().MyPositionMark().SetUserPosition({1, 1}, true);
}

- (void)testObsoleteErrorDoesNotConsumeNewApiNavigationIntent
{
  [self buildInvalidPreview];
  [self holdRouteBuild];
  auto const type = GetFramework().ParseAndSetApiURL("om://v2/nav?destination=2,2");
  XCTAssertTrue(type == url_scheme::ParsedMapApi::UrlType::Route);
  [MWMRouter buildApiRouteWithType:MWMRouterTypeVehicle startRouteNavigation:YES];

  [self drainNotifications];

  XCTAssertTrue(GetFramework().GetRoutingManager().IsRouteBuilding());
  XCTAssertTrue([MWMRouter router].startNavigationAfterBuild);
  XCTAssertEqual(self.observer.codes.count, 0);
}

- (void)testIdenticalApiRequestsDeliverOnlyTheCurrentFailure
{
  [self buildInvalidPreview];
  [self buildInvalidPreview];

  [self drainNotifications];

  XCTAssertEqualObjects(self.observer.codes, (@[@(static_cast<int>(routing::RouterResultCode::Cancelled))]));
}

- (void)testUnrelatedMapLinkPreservesTheCurrentRouteEvent
{
  [self buildInvalidPreview];
  auto const type = GetFramework().ParseAndSetApiURL("om://map?ll=3,3");
  XCTAssertTrue(type == url_scheme::ParsedMapApi::UrlType::Map);

  [self drainNotifications];

  XCTAssertEqualObjects(self.observer.codes, (@[@(static_cast<int>(routing::RouterResultCode::Cancelled))]));
}

- (void)testStopDiscardsQueuedRouteEvents
{
  [self buildInvalidPreview];
  [MWMRouter stopRouting];

  [self drainNotifications];

  XCTAssertEqual(self.observer.codes.count, 0);
}

- (void)testPositionRetryDiscardsTheQueuedNoPositionError
{
  GetFramework().GetBookmarkManager().MyPositionMark().SetUserPosition({1, 1}, false);
  auto const type = GetFramework().ParseAndSetApiURL("om://v2/nav?destination=2,2");
  XCTAssertTrue(type == url_scheme::ParsedMapApi::UrlType::Route);
  [MWMRouter buildApiRouteWithType:MWMRouterTypeVehicle startRouteNavigation:YES];

  [self holdRouteBuild];
  GetFramework().GetRoutingManager().BuildRoute();
  [self drainNotifications];

  XCTAssertTrue(GetFramework().GetRoutingManager().IsRouteBuilding());
  XCTAssertTrue([MWMRouter router].startNavigationAfterBuild);
  XCTAssertEqual(self.observer.codes.count, 0);
}
@end
