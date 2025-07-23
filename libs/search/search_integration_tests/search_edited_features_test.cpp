#include "testing/testing.hpp"

#include "search/search_tests_support/helpers.hpp"
#include "search/search_tests_support/test_results_matching.hpp"

#include "generator/generator_tests_support/test_feature.hpp"

#include "indexer/editable_map_object.hpp"

#include "geometry/point2d.hpp"

#include "coding/string_utf8_multilang.hpp"

namespace search_edited_features_test
{
using namespace generator::tests_support;
using namespace search::tests_support;
using namespace search;
using namespace std;

using SearchEditedFeaturesTest = SearchTest;

UNIT_CLASS_TEST(SearchEditedFeaturesTest, Smoke)
{
  TestCity city(m2::PointD(0, 0), "Quahog", "default", 100 /* rank */);
  TestCafe cafe(m2::PointD(0, 0), "Bar", "default");

  BuildWorld([&](TestMwmBuilder & builder) { builder.Add(city); });

  auto const id = BuildCountry("Wonderland", [&](TestMwmBuilder & builder) { builder.Add(cafe); });

  FeatureID cafeId(id, 0 /* index */);

  {
    Rules const rules = {ExactMatch(id, cafe)};

    TEST(ResultsMatch("Eat ", rules), ());
    TEST(ResultsMatch("cafe Bar", rules), ());
    TEST(ResultsMatch("Drunken", Rules{}), ());

    EditFeature(cafeId, [](osm::EditableMapObject & emo)
    { emo.SetName("The Drunken Clam", StringUtf8Multilang::kEnglishCode); });

    TEST(ResultsMatch("Eat ", rules), ());
    TEST(ResultsMatch("cafe Bar", rules), ());
    TEST(ResultsMatch("Drunken", rules), ());
  }

  {
    Rules const rules = {ExactMatch(id, cafe)};

    TEST(ResultsMatch("Wifi", Rules{}), ());

    EditFeature(cafeId, [](osm::EditableMapObject & emo) { emo.SetInternet(feature::Internet::Wlan); });

    TEST(ResultsMatch("Wifi", rules), ());
    TEST(ResultsMatch("wifi bar quahog", rules), ());
  }
}

UNIT_CLASS_TEST(SearchEditedFeaturesTest, NonamePoi)
{
  TestCafe nonameCafe(m2::PointD(0, 0), "", "default");

  auto const id = BuildCountry("Wonderland", [&](TestMwmBuilder & builder) { builder.Add(nonameCafe); });

  FeatureID cafeId(id, 0 /* index */);

  {
    Rules const rules = {ExactMatch(id, nonameCafe)};

    TEST(ResultsMatch("Eat ", rules), ());

    EditFeature(cafeId, [](osm::EditableMapObject & emo) { emo.SetInternet(feature::Internet::Wlan); });

    TEST(ResultsMatch("Eat ", rules), ());
  }
}

UNIT_CLASS_TEST(SearchEditedFeaturesTest, SearchInViewport)
{
  TestCity city(m2::PointD(0, 0), "Canterlot", "default", 100 /* rank */);
  TestPOI bakery0(m2::PointD(0, 0), "French Bakery 0", "default");
  TestPOI cornerPost(m2::PointD(100, 100), "Corner Post", "default");
  auto & editor = osm::Editor::Instance();

  BuildWorld([&](TestMwmBuilder & builder) { builder.Add(city); });

  auto const countryId = BuildCountry("Equestria", [&](TestMwmBuilder & builder)
  {
    builder.Add(bakery0);
    builder.Add(cornerPost);
  });

  auto const tmp1 = TestPOI::AddWithEditor(editor, countryId, "French Bakery1", {1.0, 1.0});
  TestPOI const & bakery1 = tmp1.first;
  FeatureID const & id1 = tmp1.second;
  auto const tmp2 = TestPOI::AddWithEditor(editor, countryId, "French Bakery2", {2.0, 2.0});
  TestPOI const & bakery2 = tmp2.first;
  FeatureID const & id2 = tmp2.second;
  auto const tmp3 = TestPOI::AddWithEditor(editor, countryId, "French Bakery3", {3.0, 3.0});
  TestPOI const & bakery3 = tmp3.first;
  FeatureID const & id3 = tmp3.second;
  UNUSED_VALUE(id2);

  SetViewport({-1.0, -1.0, 4.0, 4.0});
  {
    Rules const rules = {ExactMatch(countryId, bakery0), ExactMatch(countryId, bakery1), ExactMatch(countryId, bakery2),
                         ExactMatch(countryId, bakery3)};

    TEST(ResultsMatch("french bakery", rules, "en", Mode::Viewport), ());
  }

  SetViewport({-2.0, -2.0, -1.0, -1.0});
  {
    TEST(ResultsMatch("french bakery", {}, "en", Mode::Viewport), ());
  }

  SetViewport({-1.0, -1.0, 1.5, 1.5});
  {
    Rules const rules = {ExactMatch(countryId, bakery0), ExactMatch(countryId, bakery1)};
    TEST(ResultsMatch("french bakery", rules, "en", Mode::Viewport), ());
  }

  SetViewport({1.5, 1.5, 4.0, 4.0});
  {
    Rules const rules = {ExactMatch(countryId, bakery2), ExactMatch(countryId, bakery3)};
    TEST(ResultsMatch("french bakery", rules, "en", Mode::Viewport), ());
  }

  editor.DeleteFeature(id1);
  editor.DeleteFeature(id3);

  SetViewport({-1.0, -1.0, 4.0, 4.0});
  {
    Rules const rules = {ExactMatch(countryId, bakery0), ExactMatch(countryId, bakery2)};
    TEST(ResultsMatch("french bakery", rules, "en", Mode::Viewport), ());
  }
}

UNIT_CLASS_TEST(SearchEditedFeaturesTest, ViewportFilter)
{
  TestCafe restaurant(m2::PointD(0.0, 0.0), "Pushkin", "default");
  // Need this POI for mwm bounding box.
  TestPOI dummy(m2::PointD(1.0, 1.0), "dummy", "default");
  auto & editor = osm::Editor::Instance();

  auto const countryId = BuildCountry("Wounderland", [&](TestMwmBuilder & builder)
  {
    builder.Add(restaurant);
    builder.Add(dummy);
  });

  auto const tmp = TestPOI::AddWithEditor(editor, countryId, "Pushkin cafe", {0.01, 0.01});
  TestPOI const & cafe = tmp.first;

  SearchParams params;
  params.m_query = "pushkin";
  params.m_inputLocale = "en";
  params.m_viewport = {-1.0, -1.0, 1.0, 1.0};
  params.m_mode = Mode::Viewport;

  // Test center for created feature loaded and filter for viewport works.
  {
    params.m_minDistanceOnMapBetweenResults = {0.02, 0.02};

    // Min distance on map between results is 0.02, distance between results is 0.01.
    // The second result must be filtered out.
    Rules const rulesViewport = {ExactMatch(countryId, restaurant)};

    TestSearchRequest request(m_engine, params);
    request.Run();
    TEST(ResultsMatch(request.Results(), rulesViewport), ());
  }

  {
    params.m_minDistanceOnMapBetweenResults = {0.005, 0.005};

    // Min distance on map between results is 0.005, distance between results is 0.01.
    // Filter should keep both results.
    Rules const rulesViewport = {ExactMatch(countryId, restaurant), ExactMatch(countryId, cafe)};

    TestSearchRequest request(m_engine, params);
    request.Run();
    TEST(ResultsMatch(request.Results(), rulesViewport), ());
  }

  SetViewport({-1.0, -1.0, 1.0, 1.0});
  {
    params.m_mode = Mode::Everywhere;
    params.m_minDistanceOnMapBetweenResults = {0.02, 0.02};

    // No viewport filter for everywhere search mode.
    Rules const rulesEverywhere = {ExactMatch(countryId, restaurant), ExactMatch(countryId, cafe)};

    TestSearchRequest request(m_engine, params);
    request.Run();
    TEST(ResultsMatch(request.Results(), rulesEverywhere), ());
  }
}
}  // namespace search_edited_features_test
