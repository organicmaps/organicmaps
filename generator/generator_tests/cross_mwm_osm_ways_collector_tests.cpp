#include "testing/testing.hpp"

#include "generator/generator_tests/common.hpp"

#include "generator/collector_collection.hpp"
#include "generator/collector_tag.hpp"
#include "generator/cross_mwm_osm_ways_collector.hpp"

#include "platform/platform.hpp"

#include "indexer/classificator.hpp"
#include "indexer/classificator_loader.hpp"

#include "geometry/mercator.hpp"

#include "base/assert.hpp"
#include "base/scope_guard.hpp"

#include <cstdint>
#include <fstream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace cross_mwm_osm_ways_collector_tests
{
using namespace generator;
using namespace generator_tests;

std::string const kTmpDirName = "cross_mwm_ways";

std::vector<std::string> const kHighwayUnclassifiedPath = {"highway", "unclassified"};
std::vector<std::pair<std::string, std::string>> const kHighwayUnclassified = {{"highway", "unclassified"}};

std::string const kOsmWayId_1 = std::to_string(base::MakeOsmWay(1).GetEncodedId());
std::string const kOsmWayId_2 = std::to_string(base::MakeOsmWay(2).GetEncodedId());
std::string const kOsmWayId_3 = std::to_string(base::MakeOsmWay(3).GetEncodedId());

class CrossMwmWayCollectorTest
{
public:
  CrossMwmWayCollectorTest()
  {
    classificator::Load();
    auto const targetDir = GetPlatform().WritableDir();

    m_affiliation = std::make_shared<feature::CountriesFilesAffiliation>(targetDir, true /*haveBordersForWholeWorld*/);

    auto const intermediateDir = base::JoinPath(targetDir, kTmpDirName);
    if (!Platform::MkDirChecked(intermediateDir))
      MYTHROW(FileSystemException, ("Can't create intermediateDir", intermediateDir));
    m_intermediateDir = intermediateDir;
  }

  feature::CountriesFilesAffiliation const & GetCountries() { return *m_affiliation; }

  ~CrossMwmWayCollectorTest() { Platform::RmDirRecursively(m_intermediateDir); }

  std::shared_ptr<CollectorCollection> InitCollection()
  {
    auto collection = std::make_shared<CollectorCollection>();
    collection->Append(std::make_shared<CrossMwmOsmWaysCollector>(m_intermediateDir, m_affiliation));
    return collection;
  }

  void Check(std::string const & countryName, std::vector<std::string> const & answers) const
  {
    std::ifstream stream(FormatPath(countryName));
    size_t pos = 0;
    std::string line;
    while (std::getline(stream, line).good())
    {
      TEST_EQUAL(line, answers[pos], (countryName));
      pos++;
    }
    TEST_EQUAL(pos, answers.size(), (countryName));
  }

private:
  std::string FormatPath(std::string const & countryName) const
  {
    return base::JoinPath(m_intermediateDir, CROSS_MWM_OSM_WAYS_DIR, countryName);
  }

  std::shared_ptr<feature::CountriesFilesAffiliation> m_affiliation;
  std::string m_intermediateDir;
};

class Sample1Test : public CrossMwmWayCollectorTest
{
public:
  // Cross mwm collector format is:
  // osmId crossMwmSegmentsNumber [crossMwmSegmentsId forwardIsEnter]+
  void Checker()
  {
    std::vector<std::string> answersFor_RomaniaNorth_West = {kOsmWayId_1 + " 1 1 0 ", kOsmWayId_2 + " 1 0 0 "};

    std::vector<std::string> answersFor_Hungary_Northern_Great_Plain = {kOsmWayId_1 + " 1 1 1 ",
                                                                        kOsmWayId_2 + " 1 0 1 "};

    std::vector<std::string> answersFor_Russia_Moscow = {kOsmWayId_3 + " 1 0 1 "};

    std::vector<std::string> answersFor_Russia_Moscow_Oblast_West = {kOsmWayId_3 + " 1 0 0 "};

    Check("Romania_North_West", answersFor_RomaniaNorth_West);
    Check("Hungary_Northern Great Plain", answersFor_Hungary_Northern_Great_Plain);
    Check("Russia_Moscow", answersFor_Russia_Moscow);
    Check("Russia_Moscow Oblast_West", answersFor_Russia_Moscow_Oblast_West);
  }
};

feature::FeatureBuilder CreateFeatureBuilderFromOsmWay(uint64_t osmId, std::vector<ms::LatLon> const & llPoints)
{
  feature::FeatureBuilder fb;
  fb.AddOsmId(base::MakeOsmWay(osmId));

  std::vector<m2::PointD> points;
  base::Transform(llPoints, std::back_inserter(points), [](ms::LatLon const & ll) { return mercator::FromLatLon(ll); });
  fb.AssignPoints(std::move(points));
  fb.SetLinear();

  fb.AddType(classif().GetTypeByPath(kHighwayUnclassifiedPath));
  return fb;
}

void AddOsmWayByPoints(uint64_t osmId, std::vector<ms::LatLon> const & points,
                       std::shared_ptr<CollectorInterface> const & collection)
{
  collection->CollectFeature(CreateFeatureBuilderFromOsmWay(osmId, points),
                             MakeOsmElement(osmId, kHighwayUnclassified, OsmElement::EntityType::Way));
}

void AppendFirstWayFromRomaniaToHungary(std::shared_ptr<CollectorInterface> const & collection)
{
  {
    // In "Romania_North_West", out of "Hungary_Northern Great Plain"
    ms::LatLon a{47.48897, 22.22737};
    // In "Romania_North_West", out of "Hungary_Northern Great Plain"
    ms::LatLon b{47.52341, 22.24097};
    // Out of "Romania_North_West", in "Hungary_Northern Great Plain"
    ms::LatLon c{47.63462, 22.04041};
    AddOsmWayByPoints(1 /* osmId */, {a, b, c} /* points */, collection);
  }
}

void AppendSecondWayFromRomaniaToHungary(std::shared_ptr<CollectorInterface> const & collection)
{
  {
    // In "Romania_North_West", out of "Hungary_Northern Great Plain"
    ms::LatLon a{47.36594, 22.16958};
    // Out of "Romania_North_West", in "Hungary_Northern Great Plain"
    ms::LatLon b{47.49356, 21.77018};
    AddOsmWayByPoints(2 /* osmId */, {a, b} /* points */, collection);
  }
}

void AppendThirdWayEndsExactlyAtRussiaMoscowBorder(std::shared_ptr<CollectorInterface> const & collection)
{
  {
    // At "Russia_Moscow" border
    ms::LatLon a{55.50334, 36.82098};
    // In "Russia_Moscow", out of "Russia_Moscow Oblast_West"
    ms::LatLon b{55.50222, 36.82246};
    AddOsmWayByPoints(3 /* osmId */, {a, b} /* points */, collection);
  }
}

UNIT_CLASS_TEST(Sample1Test, OneCollectorTest)
{
  auto collection1 = InitCollection();

  AppendFirstWayFromRomaniaToHungary(collection1);
  AppendSecondWayFromRomaniaToHungary(collection1);
  AppendThirdWayEndsExactlyAtRussiaMoscowBorder(collection1);

  collection1->Finalize();

  Checker();
}

UNIT_CLASS_TEST(Sample1Test, TwoCollectorsTest)
{
  auto collection1 = InitCollection();
  AppendFirstWayFromRomaniaToHungary(collection1);
  AppendThirdWayEndsExactlyAtRussiaMoscowBorder(collection1);

  auto collection2 = collection1->Clone();
  AppendSecondWayFromRomaniaToHungary(collection2);

  collection1->Finish();
  collection2->Finish();
  collection1->Merge(*collection2);
  collection1->Finalize();

  Checker();
}

UNIT_CLASS_TEST(CrossMwmWayCollectorTest, Lithuania_Belarus_Kamenny_Log)
{
  auto collection = InitCollection();

  ms::LatLon const connected{54.5442780, 25.6996741};
  auto const countries = GetCountries().GetAffiliations(mercator::FromLatLon(connected));
  TEST_EQUAL(countries.size(), 1, ());

  // https://www.openstreetmap.org/way/614091318 should present in Lithuania and Belarus
  AddOsmWayByPoints(1,
                    {{54.5460103, 25.6945156},
                     {54.5454276, 25.6952895},
                     {54.5453567, 25.6953987},
                     {54.5453056, 25.6955672},
                     {54.5443252, 25.6994996},
                     {54.5443107, 25.6995562},  // 5 segId starts here
                     connected},
                    collection);

  collection->Finalize();

  Check("Lithuania_East", {kOsmWayId_1 + " 1 5 0 "});
  Check("Belarus_Hrodna Region", {kOsmWayId_1 + " 1 5 1 "});
}

UNIT_CLASS_TEST(CrossMwmWayCollectorTest, Belarus_Lithuania_Kamenny_Log)
{
  auto collection = InitCollection();

  ms::LatLon const connected{54.5443346, 25.6997363};
  auto const countries = GetCountries().GetAffiliations(mercator::FromLatLon(connected));
  TEST_EQUAL(countries.size(), 2, ());

  // https://www.openstreetmap.org/way/533044131
  AddOsmWayByPoints(1,
                    {
                        {54.5442277, 25.7001698},
                        {54.5442419, 25.7001125},
                        connected,
                    },
                    collection);

  // https://www.openstreetmap.org/way/489294139
  AddOsmWayByPoints(2,
                    {
                        connected,
                        {54.5443587, 25.6996293},
                        {54.5443765, 25.6995660},
                    },
                    collection);

  collection->Finalize();

  Check("Belarus_Hrodna Region", {kOsmWayId_1 + " 1 1 0 ", kOsmWayId_2 + " 2 0 1 1 0 "});
  Check("Lithuania_East", {kOsmWayId_1 + " 1 1 1 ", kOsmWayId_2 + " 2 0 1 1 1 "});
}

}  // namespace cross_mwm_osm_ways_collector_tests
