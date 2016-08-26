#include "testing/testing.hpp"

#include "generator/generator_tests_support/test_feature.hpp"

#include "search/search_integration_tests/helpers.hpp"
#include "search/search_tests_support/test_results_matching.hpp"

#include "indexer/editable_map_object.hpp"

#include "geometry/point2d.hpp"

#include "coding/multilang_utf8_string.hpp"

#include "std/shared_ptr.hpp"

using namespace generator::tests_support;
using namespace search::tests_support;
using namespace search;

namespace
{
class SearchEditedFeaturesTest : public SearchTest
{
};

UNIT_CLASS_TEST(SearchEditedFeaturesTest, Smoke)
{
  TestCity city(m2::PointD(0, 0), "Quahog", "default", 100 /* rank */);
  TestPOI cafe(m2::PointD(0, 0), "Bar", "default");

  BuildWorld([&](TestMwmBuilder & builder) { builder.Add(city); });

  auto const id = BuildCountry("Wonderland", [&](TestMwmBuilder & builder) { builder.Add(cafe); });

  FeatureID cafeId(id, 0 /* index */);

  {
    TRules const rules = {ExactMatch(id, cafe)};

    TEST(ResultsMatch("Bar", rules), ());
    TEST(ResultsMatch("Drunken", TRules{}), ());

    EditFeature(cafeId, [](osm::EditableMapObject & emo) {
      emo.SetName("The Drunken Clam", StringUtf8Multilang::kEnglishCode);
    });

    TEST(ResultsMatch("Bar", rules), ());
    TEST(ResultsMatch("Drunken", rules), ());
  }

  {
    TRules const rules = {ExactMatch(id, cafe)};

    TEST(ResultsMatch("Wifi", TRules{}), ());

    EditFeature(cafeId, [](osm::EditableMapObject & emo) { emo.SetInternet(osm::Internet::Wlan); });

    TEST(ResultsMatch("Wifi", rules), ());
    TEST(ResultsMatch("wifi bar quahog", rules), ());
  }
}
}  // namespace
