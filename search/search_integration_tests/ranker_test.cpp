#include "testing/testing.hpp"

#include "search/search_integration_tests/helpers.hpp"
#include "search/search_tests_support/test_results_matching.hpp"

#include "generator/generator_tests_support/test_feature.hpp"
#include "generator/generator_tests_support/test_mwm_builder.hpp"

using namespace generator::tests_support;
using namespace search::tests_support;
using namespace search;

namespace
{
class RankerTest : public SearchTest
{
};

UNIT_CLASS_TEST(RankerTest, ErrorsInStreets)
{
  TestStreet mazurova(
      vector<m2::PointD>{m2::PointD(-0.001, -0.001), m2::PointD(0, 0), m2::PointD(0.001, 0.001)},
      "Мазурова", "ru");
  TestBuilding mazurova14(m2::PointD(-0.001, -0.001), "", "14", mazurova, "ru");

  TestStreet masherova(
      vector<m2::PointD>{m2::PointD(-0.001, 0.001), m2::PointD(0, 0), m2::PointD(0.001, -0.001)},
      "Машерова", "ru");
  TestBuilding masherova14(m2::PointD(0.001, 0.001), "", "14", masherova, "ru");

  auto id = BuildCountry("Belarus", [&](TestMwmBuilder & builder) {
    builder.Add(mazurova);
    builder.Add(mazurova14);

    builder.Add(masherova);
    builder.Add(masherova14);
  });

  SetViewport(m2::RectD(m2::PointD(0, 0), m2::PointD(0.001, 0.001)));
  {
    auto request = MakeRequest("Мазурова 14");
    auto const & results = request->Results();

    TRules rules = {ExactMatch(id, mazurova14), ExactMatch(id, masherova14)};
    TEST(ResultsMatch(results, rules), ());

    TEST_EQUAL(results.size(), 2, ());
    TEST(ResultsMatch({results[0]}, {rules[0]}), ());
    TEST(ResultsMatch({results[1]}, {rules[1]}), ());
  }
}
}  // namespace
