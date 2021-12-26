#include "testing/benchmark.hpp"
#include "testing/testing.hpp"

#include "storage/storage_tests/helpers.hpp"

#include "storage/country.hpp"
#include "storage/country_decl.hpp"
#include "storage/country_info_getter.hpp"
#include "storage/storage.hpp"

#include "geometry/mercator.hpp"
#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"

#include "platform/mwm_version.hpp"
#include "platform/platform.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"
#include "base/stats.hpp"
#include "base/timer.hpp"

#include <map>
#include <memory>
#include <random>
#include <string>
#include <utility>
#include <vector>

using namespace storage;
using namespace std;

namespace
{
bool IsEmptyName(map<string, CountryInfo> const & id2info, string const & id)
{
  auto const it = id2info.find(id);
  TEST(it != id2info.end(), ());
  return it->second.m_name.empty();
}

// A helper class to sample random points from mwms uniformly.
class RandomPointGenerator
{
public:
  explicit RandomPointGenerator(mt19937 & randomEngine, vector<m2::RegionD> const & regions)
    : m_randomEngine(randomEngine), m_regions(regions)
  {
    CHECK(!m_regions.empty(), ());
    vector<double> areas(m_regions.size());
    for (size_t i = 0; i < m_regions.size(); ++i)
      areas[i] = m_regions[i].CalculateArea();

    m_distr = discrete_distribution<size_t>(areas.begin(), areas.end());
  }

  m2::PointD operator()()
  {
    auto const i = m_distr(m_randomEngine);
    return m_regions[i].GetRandomPoint(m_randomEngine);
  }

private:
  mt19937 m_randomEngine;

  vector<m2::RegionD> m_regions;
  discrete_distribution<size_t> m_distr;
};

template <typename Cont>
Cont Flatten(vector<Cont> const & cs)
{
  Cont res;
  for (auto const & c : cs)
    res.insert(res.end(), c.begin(), c.end());
  return res;
}
}  // namespace

UNIT_TEST(CountryInfoGetter_GetByPoint_Smoke)
{
  auto const getter = CreateCountryInfoGetter();

  CountryInfo info;

  // Minsk
  getter->GetRegionInfo(mercator::FromLatLon(53.9022651, 27.5618818), info);
  TEST_EQUAL(info.m_name, "Belarus, Minsk Region", ());

  getter->GetRegionInfo(mercator::FromLatLon(-6.4146288, -38.0098101), info);
  TEST_EQUAL(info.m_name, "Brazil, Rio Grande do Norte", ());

  getter->GetRegionInfo(mercator::FromLatLon(34.6509, 135.5018), info);
  TEST_EQUAL(info.m_name, "Japan, Kinki Region_Osaka_Osaka", ());
}

UNIT_TEST(CountryInfoGetter_GetRegionsCountryIdByRect_Smoke)
{
  auto const getter = CreateCountryInfoGetter();

  m2::PointD const p = mercator::FromLatLon(52.537695, 32.203884);

  // Single mwm.
  m2::PointD const halfSize = m2::PointD(0.1, 0.1);
  auto const countries =
      getter->GetRegionsCountryIdByRect(m2::RectD(p - halfSize, p + halfSize), false /* rough */);
  TEST_EQUAL(countries, vector<storage::CountryId>{"Russia_Bryansk Oblast"}, ());

  // Several countries.
  m2::PointD const halfSize2 = m2::PointD(0.5, 0.5);
  auto const countries2 =
      getter->GetRegionsCountryIdByRect(m2::RectD(p - halfSize2, p + halfSize2), false /* rough */);
  auto const expected = vector<storage::CountryId>{
      "Belarus_Homiel Region", "Russia_Bryansk Oblast", "Ukraine_Chernihiv Oblast"};
  TEST_EQUAL(countries2, expected, ());

  // No one found.
  auto const countries3 =
      getter->GetRegionsCountryIdByRect(m2::RectD(-halfSize, halfSize), false /* rough */);
  TEST_EQUAL(countries3, vector<storage::CountryId>{}, ());

  // Inside mwm (rough).
  auto const countries4 =
      getter->GetRegionsCountryIdByRect(m2::RectD(p - halfSize, p + halfSize), true /* rough */);
  TEST_EQUAL(countries, vector<storage::CountryId>{"Russia_Bryansk Oblast"}, ());

  // Several countries (rough).
  auto const countries5 =
      getter->GetRegionsCountryIdByRect(m2::RectD(p - halfSize2, p + halfSize2), true /* rough */);
  auto const expected2 = vector<storage::CountryId>{"Belarus_Homiel Region",
                                                    "Belarus_Maglieu Region",
                                                    "Russia_Bryansk Oblast",
                                                    "Ukraine_Chernihiv Oblast",
                                                    "US_Alaska"};
  TEST_EQUAL(countries5, expected2, ());
}

UNIT_TEST(CountryInfoGetter_ValidName_Smoke)
{
  string buffer;
  ReaderPtr<Reader>(GetPlatform().GetReader(COUNTRIES_FILE)).ReadAsString(buffer);

  map<string, CountryInfo> id2info;
  storage::LoadCountryFile2CountryInfo(buffer, id2info);

  Storage storage;

  TEST(!IsEmptyName(id2info, "Belgium_West Flanders"), ());
  TEST(!IsEmptyName(id2info, "France_Ile-de-France_Paris"), ());
}

