#include "testing/testing.hpp"

#include "search/mode.hpp"
#include "search/search_integration_tests/helpers.hpp"
#include "search/search_tests_support/test_results_matching.hpp"
#include "search/search_tests_support/test_search_request.hpp"
#include "search/viewport_search_callback.hpp"

#include "base/macros.hpp"

using namespace generator::tests_support;
using namespace search::tests_support;

namespace search
{
namespace
{
struct Stats
{
  size_t m_shownResults = 0;
  bool m_mode = false;
};

class TestDelegate : public ViewportSearchCallback::Delegate
{
public:
  explicit TestDelegate(Stats & stats) : m_stats(stats) {}

  ~TestDelegate() override = default;

  // ViewportSearchCallback::Delegate overrides:
  void RunUITask(function<void()> fn) override { fn(); }

  void SetHotelDisplacementMode() override { m_stats.m_mode = true; }

  bool IsViewportSearchActive() const override { return true; }

  void ShowViewportSearchResults(Results const & results) override
  {
    m_stats.m_shownResults = results.GetCount();
  }

  void ClearViewportSearchResults() override { m_stats.m_shownResults = 0; }

 private:
  Stats & m_stats;
};

class InteractiveSearchRequest : public TestDelegate, public TestSearchRequest
{
public:
  InteractiveSearchRequest(TestSearchEngine & engine, string const & query,
                           m2::RectD const & viewport, Stats & stats)
    : TestDelegate(stats)
    , TestSearchRequest(engine, query, "en" /* locale */, Mode::Viewport, viewport)
  {
    SetCustomOnResults(
        ViewportSearchCallback(static_cast<ViewportSearchCallback::Delegate &>(*this),
                               bind(&InteractiveSearchRequest::OnResults, this, _1)));
  }
};

class InteractiveSearchTest : public SearchTest
{
};

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

  auto const id = BuildCountry("Wonderland", [&](TestMwmBuilder & builder) {
    for (auto const & cafe : cafes)
      builder.Add(cafe);
    for (auto const & hotel : hotels)
      builder.Add(hotel);
  });

  {
    Stats stats;
    InteractiveSearchRequest request(
        m_engine, "cafe", m2::RectD(m2::PointD(-1.5, -1.5), m2::PointD(-0.5, -0.5)), stats);
    request.Run();

    TRules const rules = {ExactMatch(id, cafes[0]), ExactMatch(id, cafes[1]),
                          ExactMatch(id, cafes[2]), ExactMatch(id, cafes[3])};

    TEST(!stats.m_mode, ());
    TEST_EQUAL(stats.m_shownResults, 4, ());
    TEST(MatchResults(m_index, rules, request.Results()), ());
  }

  {
    Stats stats;
    InteractiveSearchRequest request(m_engine, "hotel",
                                     m2::RectD(m2::PointD(0.5, 0.5), m2::PointD(1.5, 1.5)), stats);
    request.Run();

    TRules const rules = {ExactMatch(id, hotels[0]), ExactMatch(id, hotels[1]),
                          ExactMatch(id, hotels[2]), ExactMatch(id, hotels[3])};

    TEST(stats.m_mode, ());
    TEST_EQUAL(stats.m_shownResults, 4, ());
    TEST(MatchResults(m_index, rules, request.Results()), ());
  }
}

UNIT_CLASS_TEST(InteractiveSearchTest, NearbyFeaturesInViewport)
{
  double const kEps = 1e-5;
  TestCafe cafe1(m2::PointD(0.0, 0.0));
  TestCafe cafe2(m2::PointD(0.0, kEps));
  TestCafe cafe3(m2::PointD(kEps, kEps));
  TestCafe cafe4(m2::PointD(kEps, 0.0));

  auto const id = BuildCountry("Wonderland", [&](TestMwmBuilder & builder) {
    builder.Add(cafe1);
    builder.Add(cafe2);
    builder.Add(cafe3);
    builder.Add(cafe4);
  });

  SearchParams params;
  params.m_query = "cafe";
  params.m_inputLocale = "en";
  params.m_viewport = m2::RectD(m2::PointD(-0.5, -0.5), m2::PointD(0.5, 0.5));
  params.m_mode = Mode::Viewport;
  params.m_minDistanceOnMapBetweenResults = 0.5;
  params.m_suggestsEnabled = false;

  {
    TestSearchRequest request(m_engine, params);
    request.Run();

    TEST(MatchResults(m_index, TRules{ExactMatch(id, cafe1), ExactMatch(id, cafe2),
                                      ExactMatch(id, cafe3), ExactMatch(id, cafe4)},
                      request.Results()),
         ());
  }

  params.m_minDistanceOnMapBetweenResults = 1.0;

  {
    TestSearchRequest request(m_engine, params);
    request.Run();

    auto const & results = request.Results();

    TEST(MatchResults(m_index, TRules{ExactMatch(id, cafe1), ExactMatch(id, cafe3)}, results) ||
             MatchResults(m_index, TRules{ExactMatch(id, cafe2), ExactMatch(id, cafe4)}, results),
         ());
  }
}
}  // namespace
}  // namespace search
