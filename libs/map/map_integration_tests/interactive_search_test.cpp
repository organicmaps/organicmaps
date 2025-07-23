#include "testing/testing.hpp"

#include "map/viewport_search_callback.hpp"

#include "search/mode.hpp"
#include "search/search_tests_support/helpers.hpp"
#include "search/search_tests_support/test_results_matching.hpp"
#include "search/search_tests_support/test_search_request.hpp"

#include "base/macros.hpp"

#include <functional>

namespace interactive_search_test
{
using namespace generator::tests_support;
using namespace search::tests_support;
using namespace search;
using namespace std;

struct Stats
{
  size_t m_numShownResults = 0;
};

class TestDelegate : public ViewportSearchCallback::Delegate
{
public:
  explicit TestDelegate(Stats & stats) : m_stats(stats) {}

  ~TestDelegate() override = default;

  // ViewportSearchCallback::Delegate overrides:
  void RunUITask(function<void()> fn) override { fn(); }

  bool IsViewportSearchActive() const override { return true; }

  void ShowViewportSearchResults(Results::ConstIter begin, Results::ConstIter end, bool clear) override
  {
    if (clear)
      m_stats.m_numShownResults = 0;
    m_stats.m_numShownResults += distance(begin, end);
  }

private:
  Stats & m_stats;
};

class InteractiveSearchRequest
  : public TestDelegate
  , public TestSearchRequest
{
public:
  InteractiveSearchRequest(TestSearchEngine & engine, string const & query, m2::RectD const & viewport, Stats & stats)
    : TestDelegate(stats)
    , TestSearchRequest(engine, query, "en" /* locale */, Mode::Viewport, viewport)
  {
    SetCustomOnResults(ViewportSearchCallback(viewport, static_cast<ViewportSearchCallback::Delegate &>(*this),
                                              bind(&InteractiveSearchRequest::OnResults, this, placeholders::_1)));
  }
};

class InteractiveSearchTest : public SearchTest
{};

double const kDX[] = {-0.01, 0, 0, 0.01};
double const kDY[] = {0, -0.01, 0.01, 0};

static_assert(ARRAY_SIZE(kDX) == ARRAY_SIZE(kDY), "Wrong deltas lists");

UNIT_CLASS_TEST(InteractiveSearchTest, Smoke)
{
  m2::PointD const cafesPivot(-1, -1);
  m2::PointD const hotelsPivot(1, 1);

  vector<TestCafe> cafes;
  for (size_t i = 0; i < ARRAY_SIZE(kDX); ++i)
    cafes.emplace_back(m2::Shift(cafesPivot, kDX[i], kDY[i]));

  vector<TestHotel> hotels;
  for (size_t i = 0; i < ARRAY_SIZE(kDX); ++i)
    hotels.emplace_back(m2::Shift(hotelsPivot, kDX[i], kDY[i]));

  auto const id = BuildCountry("Wonderland", [&](TestMwmBuilder & builder)
  {
    for (auto const & cafe : cafes)
      builder.Add(cafe);
    for (auto const & hotel : hotels)
      builder.Add(hotel);
  });

  {
    Stats stats;
    InteractiveSearchRequest request(m_engine, "cafe", m2::RectD(m2::PointD(-1.5, -1.5), m2::PointD(-0.5, -0.5)),
                                     stats);
    request.Run();

    Rules const rules = {ExactMatch(id, cafes[0]), ExactMatch(id, cafes[1]), ExactMatch(id, cafes[2]),
                         ExactMatch(id, cafes[3])};

    TEST_EQUAL(stats.m_numShownResults, 4, ());
    TEST(MatchResults(m_dataSource, rules, request.Results()), ());
  }

  {
    Stats stats;
    InteractiveSearchRequest request(m_engine, "hotel", m2::RectD(m2::PointD(0.5, 0.5), m2::PointD(1.5, 1.5)), stats);
    request.Run();

    Rules const rules = {ExactMatch(id, hotels[0]), ExactMatch(id, hotels[1]), ExactMatch(id, hotels[2]),
                         ExactMatch(id, hotels[3])};

    TEST_EQUAL(stats.m_numShownResults, 4, ());
    TEST(MatchResults(m_dataSource, rules, request.Results()), ());
  }
}

UNIT_CLASS_TEST(InteractiveSearchTest, NearbyFeaturesInViewport)
{
  static double constexpr kEps = 0.1;
  TestCafe cafe1(m2::PointD(0.0, 0.0));
  TestCafe cafe2(m2::PointD(0.0, kEps));
  TestCafe cafe3(m2::PointD(0.0, 2 * kEps));

  auto const id = BuildCountry("Wonderland", [&](TestMwmBuilder & builder)
  {
    builder.Add(cafe1);
    builder.Add(cafe2);
    builder.Add(cafe3);
  });

  SearchParams params;
  params.m_query = "cafe";
  params.m_inputLocale = "en";
  params.m_viewport = {-0.5, -0.5, 0.5, 0.5};
  params.m_mode = Mode::Viewport;
  params.m_minDistanceOnMapBetweenResults = {kEps * 0.9, kEps * 0.9};
  params.m_suggestsEnabled = false;

  {
    TestSearchRequest request(m_engine, params);
    request.Run();

    TEST(MatchResults(m_dataSource, Rules{ExactMatch(id, cafe1), ExactMatch(id, cafe2), ExactMatch(id, cafe3)},
                      request.Results()),
         ());
  }

  params.m_minDistanceOnMapBetweenResults = {kEps * 1.1, kEps * 1.1};

  {
    TestSearchRequest request(m_engine, params);
    request.Run();

    auto const & results = request.Results();

    TEST(MatchResults(m_dataSource, Rules{ExactMatch(id, cafe1), ExactMatch(id, cafe3)}, results) ||
             MatchResults(m_dataSource, Rules{ExactMatch(id, cafe2)}, results),
         ());
  }
}
}  // namespace interactive_search_test
