#include "testing/testing.hpp"

#include "search/search_tests_support/helpers.hpp"
#include "search/search_tests_support/test_results_matching.hpp"
#include "search/search_tests_support/test_search_request.hpp"

#include "generator/feature_builder.hpp"
#include "generator/generator_tests_support/test_feature.hpp"
#include "generator/generator_tests_support/test_mwm_builder.hpp"

#include "search/types_skipper.hpp"

#include "indexer/classificator.hpp"
#include "indexer/feature_meta.hpp"
#include "indexer/feature_visibility.hpp"

#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"

#include "base/string_utils.hpp"

#include <string>

namespace smoke_test
{
using namespace feature;
using namespace generator::tests_support;
using namespace search;
using namespace search::tests_support;
using namespace std;

class IncorrectCountry : public TestCountry
{
public:
  IncorrectCountry(m2::PointD const & center, string const & name, string const & lang)
    : TestCountry(center, name, lang)
  {
  }

  // TestFeature overrides:
  void Serialize(FeatureBuilder & fb) const override
  {
    fb.GetMetadata().Set(Metadata::FMD_TEST_ID, strings::to_string(m_id));
    fb.SetCenter(m_center);

    m_names.ForEach([&](int8_t langCode, string_view name)
    {
      if (!name.empty())
      {
        auto const lang = StringUtf8Multilang::GetLangByCode(langCode);
        CHECK(fb.AddName(lang, name), ("Can't set feature name:", name, "(", lang, ")"));
      }
    });

    auto const & classificator = classif();
    fb.AddType(classificator.GetTypeByPath({"place", "country"}));
  }
};

class SmokeTest : public SearchTest
{
};

class AlcoShop : public TestPOI
{
public:
  AlcoShop(m2::PointD const & center, string const & name, string const & lang)
    : TestPOI(center, name, lang)
  {
    SetTypes({{"shop", "alcohol"}});
  }
};

class SubwayStation : public TestPOI
{
public:
  SubwayStation(m2::PointD const & center, string const & name, string const & lang)
    : TestPOI(center, name, lang)
  {
    SetTypes({{"railway", "station", "subway"}});
  }
};

class SubwayStationMoscow : public TestPOI
{
public:
  SubwayStationMoscow(m2::PointD const & center, string const & name, string const & lang)
    : TestPOI(center, name, lang)
  {
    SetTypes({{"railway", "station", "subway", "moscow"}});
  }
};

UNIT_CLASS_TEST(SmokeTest, Smoke)
{
  char const kCountryName[] = "BuzzTown";

  AlcoShop wineShop(m2::PointD(0, 0), "Wine shop", "en");
  AlcoShop tequilaShop(m2::PointD(1, 0), "Tequila shop", "en");
  AlcoShop brandyShop(m2::PointD(0, 1), "Brandy shop", "en");
  AlcoShop vodkaShop(m2::PointD(1, 1), "Russian vodka shop", "en");

  auto id = BuildMwm(kCountryName, DataHeader::MapType::Country, [&](TestMwmBuilder & builder)
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
    Rules rules = {ExactMatch(id, wineShop)};
    TEST(ResultsMatch("wine ", rules), ());
  }

  Rules const allRule = { ExactMatch(id, wineShop), ExactMatch(id, tequilaShop),
                          ExactMatch(id, brandyShop), ExactMatch(id, vodkaShop) };

  TEST(ResultsMatch("shop ", allRule), ());
  TEST(ResultsMatch("alcohol ", allRule), ());
  TEST(CategoryMatch("алкоголь", allRule, "ru"), ());
}

UNIT_CLASS_TEST(SmokeTest, DeepCategoryTest)
{
  char const kCountryName[] = "Wonderland";

  SubwayStation redStation(m2::PointD(0, 0), "red", "en");
  SubwayStationMoscow blueStation(m2::PointD(1, 1), "blue", "en");

  auto id = BuildMwm(kCountryName, DataHeader::MapType::Country, [&](TestMwmBuilder & builder) {
    builder.Add(redStation);
    builder.Add(blueStation);
  });

  SetViewport(m2::RectD(m2::PointD(0, 0), m2::PointD(1, 1)));
  {
    Rules rules = {ExactMatch(id, redStation), ExactMatch(id, blueStation)};
    TEST(ResultsMatch("Subway ", rules), ());
  }
}

UNIT_CLASS_TEST(SmokeTest, TypesSkipperTest)
{
  TypesSkipper skipper;

  base::StringIL const arr[] = {
    {"barrier", "block"},
    {"barrier", "toll_booth"},
    {"entrance"}
  };

  auto const & cl = classif();
  for (auto const & path : arr)
  {
    TypesHolder types;
    types.Add(cl.GetTypeByPath(path));

    TEST(!skipper.SkipAlways(types), (path));
    skipper.SkipEmptyNameTypes(types);
    TEST_EQUAL(types.Size(), 1, ());
  }
}

UNIT_CLASS_TEST(SmokeTest, CategoriesTest)
{
  auto const & cl = classif();

  // todo(@t.yan): fix some or delete category.
  base::StringIL const invisibleAsPointTags[] = {
      {"waterway", "canal"},
      {"waterway", "river"},
      {"waterway", "riverbank"},
      {"waterway", "stream"},
      {"landuse", "basin"},
      {"place", "county"},
      {"place", "islet"},
      {"highway", "footway"},
      {"highway", "cycleway"},
      {"highway", "living_street"},
      {"highway", "motorway"},
      {"highway", "motorway_link"},
      {"highway", "path"},
      {"highway", "pedestrian"},
      {"highway", "primary"},
      {"highway", "primary_link"},
      {"highway", "raceway"},
      {"highway", "residential"},
      {"highway", "road"},
      {"highway", "secondary"},
      {"highway", "secondary_link"},
      {"highway", "service"},
      {"highway", "steps"},
      {"area:highway", "steps"},
      {"highway", "tertiary"},
      {"highway", "tertiary_link"},
      {"highway", "track"},
      {"highway", "traffic_signals"},
      {"highway", "trunk"},
      {"highway", "trunk_link"},
      {"highway", "unclassified"},
      {"man_made", "tower"},
      {"man_made", "water_tower"},
      {"man_made", "water_well"},
      {"natural", "glacier"},
      {"natural", "water", "pond"},
      {"natural", "tree"}
  };
  set<uint32_t> invisibleTypes;
  for (auto const & tags : invisibleAsPointTags)
    invisibleTypes.insert(cl.GetTypeByPath(tags));

  base::StringIL const notSupportedTags[] = {
      // Not visible because excluded by TypesSkipper.
      {"building", "address"},
      // Not visible for country scale range.
      {"place", "continent"},
      {"place", "region"}
  };
  set<uint32_t> notSupportedTypes;
  for (auto const & tags : notSupportedTags)
    notSupportedTypes.insert(cl.GetTypeByPath(tags));

  auto const & holder = GetDefaultCategories();

  uint32_t const cafeType = cl.GetTypeByPath({"amenity", "cafe"});

  auto testCategory = [&](uint32_t type, CategoriesHolder::Category const &)
  {
    if (invisibleTypes.count(type) > 0)
      return;

    bool categoryIsSearchable = true;
    if (notSupportedTypes.count(type) > 0)
      categoryIsSearchable = false;

    string const countryName = "Wonderland";

    TestPOI poi(m2::PointD(1.0, 1.0), "poi", "en");
    if (IsCategoryNondrawableType(type))
      poi.SetTypes({type, cafeType});
    else
      poi.SetTypes({type});

    auto id = BuildMwm(countryName, DataHeader::MapType::Country,
                       [&](TestMwmBuilder & builder) { builder.AddSafe(poi); });

    Rules rules = {ExactMatch(id, poi)};
    auto const query = holder.GetReadableFeatureType(type, CategoriesHolder::kEnglishCode);
    TEST(CategoryMatch(query, categoryIsSearchable ? rules : Rules{}), (query));

    DeregisterMap(countryName);
  };

  holder.ForEachTypeAndCategory(testCategory);
}

UNIT_CLASS_TEST(SmokeTest, NotPrefixFreeNames)
{
  char const kCountryName[] = "ATown";

  auto id = BuildMwm(kCountryName, DataHeader::MapType::Country, [&](TestMwmBuilder & builder) {
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
    request.Run();
    TEST_EQUAL(1, request.Results().size(), ());
  }
  {
    TestSearchRequest request(m_engine, "aa ", "en", Mode::Viewport, viewport);
    request.Run();
    TEST_EQUAL(2, request.Results().size(), ());
  }
  {
    TestSearchRequest request(m_engine, "aaa ", "en", search::Mode::Viewport, viewport);
    request.Run();
    TEST_EQUAL(3, request.Results().size(), ());
  }
}

UNIT_CLASS_TEST(SmokeTest, NoDefaultNameTest)
{
  char const kCountryName[] = "Wonderland";

  IncorrectCountry wonderland(m2::PointD(0, 0), kCountryName, "en");
  auto worldId = BuildWorld([&](TestMwmBuilder & builder) { builder.Add(wonderland); });

  SetViewport(m2::RectD(m2::PointD(-0.5, -0.5), m2::PointD(0.5, 0.5)));
  TEST(ResultsMatch("Wonderland", {ExactMatch(worldId, wonderland)}), ());
}

UNIT_CLASS_TEST(SmokeTest, PoiWithAddress)
{
  char const kCountryName[] = "Wonderland";
  TestStreet mainStreet({m2::PointD(0.0, 0.0), m2::PointD(1.0, 1.0), m2::PointD(2.0, 2.0)},
                        "Main Street", "en");
  TestCafe cafe(m2::PointD(1.0, 1.0), "Starbucks", "en");
  cafe.SetStreetName(mainStreet.GetName("en"));
  cafe.SetHouseNumber("27");

  auto id = BuildMwm(kCountryName, DataHeader::MapType::Country, [&](TestMwmBuilder & builder) {
    builder.Add(mainStreet);
    builder.Add(cafe);
  });

  SetViewport(m2::RectD(m2::PointD(0.0, 0.0), m2::PointD(2.0, 2.0)));
  {
    Rules rules = {ExactMatch(id, cafe)};
    TEST(ResultsMatch("Starbucks ", rules), ());
    TEST(ResultsMatch("Main street 27 ", rules), ());
    TEST(ResultsMatch("Main street 27 Starbucks ", rules), ());
    TEST(ResultsMatch("Starbucks Main street 27 ", rules), ());
  }
}
} // namespace smoke_test
