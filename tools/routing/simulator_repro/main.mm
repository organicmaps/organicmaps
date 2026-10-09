#import <UIKit/UIKit.h>

#include "base/assert.hpp"
#include "drape_frontend/visual_params.hpp"
#include "geometry/mercator.hpp"
#include "indexer/classificator_loader.hpp"
#include "indexer/data_source.hpp"
#include "map/routing_manager.hpp"
#include "map/transit/transit_reader.hpp"
#include "platform/platform.hpp"
#include "routing/index_router.hpp"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

namespace simulator_repro
{
using namespace routing;
using namespace std;
using namespace std::chrono_literals;

bool g_fixed = false;
mutex g_hookMutex;
function<void()> g_hook;

// Called only by the separately compiled, instrumented AsyncRouter object.
void BeforePickingRequest()
{
  function<void()> hook;
  {
    lock_guard lock(g_hookMutex);
    hook = g_hook;
  }
  if (hook)
    hook();
}

template <typename T>
T Await(future<T> & result)
{
  CHECK(result.wait_for(30s) == future_status::ready, ("Timed out waiting for the routing worker/main queue"));
  return result.get();
}

void OnMain(function<void()> const & task)
{
  CHECK(![NSThread isMainThread], ());
  dispatch_sync(dispatch_get_main_queue(), ^{ task(); });
}

struct Gate
{
  mutex guard;
  condition_variable changed;
  bool entered = false;
  bool released = false;

