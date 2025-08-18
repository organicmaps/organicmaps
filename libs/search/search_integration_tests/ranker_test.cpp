#include "testing/testing.hpp"

#include "search/search_tests_support/helpers.hpp"
#include "search/search_tests_support/test_results_matching.hpp"

#include "generator/generator_tests_support/test_feature.hpp"
#include "generator/generator_tests_support/test_mwm_builder.hpp"

#include <vector>

namespace ranker_test
{
using namespace generator::tests_support;
using namespace search::tests_support;
using namespace search;
using namespace std;

class RankerTest : public SearchTest
{};

UNIT_CLASS_TEST(RankerTest, ErrorsInStreets)
{
  TestStreet mazurova(vector<m2::PointD>{m2::PointD(-0.001, -0.001), m2::PointD(0, 0), m2::PointD(0.001, 0.001)},
                      "Мазурова", "ru");
  TestBuilding mazurova14(m2::PointD(-0.001, -0.001), "", "14", mazurova.GetName("ru"), "ru");

  TestStreet masherova(vector<m2::PointD>{m2::PointD(-0.001, 0.001), m2::PointD(0, 0), m2::PointD(0.001, -0.001)},
                       "Машерова", "ru");
  TestBuilding masherova14(m2::PointD(0.001, 0.001), "", "14", masherova.GetName("ru"), "ru");

  auto id = BuildCountry("Belarus", [&](TestMwmBuilder & builder)
  {
    builder.Add(mazurova);
    builder.Add(mazurova14);

    builder.Add(masherova);
    builder.Add(masherova14);
  });

  SetViewport(m2::RectD(m2::PointD(0, 0), m2::PointD(0.001, 0.001)));
  {
    auto request = MakeRequest("Мазурова 14");
    auto const & results = request->Results();

    Rules rules = {ExactMatch(id, mazurova14), ExactMatch(id, masherova14)};
    TEST(ResultsMatch(results, rules), ());

    TEST_EQUAL(results.size(), 2, ());
    TEST(ResultsMatch({results[0]}, {rules[0]}), ());
    TEST(ResultsMatch({results[1]}, {rules[1]}), ());
  }
}

UNIT_CLASS_TEST(RankerTest, UniteSameResults)
{
  TestCity city({1.5, 1.5}, "Buenos Aires", "es", 100);

  m2::PointD org(1.0, 1.0);
  m2::PointD eps(1.0E-5, 1.0E-5);

  TestPOI bus1(org, "Terminal 1", "de");
  bus1.SetTypes({{"highway", "bus_stop"}});
  TestPOI bus2(org + eps, "Terminal 1", "de");
  bus2.SetTypes({{"highway", "bus_stop"}});
  TestPOI bus3(org + eps + eps, "Terminal 1", "de");
  bus3.SetTypes({{"highway", "bus_stop"}});

  TestCafe cafe1({0.5, 0.5}, "И точка", "ru");
  TestCafe cafe2({0.5, 0.5}, "И точка", "ru");

  auto const worldID = BuildWorld([&](TestMwmBuilder & builder) { builder.Add(city); });

  auto const countryID = BuildCountry("Wonderland", [&](TestMwmBuilder & builder)
  {
    builder.Add(city);
    builder.Add(bus1);
    builder.Add(bus2);
    builder.Add(bus3);
    builder.Add(cafe1);
    builder.Add(cafe2);
  });

  SetViewport({0, 0, 2, 2});
  {
    TEST(ResultsMatch("buenos", {ExactMatch(worldID, city)}), ());
    // Expect bus1, because it is strictly in the center of viewport. But it can be any result from bus{1-3}.
    TEST(ResultsMatch("terminal", {ExactMatch(countryID, bus1)}), ());
    TEST_EQUAL(GetResultsNumber("точка", "ru"), 1, ());
  }
}

UNIT_CLASS_TEST(RankerTest, PreferCountry)
{
  std::string const name = "Wonderland";
  TestCountry wonderland(m2::PointD(9.0, 9.0), name, "en");  // ~1400 km from (0, 0)
  TestPOI cafe(m2::PointD(0.0, 0.0), name, "en");
  cafe.SetTypes({{"amenity", "cafe"}});

  auto const worldID = BuildWorld([&](TestMwmBuilder & builder) { builder.Add(wonderland); });

  auto const countryID = BuildCountry(name, [&](TestMwmBuilder & builder) { builder.Add(cafe); });

  SetViewport({0.0, 0.0, 1.0, 1.0});
  {
    // Country which exactly matches the query should be preferred even if cafe is much closer to viewport center.
    Rules const rules = {ExactMatch(worldID, wonderland), ExactMatch(countryID, cafe)};
    TEST(OrderedResultsMatch(name, rules), ());
  }
  {
    /// @todo Fuzzy match should rank nearest cafe first?
    Rules const rules = {ExactMatch(worldID, wonderland), ExactMatch(countryID, cafe)};
    TEST(OrderedResultsMatch("Wanderland", rules), ());
  }
}
}  // namespace ranker_test
