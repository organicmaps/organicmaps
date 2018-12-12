#include "testing/testing.hpp"

#include "search/search_tests_support/helpers.hpp"
#include "search/search_tests_support/test_results_matching.hpp"
#include "search/search_tests_support/test_search_request.hpp"

#include "generator/feature_builder.hpp"
#include "generator/generator_tests_support/test_feature.hpp"
#include "generator/generator_tests_support/test_mwm_builder.hpp"

#include "indexer/classificator.hpp"
#include "indexer/feature_meta.hpp"

#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"

#include "base/string_utils.hpp"

#include <string>

using namespace generator::tests_support;
using namespace search::tests_support;
using namespace std;

namespace search
{
namespace
{
class IncorrectCountry : public TestCountry
{
public:
  IncorrectCountry(m2::PointD const & center, string const & name, string const & lang)
    : TestCountry(center, name, lang)
  {
  }

  // TestFeature overrides:
  void Serialize(FeatureBuilder1 & fb) const override
  {
    fb.GetMetadata().Set(feature::Metadata::FMD_TEST_ID, strings::to_string(m_id));
    fb.SetCenter(m_center);

    if (!m_name.empty())
      CHECK(fb.AddName(m_lang, m_name), ("Can't set feature name:", m_name, "(", m_lang, ")"));

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

  ~AlcoShop() override = default;
};

class SubwayStation : public TestPOI
{
public:
  SubwayStation(m2::PointD const & center, string const & name, string const & lang)
    : TestPOI(center, name, lang)
  {
    SetTypes({{"railway", "station", "subway"}});
  }

  ~SubwayStation() override = default;
};

class SubwayStationMoscow : public TestPOI
{
public:
  SubwayStationMoscow(m2::PointD const & center, string const & name, string const & lang)
    : TestPOI(center, name, lang)
  {
    SetTypes({{"railway", "station", "subway", "moscow"}});
  }

  ~SubwayStationMoscow() override = default;
};

UNIT_CLASS_TEST(SmokeTest, Smoke)
{
  char const kCountryName[] = "BuzzTown";

  AlcoShop wineShop(m2::PointD(0, 0), "Wine shop", "en");
  AlcoShop tequilaShop(m2::PointD(1, 0), "Tequila shop", "en");
  AlcoShop brandyShop(m2::PointD(0, 1), "Brandy shop", "en");
  AlcoShop vodkaShop(m2::PointD(1, 1), "Russian vodka shop", "en");

  auto id = BuildMwm(kCountryName, feature::DataHeader::country, [&](TestMwmBuilder & builder) {
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

UNIT_CLASS_TEST(SmokeTest, DeepCategoryTest)
{
  char const kCountryName[] = "Wonderland";

  SubwayStation redStation(m2::PointD(0, 0), "red", "en");
  SubwayStationMoscow blueStation(m2::PointD(1, 1), "blue", "en");

  auto id = BuildMwm(kCountryName, feature::DataHeader::country, [&](TestMwmBuilder & builder) {
    builder.Add(redStation);
    builder.Add(blueStation);
  });

  SetViewport(m2::RectD(m2::PointD(0, 0), m2::PointD(1, 1)));
  {
    TRules rules = {ExactMatch(id, redStation), ExactMatch(id, blueStation)};
    TEST(ResultsMatch("Subway ", rules), ());
  }
}

UNIT_CLASS_TEST(SmokeTest, CategoriesTest)
{
  // todo(@t.yan): fix some or delete category.
  vector<vector<string>> const invisibleAsPointTags = {{"amenity", "driving_school"},
                                                       {"military", "bunker"},
                                                       {"waterway", "canal"},
                                                       {"waterway", "river"},
                                                       {"waterway", "riverbank"},
                                                       {"waterway", "stream"},
                                                       {"landuse", "basin"},
                                                       {"place", "county"},
                                                       {"place", "islet"},
                                                       {"power", "pole"},
                                                       {"highway", "footway"},
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
                                                       {"man_made", "surveillance"},
                                                       {"man_made", "tower"},
                                                       {"man_made", "water_tower"},
                                                       {"man_made", "water_well"},
                                                       {"natural", "glacier"},
                                                       {"natural", "pond"},
                                                       {"natural", "tree"},
                                                       {"natural", "wood"}};
  set<uint32_t> invisibleTypes;
  for (auto const & tags : invisibleAsPointTags)
    invisibleTypes.insert(classif().GetTypeByPath(tags));

  vector<vector<string>> const notSupportedTags = {// Not visible because excluded by TypesSkipper.
                                                   {"barrier", "block"},
                                                   {"barrier", "bollard"},
                                                   {"barrier", "entrance"},
                                                   {"barrier", "gate"},
                                                   {"barrier", "lift_gate"},
                                                   {"barrier", "stile"},
                                                   {"barrier", "toll_booth"},
                                                   {"building", "address"},
                                                   {"entrance"},
                                                   // Not visible for country scale range.
                                                   {"place", "continent"},
                                                   {"place", "region"}};
  set<uint32_t> notSupportedTypes;
  for (auto const & tags : notSupportedTags)
    notSupportedTypes.insert(classif().GetTypeByPath(tags));

  auto const & holder = GetDefaultCategories();
  auto testCategory = [&](uint32_t type, CategoriesHolder::Category const &) {
    if (invisibleTypes.find(type) != invisibleTypes.end())
      return;

    bool categoryIsSearchable = true;
    if (notSupportedTypes.find(type) != notSupportedTypes.end())
      categoryIsSearchable = false;

    string const countryName = "Wonderland";

    TestPOI poi(m2::PointD(1.0, 1.0), "poi", "en");
    poi.SetTypes({strings::Tokenize(classif().GetFullObjectName(type), "|")});

    auto id = BuildMwm(countryName, feature::DataHeader::country,
                       [&](TestMwmBuilder & builder) { builder.Add(poi); });

    SetViewport(m2::RectD(m2::PointD(0.0, 0.0), m2::PointD(2.0, 2.0)));
    {
      TRules rules = {ExactMatch(id, poi)};
      auto const query = holder.GetReadableFeatureType(type, CategoriesHolder::kEnglishCode) + " ";
      TEST(ResultsMatch(query, categoryIsSearchable ? rules : TRules{}), ());
    }
    DeregisterMap(countryName);
  };

  holder.ForEachTypeAndCategory(testCategory);
}

UNIT_CLASS_TEST(SmokeTest, NotPrefixFreeNames)
{
  char const kCountryName[] = "ATown";

  auto id = BuildMwm(kCountryName, feature::DataHeader::country, [&](TestMwmBuilder & builder) {
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
  cafe.SetStreet(mainStreet);
  cafe.SetHouseNumber("27");

  auto id = BuildMwm(kCountryName, feature::DataHeader::country, [&](TestMwmBuilder & builder) {
    builder.Add(mainStreet);
    builder.Add(cafe);
  });

  SetViewport(m2::RectD(m2::PointD(0.0, 0.0), m2::PointD(2.0, 2.0)));
  {
    TRules rules = {ExactMatch(id, cafe)};
    TEST(ResultsMatch("Starbucks ", rules), ());
    TEST(ResultsMatch("Main street 27 ", rules), ());
    TEST(ResultsMatch("Main street 27 Starbucks ", rules), ());
    TEST(ResultsMatch("Starbucks Main street 27 ", rules), ());
  }
}
}  // namespace
}  // namespace search
