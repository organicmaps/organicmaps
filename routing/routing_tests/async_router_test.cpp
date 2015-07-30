#include "testing/testing.hpp"

#include "routing/async_router.hpp"
#include "routing/router.hpp"
#include "routing/online_absent_fetcher.hpp"

#include "geometry/point2d.hpp"

#include "base/timer.hpp"

#include "std/string.hpp"
#include "std/vector.hpp"

using namespace routing;

using ResultCode = routing::IRouter::ResultCode;

namespace
{
class DummyRouter : public IRouter
{
  ResultCode m_result;
  vector<string> m_absent;

public:
  DummyRouter(ResultCode code, vector<string> const & absent) : m_result(code), m_absent(absent) {}

  string GetName() const override { return "Dummy"; }

  ResultCode CalculateRoute(m2::PointD const & startPoint, m2::PointD const & startDirection,
                            m2::PointD const & finalPoint, RouterDelegate const & delegate,
                            Route & route) override
  {
    vector<m2::PointD> points({startPoint, finalPoint});
    route = Route("dummy", points.begin(), points.end());

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
  void GenerateRequest(m2::PointD const & startPoint, m2::PointD const & finalPoint) override {}
  void GetAbsentCountries(vector<string> & countries) override { countries = m_absent; }
};

void DummyStatisticsCallback(map<string, string> const &) {}

struct DummyResultCallback
{
  vector<ResultCode> m_codes;
  vector<vector<string>> m_absent;
  void operator()(Route & route, ResultCode code)
  {
    m_codes.push_back(code);
    auto const & absent = route.GetAbsentCountries();
    m_absent.emplace_back(absent.begin(), absent.end());
  }
};

UNIT_TEST(NeedMoreMapsSignalTest)
{
  unique_ptr<IOnlineFetcher> fetcher(new DummyFetcher({"test1", "test2"}));
  unique_ptr<IRouter> router(new DummyRouter(ResultCode::NoError, {}));
  DummyResultCallback resultCallback;
  AsyncRouter async(move(router), move(fetcher), DummyStatisticsCallback, nullptr);
  resultCallback.m_codes.clear();
  resultCallback.m_absent.clear();
  async.CalculateRoute({1, 2}, {3, 4}, {5, 6}, bind(ref(resultCallback), _1, _2), nullptr, 0);

  // Wait async process start.
  while (resultCallback.m_codes.empty())
  {
  }

  async.WaitRouting();

  TEST_EQUAL(resultCallback.m_codes.size(), 2, ());
  TEST_EQUAL(resultCallback.m_codes[0], ResultCode::NoError, ());
  TEST_EQUAL(resultCallback.m_codes[1], ResultCode::NeedMoreMaps, ());
  TEST_EQUAL(resultCallback.m_absent.size(), 2, ());
  TEST(resultCallback.m_absent[0].empty(), ());
  TEST_EQUAL(resultCallback.m_absent[1].size(), 2, ())
}

UNIT_TEST(StandartAsyncFogTest)
{
  unique_ptr<IOnlineFetcher> fetcher(new DummyFetcher({}));
  unique_ptr<IRouter> router(new DummyRouter(ResultCode::NoError, {}));
  DummyResultCallback resultCallback;
  AsyncRouter async(move(router), move(fetcher), DummyStatisticsCallback, nullptr);
  resultCallback.m_codes.clear();
  resultCallback.m_absent.clear();
  async.CalculateRoute({1, 2}, {3, 4}, {5, 6}, bind(ref(resultCallback), _1, _2), nullptr, 0);

  // Wait async process start.
  while (resultCallback.m_codes.empty())
  {
  }

  async.WaitRouting();

  TEST_EQUAL(resultCallback.m_codes.size(), 1, ());
  TEST_EQUAL(resultCallback.m_codes[0], ResultCode::NoError, ());
  TEST_EQUAL(resultCallback.m_absent.size(), 1, ());
  TEST(resultCallback.m_absent[0].empty(), ());
}
}  //  namespace
