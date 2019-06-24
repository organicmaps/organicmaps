#include "testing/testing.hpp"

#include "generator/feature_builder.hpp"
#include "generator/generator_tests/common.hpp"
#include "generator/osm_element.hpp"
#include "generator/regions/collector_region_info.hpp"
#include "generator/regions/regions.hpp"
#include "generator/regions/regions_builder.hpp"
#include "generator/to_string_policy.hpp"

#include "platform/platform.hpp"

#include "coding/transliteration.hpp"

#include "base/file_name_utils.hpp"
#include "base/macros.hpp"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <memory>
#include <string>
#include <utility>
#include <vector>

using namespace generator_tests;
using namespace generator::regions;
using namespace feature;
using namespace base;

namespace
{
using Tags = std::vector<std::pair<std::string, std::string>>;

FeatureBuilder const kEmptyFeature;

OsmElement CreateOsmRelation(uint64_t id, std::string const & adminLevel,
                             std::string const & place = "")
{
  OsmElement el;
  el.m_id = id;
  el.m_type = OsmElement::EntityType::Relation;
  el.AddTag("place", place);
  el.AddTag("admin_level", adminLevel);

  return el;
}

std::string MakeCollectorData()
{
  auto const filename = GetFileName();
  CollectorRegionInfo collector(filename);
  collector.CollectFeature(kEmptyFeature, CreateOsmRelation(1 /* id */, "2" /* adminLevel */));
  collector.CollectFeature(kEmptyFeature, CreateOsmRelation(2 /* id */, "2" /* adminLevel */));
  collector.CollectFeature(kEmptyFeature,
                           CreateOsmRelation(3 /* id */, "4" /* adminLevel */, "state"));
  collector.CollectFeature(kEmptyFeature,
                           CreateOsmRelation(4 /* id */, "4" /* adminLevel */, "state"));
  collector.CollectFeature(kEmptyFeature,
                           CreateOsmRelation(5 /* id */, "4" /* adminLevel */, "state"));
  collector.CollectFeature(kEmptyFeature,
                           CreateOsmRelation(6 /* id */, "6" /* adminLevel */, "district"));
  collector.CollectFeature(kEmptyFeature,
                           CreateOsmRelation(7 /* id */, "6" /* adminLevel */, "district"));
  collector.CollectFeature(kEmptyFeature,
                           CreateOsmRelation(8 /* id */, "4" /* adminLevel */, "state"));
  collector.Save();
  return filename;
}

RegionsBuilder::Regions MakeTestDataSet1(RegionInfo & collector)
{
  RegionsBuilder::Regions regions;
  {
    FeatureBuilder fb;
    fb.AddName("default", "Country_1");
    fb.SetOsmId(MakeOsmRelation(1 /* id */));
    vector<m2::PointD> poly = {{2, 8},  {3, 12}, {8, 15}, {13, 12},
                               {15, 7}, {11, 2}, {4, 4},  {2, 8}};
    fb.AddPolygon(poly);
    fb.SetHoles({{{5, 8}, {7, 10}, {10, 10}, {11, 7}, {10, 4}, {7, 5}, {5, 8}}});
    fb.SetArea();
    regions.emplace_back(Region(fb, collector.Get(MakeOsmRelation(1 /* id */))));
  }

  {
    FeatureBuilder fb;
    fb.AddName("default", "Country_2");
    fb.SetOsmId(MakeOsmRelation(2 /* id */));
    vector<m2::PointD> poly = {{5, 8}, {7, 10}, {10, 10}, {11, 7}, {10, 4}, {7, 5}, {5, 8}};
    fb.AddPolygon(poly);
    fb.SetArea();
    regions.emplace_back(Region(fb, collector.Get(MakeOsmRelation(2 /* id */))));
  }

  {
    FeatureBuilder fb;
    fb.AddName("default", "Country_2");
    fb.SetOsmId(MakeOsmRelation(2 /* id */));
    vector<m2::PointD> poly = {{0, 0}, {0, 2}, {2, 2}, {2, 0}, {0, 0}};
    fb.AddPolygon(poly);
    fb.SetArea();
    regions.emplace_back(Region(fb, collector.Get(MakeOsmRelation(2 /* id */))));
  }

  {
    FeatureBuilder fb;
    fb.AddName("default", "Country_1_Region_3");
    fb.SetOsmId(MakeOsmRelation(3 /* id */));
    vector<m2::PointD> poly = {{4, 4}, {7, 5}, {10, 4}, {12, 9}, {15, 7}, {11, 2}, {4, 4}};
    fb.AddPolygon(poly);
    fb.SetArea();
    regions.emplace_back(Region(fb, collector.Get(MakeOsmRelation(3 /* id */))));
  }

  {
    FeatureBuilder fb;
    fb.AddName("default", "Country_1_Region_4");
    fb.SetOsmId(MakeOsmRelation(4 /* id */));
    vector<m2::PointD> poly = {{7, 10}, {9, 12}, {8, 15},  {13, 12}, {15, 7},
                               {12, 9}, {11, 7}, {10, 10}, {7, 10}};
    fb.AddPolygon(poly);
    fb.SetArea();
    regions.emplace_back(Region(fb, collector.Get(MakeOsmRelation(4 /* id */))));
  }

  {
    FeatureBuilder fb;
    fb.AddName("default", "Country_1_Region_5");
    fb.SetOsmId(MakeOsmRelation(5 /* id */));
    vector<m2::PointD> poly = {{4, 4},  {2, 8}, {3, 12}, {8, 15}, {9, 12},
                               {7, 10}, {5, 8}, {7, 5},  {4, 4}};
    fb.AddPolygon(poly);
    fb.SetArea();
    regions.emplace_back(Region(fb, collector.Get(MakeOsmRelation(5 /* id */))));
  }

  {
    FeatureBuilder fb;
    fb.AddName("default", "Country_1_Region_5_Subregion_6");
    fb.SetOsmId(MakeOsmRelation(6 /* id */));
    vector<m2::PointD> poly = {{4, 4}, {2, 8}, {3, 12}, {4, 10}, {5, 10}, {5, 8}, {7, 5}, {4, 4}};
    fb.AddPolygon(poly);
    fb.SetArea();
    regions.emplace_back(Region(fb, collector.Get(MakeOsmRelation(6 /* id */))));
  }

  {
    FeatureBuilder fb;
    fb.AddName("default", "Country_1_Region_5_Subregion_7");
    fb.SetOsmId(MakeOsmRelation(7 /* id */));
    vector<m2::PointD> poly = {{3, 12}, {8, 15}, {9, 12}, {7, 10},
                               {5, 8},  {5, 10}, {4, 10}, {3, 12}};
    fb.AddPolygon(poly);
    fb.SetArea();
    regions.emplace_back(Region(fb, collector.Get(MakeOsmRelation(7 /* id */))));
  }

  {
    FeatureBuilder fb;
    fb.AddName("default", "Country_2_Region_8");
    fb.SetOsmId(MakeOsmRelation(8 /* id */));
    vector<m2::PointD> poly = {{0, 0}, {0, 1}, {1, 1}, {1, 0}, {0, 0}};
    fb.AddPolygon(poly);
    fb.SetArea();
    regions.emplace_back(Region(fb, collector.Get(MakeOsmRelation(8 /* id */))));
  }

  return regions;
}

class StringJoinPolicy : public ToStringPolicyInterface
{
public:
  std::string ToString(NodePath const & nodePath) const override
  {
    std::stringstream stream;
    for (auto const & n : nodePath)
      stream << n->GetData().GetName();

    return stream.str();
  }
};

bool NameExists(std::vector<std::string> const & coll, std::string const & name)
{
  auto const end = std::end(coll);
  return std::find(std::begin(coll), end, name) != end;
};
}  // namespace

