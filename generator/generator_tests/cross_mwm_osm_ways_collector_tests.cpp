#include "testing/testing.hpp"

#include "generator/generator_tests/common.hpp"

#include "generator/collector_collection.hpp"
#include "generator/collector_tag.hpp"
#include "generator/cross_mwm_osm_ways_collector.hpp"

#include "platform/platform.hpp"

#include "indexer/classificator_loader.cpp"

#include "geometry/mercator.hpp"

#include "base/assert.hpp"
#include "base/macros.hpp"
#include "base/scope_guard.hpp"

#include <cstdint>
#include <fstream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

using namespace generator;
using namespace generator_tests;

namespace
{
std::string const kTmpDirName = "cross_mwm_ways";

std::vector<std::string> const kHighwayUnclassifiedPath = {"highway", "unclassified"};
std::vector<std::pair<std::string, std::string>> const kHighwayUnclassified = {
    {"highway", "unclassified"}};

std::string const kOsmWayIdOne = std::to_string(base::MakeOsmWay(1).GetEncodedId());
std::string const kOsmWayIdTwo = std::to_string(base::MakeOsmWay(2).GetEncodedId());

class CrossMwmWayCollectorTest
{
public:
  CrossMwmWayCollectorTest()
  {
    classificator::Load();
    m_targetDir = GetPlatform().WritableDir();

    auto const & intermediateDir = base::JoinPath(m_targetDir, kTmpDirName);
    UNUSED_VALUE(Platform::MkDir(intermediateDir));
    m_intermediateDir = intermediateDir;
  }

  ~CrossMwmWayCollectorTest() { Platform::RmDirRecursively(m_intermediateDir); }

  std::shared_ptr<CollectorCollection> InitCollection()
  {
    auto collection = std::make_shared<CollectorCollection>();
    collection->Append(std::make_shared<CrossMwmOsmWaysCollector>(
        m_intermediateDir, m_targetDir, true /* haveBordersForWholeWorld */));
    return collection;
  }

  void Checker()
  {
    std::vector<std::string> answersForRomaniaNorth_West = {
        /* osmId crossMwmSegmentsNumber [crossMwmSegmentsIds forwardIsEnter]+ */
        kOsmWayIdOne + " 1 1 0 ", kOsmWayIdTwo + " 1 0 0 "};

    std::vector<std::string> answersForHungary_Northern_Great_Plain = {
        /* osmId crossMwmSegmentsNumber [crossMwmSegmentsIds forwardIsEnter]+ */
        kOsmWayIdOne + " 1 1 1 ", kOsmWayIdTwo + " 1 0 1 "};

    auto const & pathToRomania =
        base::JoinPath(m_intermediateDir, CROSS_MWM_OSM_WAYS_DIR, "Romania_North_West");
    auto const & pathToHungary =
        base::JoinPath(m_intermediateDir, CROSS_MWM_OSM_WAYS_DIR, "Hungary_Northern Great Plain");

    Check(pathToRomania, std::move(answersForRomaniaNorth_West));
    Check(pathToHungary, std::move(answersForHungary_Northern_Great_Plain));
  }

private:
  static void Check(std::string const & filename, std::vector<std::string> && answers)
  {
    std::ifstream stream;
    stream.exceptions(std::ios::badbit);
    stream.open(filename);
    size_t pos = 0;
    std::string line;
    while (std::getline(stream, line))
    {
      TEST_EQUAL(line, answers[pos], ());
      pos++;
    }
    TEST_EQUAL(pos, answers.size(), ());
  }

  std::string m_intermediateDir;
  std::string m_targetDir;
};

feature::FeatureBuilder CreateFeatureBuilderFromOsmWay(uint64_t osmId,
                                                       std::vector<m2::PointD> && points)
{
  feature::FeatureBuilder fb;
  fb.AddOsmId(base::MakeOsmWay(osmId));
  fb.SetLinear();
  for (auto const & point : points)
    fb.AddPoint(point);

  fb.AddType(classif().GetTypeByPath(kHighwayUnclassifiedPath));
  return fb;
}

void AddOsmWayByPoints(uint64_t osmId, std::vector<m2::PointD> && points,
                       std::shared_ptr<CollectorInterface> const & collection)
{
  auto const & featureBuilder = CreateFeatureBuilderFromOsmWay(osmId, std::move(points));
  collection->CollectFeature(
      featureBuilder, MakeOsmElement(osmId, kHighwayUnclassified, OsmElement::EntityType::Way));
}

void AppendFirstWayFromRomaniaToHungary(std::shared_ptr<CollectorInterface> const & collection)
{
  {
    // In "Romania_North_West", Out of "Hungary_Northern Great Plain"
    auto const & a = MercatorBounds::FromLatLon({47.48897, 22.22737});
    // In "Romania_North_West", Out of "Hungary_Northern Great Plain"
    auto const & b = MercatorBounds::FromLatLon({47.52341, 22.24097});
    // Out of "Romania_North_West", in "Hungary_Northern Great Plain"
    auto const & c = MercatorBounds::FromLatLon({47.63462, 22.04041});
    AddOsmWayByPoints(1 /* osmId */, {a, b, c} /* points */, collection);
  }
}

void AppendSecondWayFromRomaniaToHungary(std::shared_ptr<CollectorInterface> const & collection)
{
  {
    // In "Romania_North_West", Out of "Hungary_Northern Great Plain"
    auto const & a = MercatorBounds::FromLatLon({47.36594, 22.16958});
    // Out of "Romania_North_West", in "Hungary_Northern Great Plain"
    auto const & b = MercatorBounds::FromLatLon({47.49356, 21.77018});
    AddOsmWayByPoints(2 /* osmId */, {a, b} /* points */, collection);
  }
}

UNIT_CLASS_TEST(CrossMwmWayCollectorTest, OneCollectorTest)
{
  auto collection1 = InitCollection();

  AppendFirstWayFromRomaniaToHungary(collection1);
  AppendSecondWayFromRomaniaToHungary(collection1);

  collection1->Save();

  Checker();
}

UNIT_CLASS_TEST(CrossMwmWayCollectorTest, TwoCollectorTest)
{
  auto collection1 = InitCollection();
  AppendFirstWayFromRomaniaToHungary(collection1);

  auto collection2 = collection1->Clone();
  AppendSecondWayFromRomaniaToHungary(collection2);

  collection1->Finish();
  collection2->Finish();
  collection1->Merge(*collection2);
  collection1->Save();

  Checker();
}
}  // namespace