  void ParkOnce()
  {
    unique_lock lock(guard);
    if (entered)
      return;
    entered = true;
    changed.notify_all();
    CHECK(changed.wait_for(lock, 30s, [this] { return released; }), ("Gate was never released"));
  }
  void Wait()
  {
    unique_lock lock(guard);
    CHECK(changed.wait_for(lock, 30s, [this] { return entered; }), ("Worker never reached the gate"));
  }
  void Release()
  {
    lock_guard lock(guard);
    released = true;
    changed.notify_all();
  }
};

Route MakeRoute(Checkpoints const & checkpoints)
{
  auto const & points = checkpoints.GetPoints();
  Route route;
  route.SetGeometry(points.cbegin(), points.cend());
  vector<RouteSegment> segments;
  vector<Route::SubrouteAttrs> subroutes;
  double meters = 0;
  double merc = 0;
  for (size_t i = 1; i < points.size(); ++i)
  {
    auto const start = geometry::PointWithAltitude(points[i - 1], geometry::kDefaultAltitudeMeters);
    auto const finish = geometry::PointWithAltitude(points[i], geometry::kDefaultAltitudeMeters);
    meters += mercator::DistanceOnEarth(points[i - 1], points[i]);
    merc += (points[i] - points[i - 1]).Length();
    auto const turn = turns::TurnItem(
        i, i + 1 == points.size() ? turns::CarDirection::ReachedYourDestination : turns::CarDirection::None);
    segments.emplace_back(Segment(0, 0, i - 1, true), turn, finish, RouteSegment::RoadNameInfo());
    segments.back().SetDistancesAndTime(meters, merc, meters / 10.0);
    subroutes.emplace_back(start, finish, i - 1, i);
  }
  route.SetRouteSegments(std::move(segments));
  route.SetSubroutes(std::move(subroutes), checkpoints.GetPassedIdx());
  route.SetDiffMidpoint(points[1]);
  return route;
}

class ProbeRouter : public IRouter
{
public:
  string GetName() const override { return "simulator reproduction"; }
  void ClearState() override { ++clears; }
  void SetGuides(GuidesTracks &&) override {}
  void SwapAltRouteToActive() override { ++swaps; }
  bool FindClosestProjectionToRoad(m2::PointD const &, m2::PointD const &, double, EdgeProj &) override
  {
    ++projections;
    return false;
  }
  RouterResultCode CalculateRoute(Checkpoints const & checkpoints, m2::PointD const &, bool, bool alternatives,
                                  RouterDelegate const &, RoutesResult & result) override
  {
    auto const call = ++calls;
    if (blockedCall == call)
      gate.ParkOnce();
    result.MakeFrom(GetName(), MakeRoute(checkpoints));
    if (alternatives)
    {
      auto alternative = MakeRoute(checkpoints);
      result.m_routes.emplace_back(std::move(static_cast<RouteBase &>(alternative)));
    }
    return RouterResultCode::NoError;
  }
  atomic<size_t> calls{0}, clears{0}, projections{0}, swaps{0};
  size_t blockedCall = 0;
  Gate gate;
};

struct Request
{
  shared_ptr<promise<uint64_t>> ready = make_shared<promise<uint64_t>>();
  future<uint64_t> result = ready->get_future();
  void Start(AsyncRouter & async)
  {
    async.CalculateRoute(Checkpoints({0, .001}, {0, .006}), {}, false, true,
                         [ready = ready](shared_ptr<RoutesResult> const & result, RouterResultCode code)
    {
      CHECK_EQUAL(code, RouterResultCode::NoError, ());
      ready->set_value(result->m_routesId);
    }, nullptr, nullptr, nullptr);
  }
};

void ClearGap()
{
  auto gate = make_shared<Gate>();
  {
    lock_guard lock(g_hookMutex);
    g_hook = [gate] { gate->ParkOnce(); };
  }
  auto router = make_unique<ProbeRouter>();
  auto * raw = router.get();
  {
    AsyncRouter async(nullptr);
    async.SetRouter(std::move(router), nullptr);
    Request first;
    first.Start(async);
    gate->Wait();
    async.ClearState();
    Request replacement;
    replacement.Start(async);
    gate->Release();
    auto const id = Await(replacement.result);
    auto const deadline = chrono::steady_clock::now() + 30s;
    while (raw->clears == 0 && chrono::steady_clock::now() < deadline)
      this_thread::sleep_for(1ms);
    CHECK_EQUAL(raw->calls.load(), 1, ());
    CHECK_EQUAL(raw->clears.load(), 1, ());
    CHECK(first.result.wait_for(0s) != future_status::ready, ("Superseded request must stay cancelled"));
    bool const promoted = async.SwapAltRouteToActive(id);
    CHECK_EQUAL(promoted, g_fixed, ());
    printf("ISSUE 13738: NoError=1 calculations=%zu clears=%zu cache_promotable=%d\n", raw->calls.load(),
           raw->clears.load(), promoted);
  }
  lock_guard lock(g_hookMutex);
  g_hook = {};
}

void InitSession(RoutingSession & session)
{
  session.Init(nullptr);
  session.SetRoutingSettings(GetRoutingSettings(VehicleType::Car));
  session.SetOnNewTurnCallback([] {});
}

void CheckpointProgress()
{
  unique_ptr<RoutingSession> session;
  ProbeRouter * raw = nullptr;
  size_t checkpoint = 0;
  auto built = make_shared<promise<void>>();
  auto ready = built->get_future();
  OnMain([&]
  {
    session = make_unique<RoutingSession>();
    InitSession(*session);
    auto router = make_unique<ProbeRouter>();
    raw = router.get();
    raw->blockedCall = 2;
    session->SetRouter(std::move(router), nullptr);
    session->SetCheckpointCallback([&](size_t passed) { checkpoint = passed; });
    session->SetRoutingCallbacks([built](RoutesResult const &, RouterResultCode code)
    {
      CHECK_EQUAL(code, RouterResultCode::NoError, ());
      built->set_value();
    }, nullptr, nullptr, nullptr);
    session->BuildRoute(Checkpoints(CheckpointsGeometry{{0, .001}, {0, .003}, {0, .006}}), RouterDelegate::kNoTimeout);
  });
  Await(ready);
  auto rebuilt = make_shared<promise<size_t>>();
  auto finished = rebuilt->get_future();
  OnMain([&]
  {
    CHECK(session->EnableFollowMode(), ());
    location::GpsInfo gps;
    gps.m_latitude = mercator::YToLat(.002);
    gps.m_longitude = 0;
    gps.m_horizontalAccuracy = 1;
    session->OnLocationPositionChanged(gps);
    session->RebuildRoute({0, .002}, [rebuilt](RoutesResult const & result, RouterResultCode code)
    {
      CHECK_EQUAL(code, RouterResultCode::NoError, ());
      rebuilt->set_value(result.GetActive().GetCurrentSubrouteIdx());
    }, nullptr, nullptr, RouterDelegate::kNoTimeout, SessionState::RouteRebuilding, true);
  });
  raw->gate.Wait();
  OnMain([&]
  {
    location::GpsInfo gps;
    gps.m_latitude = mercator::YToLat(.003);
    gps.m_longitude = 0;
    gps.m_horizontalAccuracy = 1;
    session->OnLocationPositionChanged(gps);
    CHECK_EQUAL(checkpoint, 1, ());
    CHECK_EQUAL(session->GetRoute()->GetCurrentSubrouteIdx(), 1, ());
  });
  raw->gate.Release();
  size_t const cursor = Await(finished);
  CHECK_EQUAL(cursor, g_fixed ? 1 : 0, ());
  OnMain([&]
  {
    CHECK_EQUAL(checkpoint, 1, ());
    CHECK_EQUAL(session->GetRoute()->GetCurrentSubrouteIdx(), cursor, ());
    printf("ISSUE 13739: checkpoint_passed=%zu cursor_before=1 cursor_after=%zu\n", checkpoint, cursor);
    session.reset();
  });
}

class Delegate final : public RoutingManager::Delegate
{
public:
  void OnRouteFollow(RouterType) override {}
};

void QueuedBalloons(bool reset)
{
  Delegate delegate;
  unique_ptr<BookmarkManager> bookmarks;
  unique_ptr<RoutingManager> manager;
  FrozenDataSource data;
  unique_ptr<TransitReadManager> transit;
  auto observed = make_shared<promise<pair<bool, size_t>>>();
  auto finished = observed->get_future();
  OnMain([&]
  {
    df::VisualParams::Init(1.0, 256);
    bookmarks = make_unique<BookmarkManager>(BookmarkManager::Callbacks([]() -> StringsBundle const &
    {
      static StringsBundle const bundle;
      return bundle;
    }, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr));
    transit = make_unique<TransitReadManager>(data, nullptr, nullptr);
    manager =
        make_unique<RoutingManager>(RoutingManager::Callbacks(nullptr, nullptr, nullptr, nullptr, nullptr), delegate);
    manager->m_currentRouterType = RouterType::Vehicle;
    manager->SetBookmarkManager(bookmarks.get());
    manager->SetTransitManager(transit.get());
    auto & session = manager->RoutingSession();
    session.SetRouter(make_unique<ProbeRouter>(), nullptr);
    session.SetRoutingCallbacks([&, observed, reset](RoutesResult const & result, RouterResultCode code)
    {
      CHECK([NSThread isMainThread], ("This reproduction requires the native iOS main queue"));
      CHECK_EQUAL(code, RouterResultCode::NoError, ());
      manager->CreateRouteAltMarks(result);
      if (reset)
      {
        manager->RoutingSession().Reset();
        manager->ClearAlternativeRoutes();
      }
      else
      {
        manager->FollowRoute();
      }
      CHECK_EQUAL(bookmarks->GetUserMarkIds(UserMark::Type::ROUTE_ALT).size(), 0, ());
      GetPlatform().RunTask(Platform::Thread::Gui, [&, observed]
      {
        observed->set_value(
            {manager->RoutingSession().IsFollowing(), bookmarks->GetUserMarkIds(UserMark::Type::ROUTE_ALT).size()});
      });
    }, nullptr, nullptr, nullptr);
    session.BuildRoute(Checkpoints({0, .001}, {0, .006}), RouterDelegate::kNoTimeout);
  });
  auto const [following, marks] = Await(finished);
  CHECK_EQUAL(following, !reset, ());
  CHECK_EQUAL(marks, reset || g_fixed ? 0 : 2, ());
  printf("ISSUE 13740%s: main_queue=1 following=%d marks_after_cleanup=0 marks_after_queue=%zu\n",
         reset ? " reset_control" : "", following, marks);
  OnMain([&]
  {
    manager.reset();
    transit.reset();
    bookmarks.reset();
  });
}

class TrafficRouter final : public ProbeRouter
{
public:
  explicit TrafficRouter(shared_ptr<promise<EdgeEstimator::Strategy>> observed)
    : m_ids(make_shared<NumMwmIds>())
    , m_observed(std::move(observed))
  {
    m_ids->RegisterFile(platform::CountryFile("Germany"));
    m_index = make_unique<IndexRouter>(VehicleType::Car, false, [](string const &) { return string(); },
                                       [](m2::PointD const &) { return string("Germany"); }, [](string const &)
    { return m2::RectD(); }, m_ids, make_shared<m4::Tree<NumMwmId>>(), m_traffic, m_data);
  }
  void ClearState() override
  {
    ++clears;
    m_index->ClearState();
  }
  void SwapAltRouteToActive() override
  {
    ++swaps;
    m_index->SwapAltRouteToActive();
    CHECK(m_index->m_activeStrategy == EdgeEstimator::Strategy::DistanceBiased, ());
  }
  RouterResultCode CalculateRoute(Checkpoints const & checkpoints, m2::PointD const & direction, bool adjust,
                                  bool alternatives, RouterDelegate const & delegate, RoutesResult & result) override
  {
    auto const code = ProbeRouter::CalculateRoute(checkpoints, direction, adjust, alternatives, delegate, result);
    if (calls == 2)
    {
      CHECK(!adjust, ("Traffic must force a full calculation"));
      m_observed->set_value(m_index->m_activeStrategy);
    }
    return code;
  }

private:
  FrozenDataSource m_data;
  traffic::TrafficCache m_traffic;
  shared_ptr<NumMwmIds> m_ids;
  unique_ptr<IndexRouter> m_index;
  shared_ptr<promise<EdgeEstimator::Strategy>> m_observed;
};

void TrafficSelection()
{
  unique_ptr<RoutingSession> session;
  auto observed = make_shared<promise<EdgeEstimator::Strategy>>();
  auto outcome = observed->get_future();
  auto built = make_shared<promise<void>>();
  auto ready = built->get_future();
  auto rebuilt = make_shared<promise<void>>();
  auto finished = rebuilt->get_future();
  OnMain([&]
  {
    classificator::Load();
    session = make_unique<RoutingSession>();
    InitSession(*session);
    session->SetRouter(make_unique<TrafficRouter>(observed), nullptr);
    session->SetRoutingCallbacks(
        [built](RoutesResult const &, RouterResultCode code)
    {
      CHECK_EQUAL(code, RouterResultCode::NoError, ());
      built->set_value();
    }, [rebuilt](RoutesResult const &, RouterResultCode code)
    {
      CHECK_EQUAL(code, RouterResultCode::NoError, ());
      rebuilt->set_value();
    }, nullptr, nullptr);
    session->BuildRoute(Checkpoints({0, .001}, {0, .006}), RouterDelegate::kNoTimeout);
  });
  Await(ready);
  OnMain([&]
  {
    CHECK(session->SwapActiveAlternative(1), ());
    session->OnTrafficInfoClear();
  });
  auto const strategy = Await(outcome);
  CHECK(strategy == (g_fixed ? EdgeEstimator::Strategy::DistanceBiased : EdgeEstimator::Strategy::Normal), ());
  Await(finished);
  printf("ISSUE 13741: selected=DistanceBiased traffic_adjust=0 rebuilt_strategy=%s\n",
         strategy == EdgeEstimator::Strategy::DistanceBiased ? "DistanceBiased" : "Normal");
  OnMain([&] { session.reset(); });
}

void Run()
{
  @autoreleasepool
  {
    setbuf(stdout, nullptr);
    printf("SIMULATOR_REPRO mode=%s ios=%s arch=arm64 base=7d9cca20d11befd6fe97acbd5b7a065fd3916b54\n",
           g_fixed ? "candidate-fixes" : "baseline", UIDevice.currentDevice.systemVersion.UTF8String);
    ClearGap();
    CheckpointProgress();
    QueuedBalloons(false);
    QueuedBalloons(true);
    TrafficSelection();
    printf("SIMULATOR_REPRO PASS: 4 issues plus reset control\n");
    exit(0);
  }
}
}  // namespace simulator_repro