UNIT_TEST(RegionsBuilderTest_GetCountryNames)
{
  auto const filename = MakeCollectorData();
  RegionInfo collector(filename);
  RegionsBuilder builder(MakeTestDataSet1(collector));
  auto const & countryNames = builder.GetCountryNames();
  TEST_EQUAL(countryNames.size(), 2, ());
  TEST(std::count(std::begin(countryNames), std::end(countryNames), "Country_1"), ());
  TEST(std::count(std::begin(countryNames), std::end(countryNames), "Country_2"), ());
}

UNIT_TEST(RegionsBuilderTest_GetCountries)
{
  auto const filename = MakeCollectorData();
  RegionInfo collector(filename);
  RegionsBuilder builder(MakeTestDataSet1(collector));
  auto const & countries = builder.GetCountriesOuters();
  TEST_EQUAL(countries.size(), 3, ());
  TEST_EQUAL(std::count_if(std::begin(countries), std::end(countries),
                           [](const Region & r) { return r.GetName() == "Country_1"; }),
             1, ());
  TEST_EQUAL(std::count_if(std::begin(countries), std::end(countries),
                           [](const Region & r) { return r.GetName() == "Country_2"; }),
             2, ());
}

UNIT_TEST(RegionsBuilderTest_GetCountryTrees)
{
  auto const filename = MakeCollectorData();
  RegionInfo collector(filename);
  std::vector<std::string> bankOfNames;
  RegionsBuilder builder(MakeTestDataSet1(collector));
  builder.ForEachCountry([&](std::string const & name, Node::PtrList const & outers) {
    for (auto const & tree : outers)
    {
      ForEachLevelPath(tree, [&](NodePath const & path) {
        StringJoinPolicy stringifier;
        bankOfNames.push_back(stringifier.ToString(path));
      });
    }
  });

  TEST(NameExists(bankOfNames, "Country_2"), ());
  TEST(NameExists(bankOfNames, "Country_2Country_2_Region_8"), ());

  TEST(NameExists(bankOfNames, "Country_1"), ());
  TEST(NameExists(bankOfNames, "Country_1Country_1_Region_3"), ());
  TEST(NameExists(bankOfNames, "Country_1Country_1_Region_4"), ());
  TEST(NameExists(bankOfNames, "Country_1Country_1_Region_5"), ());
  TEST(NameExists(bankOfNames, "Country_1Country_1_Region_5Country_1_Region_5_Subregion_6"), ());
  TEST(NameExists(bankOfNames, "Country_1Country_1_Region_5Country_1_Region_5_Subregion_7"), ());
}

