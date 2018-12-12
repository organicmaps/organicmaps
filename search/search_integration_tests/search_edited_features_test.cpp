#include "testing/testing.hpp"

#include "search/search_tests_support/helpers.hpp"

#include "search/search_tests_support/test_results_matching.hpp"

#include "generator/generator_tests_support/test_feature.hpp"

#include "indexer/editable_map_object.hpp"

#include "geometry/point2d.hpp"

#include "coding/string_utf8_multilang.hpp"

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

UNIT_CLASS_TEST(SearchEditedFeaturesTest, SearchInViewport)
{
  TestCity city(m2::PointD(0, 0), "Canterlot", "default", 100 /* rank */);
  TestPOI bakery0(m2::PointD(0, 0), "Bakery 0", "default");
  TestPOI cornerPost(m2::PointD(100, 100), "Corner Post", "default");
  auto & editor = osm::Editor::Instance();

  BuildWorld([&](TestMwmBuilder & builder) { builder.Add(city); });

  auto const countryId = BuildCountry("Equestria", [&](TestMwmBuilder & builder) {
    builder.Add(bakery0);
    builder.Add(cornerPost);
  });

  auto const tmp1 = TestPOI::AddWithEditor(editor, countryId, "bakery1", {1.0, 1.0});
  TestPOI const & bakery1 = tmp1.first;
  FeatureID const & id1 = tmp1.second;
  auto const tmp2 = TestPOI::AddWithEditor(editor, countryId, "bakery2", {2.0, 2.0});
  TestPOI const & bakery2 = tmp2.first;
  FeatureID const & id2 = tmp2.second;
  auto const tmp3 = TestPOI::AddWithEditor(editor, countryId, "bakery3", {3.0, 3.0});
  TestPOI const & bakery3 = tmp3.first;
  FeatureID const & id3 = tmp3.second;
  UNUSED_VALUE(id2);

  SetViewport(m2::RectD(-1.0, -1.0, 4.0, 4.0));
  {
    TRules const rules = {ExactMatch(countryId, bakery0), ExactMatch(countryId, bakery1),
                          ExactMatch(countryId, bakery2), ExactMatch(countryId, bakery3)};

    TEST(ResultsMatch("bakery", Mode::Viewport, rules), ());
  }

  SetViewport(m2::RectD(-2.0, -2.0, -1.0, -1.0));
  {
    TRules const rules = {};

    TEST(ResultsMatch("bakery", Mode::Viewport, rules), ());
  }

  SetViewport(m2::RectD(-1.0, -1.0, 1.5, 1.5));
  {
    TRules const rules = {ExactMatch(countryId, bakery0), ExactMatch(countryId, bakery1)};

    TEST(ResultsMatch("bakery", Mode::Viewport, rules), ());
  }

  SetViewport(m2::RectD(1.5, 1.5, 4.0, 4.0));
  {
    TRules const rules = {ExactMatch(countryId, bakery2), ExactMatch(countryId, bakery3)};

    TEST(ResultsMatch("bakery", Mode::Viewport, rules), ());
  }

  editor.DeleteFeature(id1);
  editor.DeleteFeature(id3);

  SetViewport(m2::RectD(-1.0, -1.0, 4.0, 4.0));
  {
    TRules const rules = {ExactMatch(countryId, bakery0), ExactMatch(countryId, bakery2)};

    TEST(ResultsMatch("bakery", Mode::Viewport, rules), ());
  }
}
}  // namespace