@interface ReproAppDelegate : UIResponder <UIApplicationDelegate>
@end

@interface ReproSceneDelegate : UIResponder <UIWindowSceneDelegate>
@property(nonatomic, strong) UIWindow * window;
@end

@implementation ReproAppDelegate
- (UISceneConfiguration *)application:(UIApplication *)application
    configurationForConnectingSceneSession:(UISceneSession *)session
                                   options:(UISceneConnectionOptions *)options
{
  UISceneConfiguration * configuration = [[UISceneConfiguration alloc] initWithName:@"Reproduction"
                                                                        sessionRole:session.role];
  configuration.delegateClass = ReproSceneDelegate.class;
  return configuration;
}
@end

@implementation ReproSceneDelegate
- (void)scene:(UIScene *)scene
    willConnectToSession:(UISceneSession *)session
                 options:(UISceneConnectionOptions *)options
{
  static Platform::ThreadRunner runner;
  self.window = [[UIWindow alloc] initWithWindowScene:(UIWindowScene *)scene];
  self.window.rootViewController = [UIViewController new];
  [self.window makeKeyAndVisible];
  std::thread(&simulator_repro::Run).detach();
}
@end

int main(int argc, char * argv[])
{
  @autoreleasepool
  {
    simulator_repro::g_fixed = argc > 1 && std::string(argv[1]) == "--fixed";
    return UIApplicationMain(argc, argv, nil, NSStringFromClass(ReproAppDelegate.class));
  }
}
