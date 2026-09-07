#include "testing/testing.hpp"

#include "routing/async_router.hpp"
#include "routing/route.hpp"
#include "routing/router.hpp"
#include "routing/routing_callbacks.hpp"

#include "routing/routing_tests/tools.hpp"

#include "base/scope_guard.hpp"

#include <chrono>
#include <condition_variable>
#include <future>
#include <memory>
#include <mutex>
#include <string>

namespace async_router_test
{
using namespace routing;
using namespace std;

// Parks inside CalculateRoute until the test releases it and records projection calls.
class BlockingRouter : public IRouter
{
public:
  string GetName() const override { return "blocking"; }
  void ClearState() override {}
  void SetGuides(GuidesTracks && /* guides */) override {}

  RouterResultCode CalculateRoute(Checkpoints const & checkpoints, m2::PointD const & /* startDirection */,
                                  RouteAdjustmentContextPtr const & /* adjustmentContext */,
                                  RouterDelegate const & /* delegate */, RoutesResult & result) override
  {
    Notify(m_started);
    {
      unique_lock l(m_mutex);
      m_cv.wait(l, [this] { return m_release; });
    }

    Route route;
    route.SetGeometry(checkpoints.GetPoints().cbegin(), checkpoints.GetPoints().cend());
    result.MakeFrom(GetName(), std::move(route));
    return RouterResultCode::NoError;
  }

  bool FindClosestProjectionToRoad(m2::PointD const & /* point */, m2::PointD const & /* direction */,
                                   double /* radius */, EdgeProj & /* proj */) override
  {
    ++m_projections;
    return false;
  }

  bool WaitStarted(std::chrono::seconds timeout)
  {
    unique_lock l(m_mutex);
    return m_cv.wait_for(l, timeout, [this] { return m_started; });
  }

  void Release() { Notify(m_release); }

  size_t m_projections = 0;

private:
  void Notify(bool & flag)
  {
    {
      lock_guard l(m_mutex);
      flag = true;
    }
    m_cv.notify_all();
  }

  mutex m_mutex;
  condition_variable m_cv;
  bool m_started = false;
  bool m_release = false;
};

// CalculateRoute and the synchronous projection lookup share road-graph scratch state, so
// AsyncRouter must reject the lookup until the calculation releases the router.
UNIT_CLASS_TEST(AsyncGuiThreadTest, ProjectionWhileCalculating)
{
  auto router = make_unique<BlockingRouter>();
  auto * blocking = router.get();

  auto readyPromise = make_shared<promise<void>>();
  auto readyFuture = readyPromise->get_future();

  AsyncRouter async(nullptr /* pointCheckCallback */);
  async.SetRouter(std::move(router), nullptr /* absentRegionsFinder */);
  async.CalculateRoute(Checkpoints({1, 2} /* start */, {5, 6} /* finish */), {3, 4} /* direction */,
                       nullptr /* adjustmentContext */, [readyPromise](shared_ptr<RoutesResult>, RouterResultCode) {
    readyPromise->set_value();
  }, nullptr /* needMoreMapsCallback */, nullptr /* removeRouteCallback */, nullptr /* progressCallback */);

  // ~AsyncRouter joins the routing thread, so the router must be let go even if a TEST throws.
  SCOPE_GUARD(releaseRouter, [blocking] { blocking->Release(); });
  TEST(blocking->WaitStarted(std::chrono::seconds(30)), ("Route calculation did not start."));

  EdgeProj proj;
  m2::PointD const point(1.0, 2.0);
  m2::PointD const direction(0.0, 1.0);
  TEST(!async.FindClosestProjectionToRoad(point, direction, 50.0 /* radius */, proj),
       ("The projection lookup must not touch a calculating router."));
  TEST_EQUAL(blocking->m_projections, 0, ());

  blocking->Release();
  TEST(readyFuture.wait_for(std::chrono::seconds(30)) == future_status::ready, ("Route was not built."));

  // The mock finds nothing; reaching it once the router is idle is what matters.
  async.FindClosestProjectionToRoad(point, direction, 50.0 /* radius */, proj);
  TEST_EQUAL(blocking->m_projections, 1, ());
}
}  //  namespace async_router_test
