#include "testing/testing.hpp"

#include "indexer/classificator.hpp"
#include "indexer/classificator_loader.hpp"
#include "indexer/feature_data.hpp"

namespace feature_types_test
{

feature::TypesHolder MakeTypesHolder(std::initializer_list<base::StringIL> const & arr, bool sortBySpec = true,
                                     feature::GeomType geomType = feature::GeomType::Point)
{
  auto const & cl = classif();
  feature::TypesHolder types(geomType);
  for (auto const & t : arr)
    types.Add(cl.GetTypeByPath(t));

  if (sortBySpec)
    types.SortBySpec();
  else
    types.SortByUseless();

  return types;
}

UNIT_TEST(Feature_UselessTypes)
{
  /// @todo Take out TestWithClassificator into some common test support lib.
  classificator::Load();
  auto const & cl = classif();

  {
    feature::TypesHolder types = MakeTypesHolder(
        {
            {"wheelchair", "yes"},
            {"building", "train_station"},
        },
        false /* sortBySpec */);

    TEST_EQUAL(types.front(), cl.GetTypeByPath({"building", "train_station"}), ());
  }

  {
    feature::TypesHolder types = MakeTypesHolder(
        {
            {"hwtag", "lit"},
            {"hwtag", "oneway"},
        },
        false /* sortBySpec */);

    TEST_EQUAL(types.front(), cl.GetTypeByPath({"hwtag", "oneway"}), ());
  }
}

UNIT_TEST(Feature_TypesPriority)
{
  /// @todo Take out TestWithClassificator into some common test support lib.
  classificator::Load();
  auto const & cl = classif();

  {
    feature::TypesHolder types = MakeTypesHolder({
        {"wheelchair", "yes"},
        {"building", "train_station"},
    });

    TEST_EQUAL(types.front(), cl.GetTypeByPath({"building", "train_station"}), ());
  }

  /// @todo post_office should be bigger than copyshop.
  //  {
  //    feature::TypesHolder types = MakeTypesHolder({
  //      {"shop", "copyshop"},
  //      {"amenity", "post_office"},
  //    });

  //    TEST_EQUAL(types.front(), cl.GetTypeByPath({"amenity", "post_office"}), ());
  //  }

  {
    feature::TypesHolder types = MakeTypesHolder({
        {"internet_access", "wlan"},
        {"amenity", "compressed_air"},
        {"amenity", "fuel"},
    });

    TEST_EQUAL(types.front(), cl.GetTypeByPath({"amenity", "fuel"}), ());
  }

  {
    feature::TypesHolder types = MakeTypesHolder({
        {"leisure", "pitch"},
        {"sport", "soccer"},
    });

    TEST_EQUAL(types.front(), cl.GetTypeByPath({"sport", "soccer"}), ());
  }

  {
    feature::TypesHolder types = MakeTypesHolder({
        {"amenity", "shelter"},
        {"highway", "bus_stop"},
    });

    TEST_EQUAL(types.front(), cl.GetTypeByPath({"highway", "bus_stop"}), ());
  }

  {
    feature::TypesHolder types = MakeTypesHolder({
        {"amenity", "toilets"},
        {"amenity", "community_centre"},
    });

    TEST_EQUAL(types.front(), cl.GetTypeByPath({"amenity", "community_centre"}), ());
  }

  {
    feature::TypesHolder types = MakeTypesHolder({
        {"highway", "elevator"},
        {"emergency", "defibrillator"},
        {"railway", "subway_entrance"},
    });

    TEST_EQUAL(types.front(), cl.GetTypeByPath({"railway", "subway_entrance"}), ());
  }

  {
    feature::TypesHolder types = MakeTypesHolder(
        {
            {"hwtag", "lit"},
            {"hwtag", "oneway"},
            {"highway", "cycleway"},
        },
        true /* sortBySpec */, feature::GeomType::Line);

    TEST_EQUAL(types.front(), cl.GetTypeByPath({"highway", "cycleway"}), ());
  }
}

}  // namespace feature_types_test