UNIT_TEST(CountryInfoGetter_SomeRects)
{
  auto const getter = CreateCountryInfoGetter();

  m2::RectD rects[3];
  getter->CalcUSALimitRect(rects);

  LOG(LINFO, ("USA Continental:", rects[0]));
  LOG(LINFO, ("Alaska:", rects[1]));
  LOG(LINFO, ("Hawaii:", rects[2]));

  LOG(LINFO, ("Canada:", getter->CalcLimitRect("Canada_")));
}

UNIT_TEST(CountryInfoGetter_HitsInRadius)
{
  auto const getter = CreateCountryInfoGetter();
  CountriesVec results;
  getter->GetRegionsCountryId(mercator::FromLatLon(56.1702, 28.1505), results);
  TEST_EQUAL(results.size(), 3, ());
  TEST(find(results.begin(), results.end(), "Belarus_Vitebsk Region") != results.end(), ());
  TEST(find(results.begin(), results.end(), "Latvia") != results.end(), ());
  TEST(find(results.begin(), results.end(), "Russia_Pskov Oblast") != results.end(), ());
}

UNIT_TEST(CountryInfoGetter_HitsOnLongLine)
{
  auto const getter = CreateCountryInfoGetter();
  CountriesVec results;
  getter->GetRegionsCountryId(mercator::FromLatLon(62.2507, -102.0753), results);
  TEST_EQUAL(results.size(), 2, ());
  TEST(find(results.begin(), results.end(), "Canada_Northwest Territories_East") != results.end(),
       ());
  TEST(find(results.begin(), results.end(), "Canada_Nunavut_South") != results.end(), ());
}

UNIT_TEST(CountryInfoGetter_HitsInTheMiddleOfNowhere)
{
  auto const getter = CreateCountryInfoGetter();
  CountriesVec results;
  getter->GetRegionsCountryId(mercator::FromLatLon(62.2900, -103.9423), results);
  TEST_EQUAL(results.size(), 1, ());
  TEST(find(results.begin(), results.end(), "Canada_Northwest Territories_East") != results.end(),
       ());
}

UNIT_TEST(CountryInfoGetter_GetLimitRectForLeafSingleMwm)
{
  auto const getter = CreateCountryInfoGetter();
  Storage storage;

  m2::RectD const boundingBox = getter->GetLimitRectForLeaf("Angola");
  m2::RectD const expectedBoundingBox = {9.205259 /* minX */, -18.34456 /* minY */,
                                         24.08212 /* maxX */, -4.393187 /* maxY */};

  TEST(AlmostEqualRectsAbs(boundingBox, expectedBoundingBox), ());
}

UNIT_TEST(CountryInfoGetter_RegionRects)
{
  auto reader = CountryInfoReader::CreateCountryInfoReader(GetPlatform());
  CHECK(reader != nullptr, ());

  Storage storage;

  auto const & countries = reader->GetCountries();

  for (size_t i = 0; i < countries.size(); ++i)
  {
    vector<m2::RegionD> regions;
    reader->LoadRegionsFromDisk(i, regions);

    m2::RectD rect;
    for (auto const & region : regions)
      region.ForEachPoint([&](m2::PointD const & point) { rect.Add(point); });

    TEST(AlmostEqualRectsAbs(rect, countries[i].m_rect), (rect, countries[i].m_rect));
  }
}

// This is a test for consistency between data/countries.txt and data/packed_polygons.bin.
UNIT_TEST(CountryInfoGetter_Countries_And_Polygons)
{
  auto reader = CountryInfoReader::CreateCountryInfoReader(GetPlatform());
  CHECK(reader != nullptr, ());

  Storage storage;

  double const kRectSize = 10;

  auto const & countries = reader->GetCountries();

  // Set is used here because disputed territories may occur as leaves several times.
  set<CountryId> storageLeaves;
  storage.ForEachCountry([&](Country const & country)
  {
    storageLeaves.insert(country.Name());
  });

  TEST_EQUAL(countries.size(), storageLeaves.size(), ());

  for (size_t defId = 0; defId < countries.size(); ++defId)
  {
    auto const & countryDef = countries[defId];
    TEST_GREATER(storageLeaves.count(countryDef.m_countryId), 0, (countryDef.m_countryId));

    auto const & p = countryDef.m_rect.Center();
    auto const rect = mercator::RectByCenterXYAndSizeInMeters(p.x, p.y, kRectSize, kRectSize);
    auto vec = reader->GetRegionsCountryIdByRect(rect, false /* rough */);
    for (auto const & countryId : vec)
    {
      // This call fails a CHECK if |countryId| is not found.
      storage.GetCountryFile(countryId);
    }
  }
}

