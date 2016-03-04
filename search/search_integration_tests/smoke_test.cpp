#include "testing/testing.hpp"

#include "search/search_integration_tests/helpers.hpp"
#include "search/search_tests_support/test_feature.hpp"
#include "search/search_tests_support/test_mwm_builder.hpp"
#include "search/search_tests_support/test_results_matching.hpp"
#include "search/search_tests_support/test_search_request.hpp"

#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"

#include "std/shared_ptr.hpp"
#include "std/vector.hpp"

using namespace search::tests_support;

using TRules = vector<shared_ptr<MatchingRule>>;

namespace search
{
namespace
{
class SmokeTest : public SearchTest
{
};

UNIT_CLASS_TEST(SmokeTest, Smoke)
{
  char const kCountryName[] = "BuzzTown";

  TestPOI wineShop(m2::PointD(0, 0), "Wine shop", "en");
  TestPOI tequilaShop(m2::PointD(1, 0), "Tequila shop", "en");
  TestPOI brandyShop(m2::PointD(0, 1), "Brandy shop", "en");
  TestPOI vodkaShop(m2::PointD(1, 1), "Russian vodka shop", "en");

  auto id = BuildMwm(kCountryName, feature::DataHeader::country, [&](TestMwmBuilder & builder)
  {
    builder.Add(wineShop);
    builder.Add(tequilaShop);
    builder.Add(brandyShop);
    builder.Add(vodkaShop);
  });

  TEST_EQUAL(4, CountFeatures(m2::RectD(m2::PointD(0, 0), m2::PointD(1, 1))), ());
  TEST_EQUAL(2, CountFeatures(m2::RectD(m2::PointD(-0.5, -0.5), m2::PointD(0.5, 1.5))), ());

  SetViewport(m2::RectD(m2::PointD(0, 0), m2::PointD(100, 100)));
  {
    TRules rules = {ExactMatch(id, wineShop)};
    TEST(ResultsMatch("wine ", rules), ());
  }

  {
    TRules rules = {ExactMatch(id, wineShop), ExactMatch(id, tequilaShop),
                    ExactMatch(id, brandyShop), ExactMatch(id, vodkaShop)};
    TEST(ResultsMatch("shop ", rules), ());
  }
}

UNIT_CLASS_TEST(SmokeTest, NotPrefixFreeNames)
{
  char const kCountryName[] = "ATown";

  auto id = BuildMwm(kCountryName, feature::DataHeader::country, [&](TestMwmBuilder & builder)
  {
    builder.Add(TestPOI(m2::PointD(0, 0), "a", "en"));
    builder.Add(TestPOI(m2::PointD(0, 1), "aa", "en"));
    builder.Add(TestPOI(m2::PointD(1, 1), "aa", "en"));
    builder.Add(TestPOI(m2::PointD(1, 0), "aaa", "en"));
    builder.Add(TestPOI(m2::PointD(2, 0), "aaa", "en"));
    builder.Add(TestPOI(m2::PointD(2, 1), "aaa", "en"));
  });

  TEST_EQUAL(6, CountFeatures(m2::RectD(m2::PointD(0, 0), m2::PointD(2, 2))), ());

  m2::RectD const viewport(m2::PointD(0, 0), m2::PointD(100, 100));
  {
    TestSearchRequest request(m_engine, "a ", "en", Mode::Viewport, viewport);
    request.Wait();
    TEST_EQUAL(1, request.Results().size(), ());
  }
  {
    TestSearchRequest request(m_engine, "aa ", "en", Mode::Viewport, viewport);
    request.Wait();
    TEST_EQUAL(2, request.Results().size(), ());
  }
  {
    TestSearchRequest request(m_engine, "aaa ", "en", search::Mode::Viewport, viewport);
    request.Wait();
    TEST_EQUAL(3, request.Results().size(), ());
  }
}
}  // namespace
}  // namespace search