using Translations = std::vector<std::pair<std::string, std::string>>;
bool TestTransliteration(Translations const & translations,
                         std::string const & expectedTransliteration)
{
  StringUtf8Multilang regionName;
  for (auto const & langAndTranslation : translations)
  {
    regionName.AddString(langAndTranslation.first, langAndTranslation.second);
  }
  RegionWithName region(regionName);
  return region.GetEnglishOrTransliteratedName() == expectedTransliteration;
}

UNIT_TEST(RegionTransliteration)
{
  Transliteration & translit = Transliteration::Instance();
  translit.Init(GetPlatform().ResourcesDir());

  Translations const scotlandTranslations = {
      {"default", "Scotland"},  {"be", "Шатландыя"},  {"cs", "Skotsko"},   {"cy", "Yr Alban"},
      {"da", "Skotland"},       {"de", "Schottland"}, {"eo", "Skotlando"}, {"es", "Escocia"},
      {"eu", "Eskozia"},        {"fi", "Skotlanti"},  {"fr", "Écosse"},    {"ga", "Albain"},
      {"gd", "Alba"},           {"hr", "Škotska"},    {"ia", "Scotia"},    {"io", "Skotia"},
      {"ja", "スコットランド"}, {"ku", "Skotland"},   {"lfn", "Scotland"}, {"nl", "Schotland"},
      {"pl", "Szkocja"},        {"ru", "Шотландия"},  {"sco", "Scotland"}, {"sk", "Škótsko"},
      {"sr", "Шкотска"},        {"sv", "Skottland"},  {"tok", "Sukosi"},   {"tzl", "Escot"},
      {"uk", "Шотландія"},      {"vo", "Skotän"},     {"zh", "苏格兰"}};

  Translations const michiganTranslations = {
      {"default", "Michigan"}, {"ar", "ميشيغان"},    {"az", "Miçiqan"},   {"be", "Мічыган"},
      {"bg", "Мичиган"},       {"br", "Michigan"},   {"en", "Michigan"},  {"eo", "Miĉigano"},
      {"es", "Míchigan"},      {"fa", "میشیگان"},    {"haw", "Mikikana"}, {"he", "מישיגן"},
      {"hy", "Միչիգան"},       {"ja", "ミシガン州"}, {"ko", "미시간"},    {"lt", "Mičiganas"},
      {"lv", "Mičigana"},      {"nv", "Míshigin"},   {"pl", "Michigan"},  {"ru", "Мичиган"},
      {"sr", "Мичиген"},       {"ta", "மிச்சிகன்"},    {"th", "รัฐมิชิแกน"},   {"tl", "Misigan"},
      {"uk", "Мічиган"},       {"yi", "מישיגן"},     {"zh", "密歇根州"}};

  TEST(TestTransliteration(scotlandTranslations, "Shotlandiya"), ());
  TEST(TestTransliteration(michiganTranslations, "Michigan"), ());
}