BENCHMARK_TEST(CountryInfoGetter_RegionsByRect)
{
  auto reader = CountryInfoReader::CreateCountryInfoReader(GetPlatform());
  CHECK(reader != nullptr, ());

  Storage storage;

  auto const & countryDefs = reader->GetCountries();

  base::Timer timer;

  double const kRectSize = 10;

  mt19937 rng(0);

  vector<vector<m2::RegionD>> allRegions;
  allRegions.reserve(countryDefs.size());
  for (size_t i = 0; i < countryDefs.size(); ++i)
  {
    vector<m2::RegionD> regions;
    reader->LoadRegionsFromDisk(i, regions);
    allRegions.emplace_back(move(regions));
  }

  size_t totalPoints = 0;
  for (auto const & regs : allRegions)
  {
    for (auto const & reg : regs)
      totalPoints += reg.Size();
  }
  LOG(LINFO, ("Total points:", totalPoints));

  {
    size_t const kNumIterations = 1000;

    double const t0 = timer.ElapsedSeconds();

    // Antarctica's rect is too large and skews the random point generation.
    vector<vector<m2::RegionD>> regionsWithoutAnarctica;
    for (size_t i = 0; i < allRegions.size(); ++i)
    {
      if (countryDefs[i].m_countryId == "Antarctica")
        continue;

      regionsWithoutAnarctica.emplace_back(allRegions[i]);
    }

    RandomPointGenerator pointGen(rng, Flatten(regionsWithoutAnarctica));
    vector<m2::PointD> points;
    for (size_t i = 0; i < kNumIterations; i++)
      points.emplace_back(pointGen());

    map<CountryId, int> hits;
    for (auto const & pt : points)
    {
      auto const rect = mercator::RectByCenterXYAndSizeInMeters(pt.x, pt.y, kRectSize, kRectSize);
      auto vec = reader->GetRegionsCountryIdByRect(rect, false /* rough */);
      for (auto const & countryId : vec)
        ++hits[countryId];
    }
    double const t1 = timer.ElapsedSeconds();

    LOG(LINFO, ("hits:", hits.size(), "/", countryDefs.size(), t1 - t0));
  }

  {
    map<CountryId, vector<double>> timesByCountry;
    map<CountryId, double> avgTimeByCountry;
    size_t kNumPointsPerCountry = 1;
    CountryId longest;
    for (size_t countryDefId = 0; countryDefId < countryDefs.size(); ++countryDefId)
    {
      RandomPointGenerator pointGen(rng, allRegions[countryDefId]);
      auto const & countryId = countryDefs[countryDefId].m_countryId;

      vector<double> & times = timesByCountry[countryId];
      times.resize(kNumPointsPerCountry);
      for (size_t i = 0; i < times.size(); ++i)
      {
        auto const pt = pointGen();
        auto const rect = mercator::RectByCenterXYAndSizeInMeters(pt.x, pt.y, kRectSize, kRectSize);
        double const t0 = timer.ElapsedSeconds();
        auto vec = reader->GetRegionsCountryIdByRect(rect, false /* rough */);
        double const t1 = timer.ElapsedSeconds();
        times[i] = t1 - t0;
      }

      avgTimeByCountry[countryId] =
          base::AverageStats<double>(times.begin(), times.end()).GetAverage();

      if (longest.empty() || avgTimeByCountry[longest] < avgTimeByCountry[countryId])
        longest = countryId;
    }

    LOG(LINFO, ("Slowest country for CountryInfoGetter (random point)", longest,
                avgTimeByCountry[longest]));
  }

  {
    map<CountryId, vector<double>> timesByCountry;
    map<CountryId, double> avgTimeByCountry;
    size_t kNumSidesPerCountry = 1;
    CountryId longest;
    for (size_t countryDefId = 0; countryDefId < countryDefs.size(); ++countryDefId)
    {
      auto const & countryId = countryDefs[countryDefId].m_countryId;

      vector<pair<m2::PointD, m2::PointD>> sides;
      for (auto const & region : allRegions[countryDefId])
      {
        auto const & points = region.Data();
        for (size_t i = 0; i < points.size(); ++i)
          sides.emplace_back(points[i], points[(i + 1) % points.size()]);
      }

      CHECK(!sides.empty(), ());
      uniform_int_distribution<size_t> distr(0, sides.size() - 1);
      vector<double> & times = timesByCountry[countryId];
      times.resize(kNumSidesPerCountry);
      for (size_t i = 0; i < times.size(); ++i)
      {
        auto const & side = sides[distr(rng)];
        auto const pt = side.first.Mid(side.second);
        auto const rect = mercator::RectByCenterXYAndSizeInMeters(pt.x, pt.y, kRectSize, kRectSize);
        double const t0 = timer.ElapsedSeconds();
        auto vec = reader->GetRegionsCountryIdByRect(rect, false /* rough */);
        double const t1 = timer.ElapsedSeconds();
        times[i] = t1 - t0;
      }

      avgTimeByCountry[countryId] =
          base::AverageStats<double>(times.begin(), times.end()).GetAverage();

      if (longest.empty() || avgTimeByCountry[longest] < avgTimeByCountry[countryId])
        longest = countryId;
    }
    LOG(LINFO, ("Slowest country for CountryInfoGetter (point on a random side)", longest,
                avgTimeByCountry[longest]));
  }
}
