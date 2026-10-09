#include "testing/testing.hpp"

#include "routing/async_router.hpp"
#include "routing/route.hpp"
#include "routing/router.hpp"
#include "routing/routing_callbacks.hpp"
#include "routing/ruler_router.hpp"

#include "platform/platform_tests_support/async_gui_thread.hpp"

#include "base/scope_guard.hpp"

#include <chrono>
#include <condition_variable>
#include <future>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace async_router_test
{
using namespace routing;
using namespace std::placeholders;
using namespace std;

class DummyRouter : public IRouter
{
  RouterResultCode m_result;

public:
  DummyRouter(RouterResultCode code, set<string> const & absent) : m_result(code) {}

  // IRouter overrides:
  string GetName() const override { return "Dummy"; }
  void SetGuides(GuidesTracks && /* guides */) override {}
  RouterResultCode CalculateRoute(Checkpoints const & checkpoints, m2::PointD const & startDirection,
                                  bool adjustToPrevRoute, bool needAlternatives, RouterDelegate const & delegate,
                                  RoutesResult & result) override
  {
    Route route;
    route.SetGeometry(checkpoints.GetPoints().cbegin(), checkpoints.GetPoints().cend());
    result.MakeFrom(GetName(), std::move(route));
    return m_result;
  }

  bool FindClosestProjectionToRoad(m2::PointD const & point, m2::PointD const & direction, double radius,
                                   EdgeProj & proj) override
  {
    return false;
  }
};

struct DummyRoutingCallbacks
{
  vector<RouterResultCode> m_codes;
  vector<set<string>> m_absent;
  condition_variable m_cv;
  mutex m_lock;
  uint32_t const m_expected;
  uint32_t m_called;

  explicit DummyRoutingCallbacks(uint32_t expectedCalls) : m_expected(expectedCalls), m_called(0) {}

  // ReadyCallbackOwnership callback
  void operator()(shared_ptr<Route> route, RouterResultCode code)
  {
    CHECK(route, ());
    m_codes.push_back(code);
    TestAndNotifyReadyCallbacks();
  }

  // NeedMoreMapsCallback callback
  void operator()(uint64_t routeId, set<string> const & absent)
  {
    m_codes.push_back(RouterResultCode::NeedMoreMaps);
    m_absent.emplace_back(absent);
    TestAndNotifyReadyCallbacks();
  }

  void WaitFinish()
  {
    unique_lock<mutex> lk(m_lock);
    return m_cv.wait(lk, [this] { return m_called == m_expected; });
  }

  void TestAndNotifyReadyCallbacks()
  {
    {
      lock_guard<mutex> calledLock(m_lock);
      ++m_called;
      TEST_LESS_OR_EQUAL(m_called, m_expected, ("The result callback called more times than expected."));
    }
    m_cv.notify_all();
  }
};

// TODO(o.khlopkova) Uncomment and update these tests.

// UNIT_CLASS_TEST(AsyncGuiThreadTest, NeedMoreMapsSignalTest)
//{
//  set<string> const absentData({"test1", "test2"});
//  unique_ptr<IOnlineFetcher> fetcher(new DummyFetcher(absentData));
//  unique_ptr<IRouter> router(new DummyRouter(RouterResultCode::NoError, {}));
//  DummyRoutingCallbacks resultCallback(2 /* expectedCalls */);
//  AsyncRouter async(DummyStatisticsCallback, nullptr /* pointCheckCallback */);
//  async.SetRouter(std::move(router), std::move(fetcher));
//  async.CalculateRoute(Checkpoints({1, 2} /* start */, {5, 6} /* finish */), {3, 4}, false,
//                       bind(ref(resultCallback), _1, _2) /* readyCallback */,
//                       bind(ref(resultCallback), _1, _2) /* needMoreMapsCallback */,
//                       nullptr /* removeRouteCallback */, nullptr /* progressCallback */);
//
//  resultCallback.WaitFinish();
//
//  TEST_EQUAL(resultCallback.m_codes.size(), 2, ());
//  TEST_EQUAL(resultCallback.m_codes[0], RouterResultCode::NoError, ());
//  TEST_EQUAL(resultCallback.m_codes[1], RouterResultCode::NeedMoreMaps, ());
//  TEST_EQUAL(resultCallback.m_absent.size(), 2, ());
//  TEST(resultCallback.m_absent[0].empty(), ());
//  TEST_EQUAL(resultCallback.m_absent[1].size(), 2, ());
//  TEST_EQUAL(resultCallback.m_absent[1], absentData, ());
//}

// UNIT_CLASS_TEST(AsyncGuiThreadTest, StandardAsyncFogTest)
//{
//  unique_ptr<IOnlineFetcher> fetcher(new DummyFetcher({}));
//  unique_ptr<IRouter> router(new DummyRouter(RouterResultCode::NoError, {}));
//  DummyRoutingCallbacks resultCallback(1 /* expectedCalls */);
//  AsyncRouter async(DummyStatisticsCallback, nullptr /* pointCheckCallback */);
//  async.SetRouter(std::move(router), std::move(fetcher));
//  async.CalculateRoute(Checkpoints({1, 2} /* start */, {5, 6} /* finish */), {3, 4}, false,
//                       bind(ref(resultCallback), _1, _2), nullptr /* needMoreMapsCallback */,
//                       nullptr /* progressCallback */, nullptr /* removeRouteCallback */);
//
//  resultCallback.WaitFinish();
//
//  TEST_EQUAL(resultCallback.m_codes.size(), 1, ());
//  TEST_EQUAL(resultCallback.m_codes[0], RouterResultCode::NoError, ());
//  TEST_EQUAL(resultCallback.m_absent.size(), 1, ());
//  TEST(resultCallback.m_absent[0].empty(), ());
//}

struct RouterState
{
  mutex m_guard;
  condition_variable m_changed;
  vector<string> m_events;
  bool m_blockFirstCalculation = false;
  bool m_released = false;
  bool m_hasCache = false;

  void WaitForEvents(size_t count)
  {
    unique_lock lock(m_guard);
    TEST(m_changed.wait_for(lock, chrono::seconds(30), [&] { return m_events.size() >= count; }), ());
  }

  void Release()
  {
    lock_guard lock(m_guard);
    m_released = true;
    m_changed.notify_all();
  }
};

class CacheRouter : public IRouter
{
public:
  explicit CacheRouter(shared_ptr<RouterState> state) : m_state(std::move(state)) {}

  string GetName() const override { return "CacheRouter"; }
  void SetGuides(GuidesTracks &&) override {}
  bool FindClosestProjectionToRoad(m2::PointD const &, m2::PointD const &, double, EdgeProj &) override
  {
    return false;
  }

  void ClearState() override
  {
    lock_guard lock(m_state->m_guard);
    m_state->m_hasCache = false;
    m_state->m_events.emplace_back("clear");
    m_state->m_changed.notify_all();
  }

  RouterResultCode CalculateRoute(Checkpoints const & checkpoints, m2::PointD const & direction, bool adjust,
                                  bool needAlternatives, RouterDelegate const & delegate,
                                  RoutesResult & result) override
  {
    unique_lock lock(m_state->m_guard);
    bool const first = m_state->m_events.empty();
    m_state->m_events.emplace_back("calculate");
    m_state->m_changed.notify_all();
    if (first && m_state->m_blockFirstCalculation)
      CHECK(m_state->m_changed.wait_for(lock, chrono::seconds(30), [&] { return m_state->m_released; }), ());
    if (delegate.IsCancelled())
      return RouterResultCode::Cancelled;

    RulerRouter ruler;
    auto const code = static_cast<IRouter &>(ruler).CalculateRoute(checkpoints, direction, adjust, needAlternatives,
                                                                   delegate, result);
    m_state->m_hasCache = code == RouterResultCode::NoError;
    return code;
  }

private:
  shared_ptr<RouterState> m_state;
};

struct BuildRequest
{
  // Keep the promise alive when cancellation suppresses its callback.
  shared_ptr<promise<RouterResultCode>> m_ready = make_shared<promise<RouterResultCode>>();
  future<RouterResultCode> m_result = m_ready->get_future();

  void Start(AsyncRouter & router)
  {
    router.CalculateRoute(Checkpoints({0, 0}, {1, 1}), {}, false, true,
                          [ready = m_ready](shared_ptr<RoutesResult> const &, RouterResultCode code)
    { ready->set_value(code); }, nullptr, nullptr, nullptr);
  }

  void Wait()
  {
    TEST(m_result.wait_for(chrono::seconds(30)) == future_status::ready, ());
    TEST_EQUAL(m_result.get(), RouterResultCode::NoError, ());
  }

  bool IsReady() { return m_result.wait_for(chrono::seconds(0)) == future_status::ready; }
};

using AsyncGuiThreadTest = platform::tests_support::AsyncGuiThread;

UNIT_CLASS_TEST(AsyncGuiThreadTest, AsyncRouter_ClearThenLatestRequest)
{
  auto state = make_shared<RouterState>();
  state->m_blockFirstCalculation = true;
  BuildRequest cancelled;
  BuildRequest superseded;
  BuildRequest replacement;
  {
    AsyncRouter router(nullptr);
    router.SetRouter(make_unique<CacheRouter>(state), nullptr);
    SCOPE_GUARD(release, [&] { state->Release(); });
    cancelled.Start(router);
    state->WaitForEvents(1);
    router.ClearState();
    superseded.Start(router);
    replacement.Start(router);
    state->Release();
    replacement.Wait();
  }
  TEST(!cancelled.IsReady(), ());
  TEST(!superseded.IsReady(), ());
  TEST_EQUAL(state->m_events, (vector<string>{"calculate", "clear", "calculate"}), ());
  TEST(state->m_hasCache, ("The replacement's successful calculation must retain its caches."));
}

UNIT_CLASS_TEST(AsyncGuiThreadTest, AsyncRouter_ClearWithoutReplacement)
{
  auto state = make_shared<RouterState>();
  BuildRequest initial;
  BuildRequest replacement;
  {
    AsyncRouter router(nullptr);
    router.SetRouter(make_unique<CacheRouter>(state), nullptr);
    initial.Start(router);
    initial.Wait();
    {
      lock_guard lock(state->m_guard);
      TEST(state->m_hasCache, ());
    }
    router.ClearState();
    state->WaitForEvents(2);
    {
      lock_guard lock(state->m_guard);
      TEST(!state->m_hasCache, ());
      TEST_EQUAL(state->m_events, (vector<string>{"calculate", "clear"}), ());
    }
    replacement.Start(router);
    replacement.Wait();
  }
  TEST_EQUAL(state->m_events, (vector<string>{"calculate", "clear", "calculate"}), ());
  TEST(state->m_hasCache, ());
}

UNIT_CLASS_TEST(AsyncGuiThreadTest, AsyncRouter_ClearBeforeRouterReplacement)
{
  auto oldState = make_shared<RouterState>();
  oldState->m_blockFirstCalculation = true;
  auto newState = make_shared<RouterState>();
  BuildRequest cancelled;
  BuildRequest replacement;
  {
    AsyncRouter router(nullptr);
    router.SetRouter(make_unique<CacheRouter>(oldState), nullptr);
    SCOPE_GUARD(release, [&] { oldState->Release(); });
    cancelled.Start(router);
    oldState->WaitForEvents(1);
    router.ClearState();
    router.SetRouter(make_unique<CacheRouter>(newState), nullptr);
    replacement.Start(router);
    oldState->Release();
    replacement.Wait();
  }
  TEST(!cancelled.IsReady(), ());
  TEST_EQUAL(oldState->m_events, (vector<string>{"calculate"}), ());
  TEST_EQUAL(newState->m_events, (vector<string>{"clear", "calculate"}), ());
  TEST(newState->m_hasCache, ());
}
}  //  namespace async_router_test
