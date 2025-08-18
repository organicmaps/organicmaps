#include "testing/testing.hpp"

#include "search/search_tests_support/helpers.hpp"

#include "generator/generator_tests_support/test_feature.hpp"
#include "generator/generator_tests_support/test_mwm_builder.hpp"

#include "search/utils.hpp"

#include "indexer/categories_holder.hpp"
#include "indexer/feature_decl.hpp"

#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"

#include <algorithm>
#include <string>
#include <vector>

namespace search_utils_test
{
using namespace generator::tests_support;
using namespace search;
using namespace std;

class SearchUtilsTest : public search::tests_support::SearchTest
{
public:
  DataSource const & GetDataSource() const { return m_dataSource; }
};

UNIT_CLASS_TEST(SearchUtilsTest, Utils)
{
  string const kCountryName = "FoodLand";
  auto file = platform::LocalCountryFile::MakeForTesting(kCountryName);

  TestPOI cafe(m2::PointD(0.0, 0.0), "cafe", "en");
  cafe.SetTypes({{"amenity", "cafe"}});

  TestPOI restaurant(m2::PointD(0.0, 0.0), "restaurant", "en");
  restaurant.SetTypes({{"amenity", "restaurant"}});

  TestPOI bar(m2::PointD(0.0, 0.0), "bar", "en");
  bar.SetTypes({{"amenity", "bar"}});

  auto id = BuildCountry(kCountryName, [&](TestMwmBuilder & builder)
  {
    builder.Add(cafe);
    builder.Add(restaurant);
    builder.Add(bar);
  });

  auto const & categories = GetDefaultCategories();
  auto const typesPost = GetCategoryTypes("Oficina de correos", "es", categories);
  auto const typesCafe = GetCategoryTypes("Cafe", "en", categories);
  auto const typesRestaurant = GetCategoryTypes("Restaurant", "en", categories);
  auto const typesBar = GetCategoryTypes("Bar", "en", categories);
  auto const typesFood = GetCategoryTypes("Eat", "en", categories);

  // GetCategoryTypes must return sorted vector of types.
  TEST(is_sorted(typesPost.begin(), typesPost.end()), ());
  TEST(is_sorted(typesCafe.begin(), typesCafe.end()), ());
  TEST(is_sorted(typesRestaurant.begin(), typesRestaurant.end()), ());
  TEST(is_sorted(typesBar.begin(), typesBar.end()), ());
  TEST(is_sorted(typesFood.begin(), typesFood.end()), ());

  for (char const * s : {"post_office", "post_box", "parcel_locker"})
    TEST(binary_search(typesPost.begin(), typesPost.end(), classif().GetTypeByPath({"amenity", s})), (s));

  // Now "Cafe" and "Restaurant" are synonyms in categories.
  TEST_EQUAL(typesCafe, typesRestaurant, ());
  TEST_NOT_EQUAL(typesCafe, typesBar, ());

  for (auto const t : typesCafe)
    TEST(binary_search(typesFood.begin(), typesFood.end(), t), ());

  for (auto const t : typesBar)
    TEST(binary_search(typesFood.begin(), typesFood.end(), t), ());

  auto const & dataSource = GetDataSource();
  auto const rect = m2::RectD(m2::PointD(-0.5, -0.5), m2::PointD(0.5, 0.5));

  auto const testTypes = [&](vector<uint32_t> const & types, size_t expectedCount)
  {
    vector<FeatureID> features;
    ForEachOfTypesInRect(dataSource, types, rect, [&features](FeatureID const & f) { features.push_back(f); });
    TEST_EQUAL(features.size(), expectedCount, ());
  };

  // |cafe| and |restaurant|.
  testTypes(typesCafe, 2);

  // |cafe| and |restaurant|.
  testTypes(typesRestaurant, 2);

  // |bar|.
  testTypes(typesBar, 1);

  // All.
  testTypes(typesFood, 3);
}

UNIT_TEST(IsCategorialRequestFuzzy)
{
  auto const isHotelRequest = [](string const & q) { return IsCategorialRequestFuzzy(q, "hotel"); };

  TEST(isHotelRequest("hotel"), ());
  TEST(isHotelRequest("Hotel"), ());
  TEST(isHotelRequest("motel"), ());
  TEST(isHotelRequest("отель"), ());
  TEST(isHotelRequest("гостиница"), ());
  TEST(isHotelRequest("гостиница москва"), ());
  TEST(isHotelRequest("new york hotel"), ());
  TEST(!isHotelRequest("new york where to stay"), ());
  TEST(!isHotelRequest("where to stay"), ());
  TEST(!isHotelRequest("random request"), ());
}
}  // namespace search_utils_test
