#include "testing/testing.hpp"

#include "routing/async_router.hpp"
#include "routing/router.hpp"
#include "routing/online_absent_fetcher.hpp"

#include "geometry/point2d.hpp"

#include "base/timer.hpp"

#include "std/condition_variable.hpp"
#include "std/mutex.hpp"
#include "std/string.hpp"
#include "std/vector.hpp"

using namespace routing;
using namespace std::placeholders;

using ResultCode = routing::IRouter::ResultCode;

namespace
{
class DummyRouter : public IRouter
{
  ResultCode m_result;
  vector<string> m_absent;

public:
  DummyRouter(ResultCode code, vector<string> const & absent) : m_result(code), m_absent(absent) {}
  
  // IRouter overrides:
  string GetName() const override { return "Dummy"; }
  ResultCode CalculateRoute(Checkpoints const & checkpoints, m2::PointD const & startDirection,
                            bool adjustToPrevRoute, RouterDelegate const & delegate,
                            Route & route) override
  {
    route = Route("dummy", checkpoints.GetPoints().cbegin(), checkpoints.GetPoints().cend());

    for (auto const & absent : m_absent)
      route.AddAbsentCountry(absent);

    return m_result;
  }
};

class DummyFetcher : public IOnlineFetcher
{
  vector<string> m_absent;

public:
  DummyFetcher(vector<string> const & absent) : m_absent(absent) {}

  // IOnlineFetcher overrides:
  void GenerateRequest(Checkpoints const &) override {}
  void GetAbsentCountries(vector<string> & countries) override { countries = m_absent; }
};

void DummyStatisticsCallback(map<string, string> const &) {}

struct DummyResultCallback
{
  vector<ResultCode> m_codes;
  vector<vector<string>> m_absent;
  condition_variable m_cv;
  mutex m_lock;
  uint32_t const m_expected;
  uint32_t m_called;

  DummyResultCallback(uint32_t expectedCalls) : m_expected(expectedCalls), m_called(0) {}

  void operator()(Route & route, ResultCode code)
  {
    m_codes.push_back(code);
    auto const & absent = route.GetAbsentCountries();
    m_absent.emplace_back(absent.begin(), absent.end());
    {
      lock_guard<mutex> calledLock(m_lock);
      ++m_called;
      TEST_LESS_OR_EQUAL(m_called, m_expected,
                         ("The result callback called more times than expected."));
    }
    m_cv.notify_all();
  }

  void WaitFinish()
  {
    unique_lock<mutex> lk(m_lock);
    return m_cv.wait(lk, [this] { return m_called == m_expected; });
  }
};

UNIT_TEST(NeedMoreMapsSignalTest)
{
  vector<string> const absentData({"test1", "test2"});
  unique_ptr<IOnlineFetcher> fetcher(new DummyFetcher(absentData));
  unique_ptr<IRouter> router(new DummyRouter(ResultCode::NoError, {}));
  DummyResultCallback resultCallback(2 /* expectedCalls */);
  AsyncRouter async(DummyStatisticsCallback, nullptr /* pointCheckCallback */);
  async.SetRouter(move(router), move(fetcher));
  async.CalculateRoute(Checkpoints({1, 2} /* start */, {5, 6} /* finish */), {3, 4}, false,
                       bind(ref(resultCallback), _1, _2), nullptr, 0);

  resultCallback.WaitFinish();

  TEST_EQUAL(resultCallback.m_codes.size(), 2, ());
  TEST_EQUAL(resultCallback.m_codes[0], ResultCode::NoError, ());
  TEST_EQUAL(resultCallback.m_codes[1], ResultCode::NeedMoreMaps, ());
  TEST_EQUAL(resultCallback.m_absent.size(), 2, ());
  TEST(resultCallback.m_absent[0].empty(), ());
  TEST_EQUAL(resultCallback.m_absent[1].size(), 2, ());
  TEST_EQUAL(resultCallback.m_absent[1], absentData, ());
}

UNIT_TEST(StandartAsyncFogTest)
{
  unique_ptr<IOnlineFetcher> fetcher(new DummyFetcher({}));
  unique_ptr<IRouter> router(new DummyRouter(ResultCode::NoError, {}));
  DummyResultCallback resultCallback(1 /* expectedCalls */);
  AsyncRouter async(DummyStatisticsCallback, nullptr /* pointCheckCallback */);
  async.SetRouter(move(router), move(fetcher));
  async.CalculateRoute(Checkpoints({1, 2} /* start */, {5, 6} /* finish */), {3, 4}, false,
                       bind(ref(resultCallback), _1, _2), nullptr, 0);

  resultCallback.WaitFinish();

  TEST_EQUAL(resultCallback.m_codes.size(), 1, ());
  TEST_EQUAL(resultCallback.m_codes[0], ResultCode::NoError, ());
  TEST_EQUAL(resultCallback.m_absent.size(), 1, ());
  TEST(resultCallback.m_absent[0].empty(), ());
}
}  //  namespace
