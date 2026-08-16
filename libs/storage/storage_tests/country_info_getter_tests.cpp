#include "testing/benchmark.hpp"
#include "testing/testing.hpp"

#include "storage/storage_tests/fake_map_files_downloader.hpp"
#include "storage/storage_tests/helpers.hpp"
#include "storage/storage_tests/task_runner.hpp"

#include "storage/country.hpp"
#include "storage/country_decl.hpp"
#include "storage/country_info_getter.hpp"
#include "storage/storage.hpp"

#include "geometry/mercator.hpp"
#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"

#include "coding/reader.hpp"
#include "coding/writer.hpp"

#include "platform/platform.hpp"
#include "platform/platform_tests_support/scoped_dir.hpp"
#include "platform/platform_tests_support/scoped_file.hpp"
#include "platform/platform_tests_support/writable_dir_changer.hpp"
#include "platform/settings.hpp"

#include "base/assert.hpp"
#include "base/file_name_utils.hpp"
#include "base/logging.hpp"
#include "base/stats.hpp"
#include "base/string_utils.hpp"
#include "base/timer.hpp"

#include "defines.hpp"

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <list>
#include <map>
#include <memory>
#include <random>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace country_info_getter_tests
{
namespace tests_support = platform::tests_support;
using namespace storage;
using namespace std;

static double constexpr kRectCompareEpsilon = 1e-2;

// The terrain tests constructing a Storage must be isolated twice over:
// - WritableDirChanger keeps the terrain files and (in a filtered run) the settings in
//   a temp dir - the artifact sweep deletes the downloader leftovers and must not
//   touch a real data/terrain;
// - the settings singleton binds its file at the FIRST use in the process, so in a full
//   suite run an earlier unguarded test pins it to the real settings; the keys the
//   terrain paths write (or act upon: a real DownloadQueue value would make
//   RestoreDownloadQueue enqueue real downloads) are saved/cleared/restored explicitly.
char const kTerrainTestDir[] = "terrain_tests";

// A fake current-version local map must MATCH its countries.json size (an
// inconsistent pair logs an error, which aborts the tests) - a sparse resize costs
// no disk and no time.
void ResizeToRemote(tests_support::ScopedFile const & file, Storage const & storage, CountryId const & id)
{
  std::filesystem::resize_file(file.GetFullPath(), storage.GetCountryFile(id).GetRemoteSize());
}

class ScopedTerrainSettings
{
public:
  ScopedTerrainSettings()
  {
    for (auto const key : kKeys)
    {
      std::string value;
      if (settings::Get(key, value))
        m_saved.emplace(key, std::move(value));
      settings::Delete(key);
    }
  }
  ~ScopedTerrainSettings()
  {
    for (auto const key : kKeys)
      if (auto const it = m_saved.find(key); it != m_saved.end())
        settings::Set(key, it->second);
      else
        settings::Delete(key);
  }

private:
  // Mirror kDownloadQueueKey/kTerrainWithMapsKey in storage.cpp.
  static constexpr std::string_view kKeys[] = {"DownloadQueue", "TerrainWithMaps"};
  std::map<std::string_view, std::string> m_saved;
};

// Feeds the storage the on-disk truth the provider scan delivers on a real start (the
// async RegisterAllMaps flow, see Storage::OnTerrainScanned): every *.twm under the
// terrain tree, (name, version folder) exactly like TerrainProvider::Rescan reports.
void ScanTerrain(Storage & storage)
{
  std::vector<terrain::TwmFile> scanned;
  for (auto const & [dir, version] : terrain::ListVersionDirs(base::JoinPath(GetPlatform().WritableDir(), TERRAIN_DIR)))
  {
    Platform::FilesList files;
    Platform::GetFilesByExt(dir, TERRAIN_FILE_EXT, files);
    for (auto const & file : files)
      scanned.push_back({base::FilenameWithoutExt(file), version});
  }
  storage.OnTerrainScanned(scanned);
}

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
    : m_randomEngine(randomEngine)
    , m_regions(regions)
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

UNIT_TEST(CountryDef_IsIntersectOrInside)
{
  using O = CountryDef::Overlap;
  // testRect is "wrapped" (minX in (-180, 180)) but may cross the antimeridian (maxX > 180).
  // countryRect is non-wrapped but may itself cross the antimeridian (maxX > 180 or minX < -180).
  auto Check = [](m2::RectD const & test, m2::RectD const & country)
  { return CountryDef::IsIntersectOrInside(test, country); };

  // 1. Both canonical, no antimeridian wrap.
  TEST_EQUAL(Check({0, 0, 10, 10}, {20, 20, 30, 30}), O::NONE, ());     // disjoint
  TEST_EQUAL(Check({0, 0, 10, 10}, {5, 5, 15, 15}), O::INTERSECT, ());  // partial overlap
  TEST_EQUAL(Check({0, 0, 10, 10}, {2, 2, 8, 8}), O::INSIDE, ());       // country inside test
  TEST_EQUAL(Check({2, 2, 8, 8}, {0, 0, 10, 10}), O::INTERSECT, ());    // test inside country (not INSIDE)

  // 2. Country stored unwrapped past the east antimeridian (maxX > 180), canonical test on the far
  //    (negative) side; the -360 country shift brings them together.
  TEST_EQUAL(Check({-175, -10, -170, -5}, {184, -12, 193, -4}), O::INTERSECT, ());
  TEST_EQUAL(Check({-100, -10, -90, -5}, {184, -12, 193, -4}), O::NONE, ());    // no overlap even shifted
  TEST_EQUAL(Check({-180, -20, -170, 0}, {184, -12, 186, -4}), O::INSIDE, ());  // small country inside test

  // 3. Country stored unwrapped past the west antimeridian (minX < -180); the +360 country shift.
  TEST_EQUAL(Check({170, -10, 176, -5}, {-193, -12, -184, -4}), O::INTERSECT, ());

  // 4. Test rect itself crosses the antimeridian (maxX > 180) and the country is canonical near it;
  //    the +360 country shift brings it into the test's extended frame.
  TEST_EQUAL(Check({170, -10, 190, -5}, {-173, -12, -171, -4}), O::INTERSECT, ());  // Samoa-like
  TEST_EQUAL(Check({170, -20, 190, 0}, {-176, -12, -174, -4}), O::INSIDE, ());      // country inside wrapped test
  TEST_EQUAL(Check({170, -10, 190, -5}, {-100, -12, -90, -4}), O::NONE, ());        // far away

  // Real-data regressions.
  // "Marae Moana" feature piece (canonical) vs the "Tokelau" border rect (unwrapped past 180).
  CountryDef const tokelau("Tokelau", {184.6, -10.4, 193.2, -5.4});
  TEST(tokelau.IsRectOverlap({-168.5, -26.2, -154.8, -5.8}), ());
  TEST(!tokelau.IsRectOverlap({-100, -10, -90, -5}), ());

  // Canonical "Samoa" vs a wrapped viewport rect that crosses the antimeridian.
  CountryDef const samoa("Samoa", {-173, -12, -171, -4});
  TEST(samoa.IsRectOverlap({170, -10, 190, -5}), ());
}

UNIT_TEST(CountryInfoGetter_GetRegionsCountryIdByRect_Antimeridian)
{
  auto const getter = CreateCountryInfoGetter();

  // US_Alaska's bound rect crosses the antimeridian (stored as [-190.22 .. -129.97]); its Aleutian
  // polygons live in the canonical [170, 180) and (-180, -170] ranges. A wrapped viewport rect that
  // crosses 180 must still match them via the precise polygon test (rough == false ->
  // IsIntersectedByRegion -> CountryDef::ForEachRectSideWrapped).
  auto hasAlaska = [&](m2::RectD const & r)
  {
    auto const cs = getter->GetRegionsCountryIdByRect(r, false /* rough */);
    return find(cs.begin(), cs.end(), "US_Alaska") != cs.end();
  };
  auto rectAround = [](double lat, double lon, double half)
  { return m2::RectD(mercator::FromLatLon(lat - half, lon - half), mercator::FromLatLon(lat + half, lon + half)); };

  // Westernmost Aleutians (canonical positive lon) - inside Alaska.
  TEST(hasAlaska(rectAround(52.7600266, 173.8999264, 0.02)), ());
  // Bering Sea north of the Aleutians - NOT Alaska.
  TEST(!hasAlaska(rectAround(59.1444374, 174.793276, 0.02)), ());

  // Bering Strait point on the Alaska (negative lon) side, reached only by the wrapped-right part of
  // a thin rect spanning lon [170, 192]: the [180, 192] half wraps to [-180, -168] and must match.
  TEST(hasAlaska({mercator::FromLatLon(65.74, 170), mercator::FromLatLon(65.78, 192)}), ());
}

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
  auto const countries = getter->GetRegionsCountryIdByRect(m2::RectD(p - halfSize, p + halfSize), false /* rough */);
  TEST_EQUAL(countries, vector<storage::CountryId>{"Russia_Bryansk Oblast"}, ());

  // Several countries.
  m2::PointD const halfSize2 = m2::PointD(0.5, 0.5);
  auto const countries2 = getter->GetRegionsCountryIdByRect(m2::RectD(p - halfSize2, p + halfSize2), false /* rough */);
  auto const expected =
      vector<storage::CountryId>{"Belarus_Homiel Region", "Russia_Bryansk Oblast", "Ukraine_Chernihiv Oblast"};
  TEST_EQUAL(countries2, expected, ());

  // No one found.
  auto const countries3 = getter->GetRegionsCountryIdByRect(m2::RectD(-halfSize, halfSize), false /* rough */);
  TEST_EQUAL(countries3, vector<storage::CountryId>{}, ());

  // Inside mwm (rough).
  auto const countries4 = getter->GetRegionsCountryIdByRect(m2::RectD(p - halfSize, p + halfSize), true /* rough */);
  TEST_EQUAL(countries, vector<storage::CountryId>{"Russia_Bryansk Oblast"}, ());

  // Several countries (rough).
  auto const countries5 = getter->GetRegionsCountryIdByRect(m2::RectD(p - halfSize2, p + halfSize2), true /* rough */);
  // US_Alaska was in the old expected list because its rect was [-180, 180] (intersected everything).
  // With extended rects, Alaska's rect is compact and no longer intersects Belarus area.
  auto const expected2 = vector<storage::CountryId>{"Belarus_Homiel Region", "Belarus_Maglieu Region",
                                                    "Russia_Bryansk Oblast", "Ukraine_Chernihiv Oblast"};
  TEST_EQUAL(countries5, expected2, ());
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
  TEST(find(results.begin(), results.end(), "Canada_Northwest Territories_East") != results.end(), ());
  TEST(find(results.begin(), results.end(), "Canada_Nunavut_South") != results.end(), ());
}

UNIT_TEST(CountryInfoGetter_HitsInTheMiddleOfNowhere)
{
  auto const getter = CreateCountryInfoGetter();
  CountriesVec results;
  getter->GetRegionsCountryId(mercator::FromLatLon(62.2900, -103.9423), results);
  TEST_EQUAL(results.size(), 1, ());
  TEST(find(results.begin(), results.end(), "Canada_Northwest Territories_East") != results.end(), ());
}

UNIT_TEST(CountryInfoGetter_BorderRelations)
{
  auto const getter = CreateCountryInfoGetter();

  ms::LatLon labels[] = {
      {42.4318876, -8.6431592}, {42.6075172, -8.4714942}, {42.3436415, -7.8674242},
      {42.1968459, -7.6114105}, {43.0118437, -7.5565749}, {43.0462247, -7.4739921},
      {43.3709703, -8.3959425}, {43.0565609, -8.5296941}, {27.0006968, 49.6532161},
  };

  for (auto const & ll : labels)
  {
    auto const region = getter->GetRegionCountryId(mercator::FromLatLon(ll));
    LOG(LINFO, (region));
    TEST(!region.empty(), (ll));
  }
}

UNIT_TEST(CountryInfoGetter_GetLimitRectForLeafSingleMwm)
{
  auto const getter = CreateCountryInfoGetter();
  Storage storage;

  m2::RectD const boundingBox = getter->GetLimitRectForLeaf("Angola");
  m2::RectD const expectedBoundingBox = {9.205259 /* minX */, -18.34456 /* minY */, 24.08212 /* maxX */,
                                         -4.393187 /* maxY */};

  TEST(AlmostEqualAbs(boundingBox, expectedBoundingBox, kRectCompareEpsilon), ());
}

// This is a test for consistency between data/countries.json and data/packed_polygons.bin.
UNIT_TEST(CountryInfoGetter_Countries_And_Polygons)
{
  auto reader = CountryInfoReader::CreateCountryInfoReader(GetPlatform());
  CHECK(reader != nullptr, ());

  Storage storage;

  double const kRectSize = 10;

  auto const & countries = reader->GetCountries();

  // Set is used here because disputed territories may occur as leaves several times.
  set<CountryId> storageLeaves;
  storage.ForEachCountry([&](platform::CountryFile const & country) { storageLeaves.insert(country.GetName()); });

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
    allRegions.emplace_back(reader->LoadRegionsFromDisk(i));

  size_t totalPoints = 0;
  for (auto const & regs : allRegions)
    for (auto const & reg : regs)
      totalPoints += reg.Size();
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

      avgTimeByCountry[countryId] = base::AverageStats<double>(times).GetAverage();

      if (longest.empty() || avgTimeByCountry[longest] < avgTimeByCountry[countryId])
        longest = countryId;
    }

    LOG(LINFO, ("Slowest country for CountryInfoGetter (random point)", longest, avgTimeByCountry[longest]));
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

      avgTimeByCountry[countryId] = base::AverageStats<double>(times).GetAverage();

      if (longest.empty() || avgTimeByCountry[longest] < avgTimeByCountry[countryId])
        longest = countryId;
    }
    LOG(LINFO, ("Slowest country for CountryInfoGetter (point on a random side)", longest, avgTimeByCountry[longest]));
  }
}

UNIT_TEST(CountryInfoGetter_ExtendedRect_ReadWriteRoundtrip)
{
  // Verify that extended rects (past +-180) survive Read/Write roundtrip.
  // These values are derived from .poly file analysis of antimeridian-crossing regions.
  struct TestCase
  {
    char const * name;
    m2::RectD rect;
  };
  TestCase const kTestCases[] = {
      {"New Zealand South_Canterbury", {166.02, -45.50, 185.62, -28.18}},
      {"Russia_Chukotka Autonomous Okrug", {157.73, 61.28, 191.03, 74.80}},
      {"Fiji", {175.99, -21.23, 182.10, -11.94}},
      {"US_Alaska", {-190.2209, 57.90752, -129.9742, 108.23441}},
      {"Kiribati", {-191.1612, -13.06341, -149.18980, 7.04328}},
      {"US_United States Minor Outlying Islands", {166.37, -0.73, 285.37, 28.66}},
  };

  for (auto const & tc : kTestCases)
  {
    CountryDef original(tc.name, tc.rect);

    // Write to buffer.
    std::vector<uint8_t> buf;
    MemWriter<decltype(buf)> writer(buf);
    Write(writer, original);

    // Read back.
    MemReader reader(buf.data(), buf.size());
    ReaderSource src(reader);
    CountryDef restored;
    Read(src, restored);

    TEST_EQUAL(restored.m_countryId, original.m_countryId, ());
    TEST(AlmostEqualAbs(restored.m_rect, original.m_rect, kRectCompareEpsilon),
         (tc.name, restored.m_rect, original.m_rect));
  }
}

UNIT_TEST(CountryInfoGetter_ExtendedRect_BelongsToRegion)
{
  // Verify that BelongsToRegion works with extended rects via the +360 shift check.
  CountryInfoGetterForTesting getter;
  getter.AddCountry(CountryDef("TestCountry", m2::RectD(166.0, -45.0, 186.0, -28.0)));

  // Point inside eastern portion [166, 180].
  TEST_EQUAL(getter.GetRegionCountryId({170.0, -40.0}), "TestCountry", ());

  // Point inside western portion [-180, -174] — wrapped by WrapX, matches via +360 shift.
  TEST_EQUAL(getter.GetRegionCountryId({-175.0, -40.0}), "TestCountry", ());

  // Point outside the region.
  TEST_EQUAL(getter.GetRegionCountryId({100.0, -40.0}), kInvalidCountryId, ());

  // Point outside (northern hemisphere).
  TEST_EQUAL(getter.GetRegionCountryId({170.0, 40.0}), kInvalidCountryId, ());
}

UNIT_TEST(CountryInfoGetter_ExtendedRect_WestExtended)
{
  // Verify that west-extended rects (minX < -180) work correctly with symmetric helpers.
  CountryInfoGetterForTesting getter;
  getter.AddCountry(CountryDef("WestCountry", m2::RectD(-194.0, -20.0, -160.0, -10.0)));

  // Point inside western portion (canonical coords [-180, -160]).
  TEST_EQUAL(getter.GetRegionCountryId({-170.0, -15.0}), "WestCountry", ());

  // Point inside eastern portion [166, 180] — wrapped by WrapX to [-175, ...], matches via -360 shift.
  TEST_EQUAL(getter.GetRegionCountryId({175.0, -15.0}), "WestCountry", ());

  // Point outside.
  TEST_EQUAL(getter.GetRegionCountryId({0.0, -15.0}), kInvalidCountryId, ());

  // Rect overlap with west-extended country.
  auto const countries = getter.GetRegionsCountryIdByRect(m2::RectD(-175.0, -20.0, -165.0, -10.0), true);
  TEST_EQUAL(countries.size(), 1, ());
  TEST_EQUAL(countries[0], "WestCountry", ());
}
UNIT_TEST(CountryInfoGetter_CanonicalCountry_ExtendedQueryRect)
{
  // Canonical country (rect within [-180, 180]) must be found when the query rect
  // is in extended coordinates (from a wrapped viewport past the antimeridian).
  CountryInfoGetterForTesting getter;
  getter.AddCountry(CountryDef("CanonicalCountry", m2::RectD(-5.0, 42.0, 9.0, 52.0)));

  // Canonical query rect — should match.
  auto countries = getter.GetRegionsCountryIdByRect(m2::RectD(-10.0, 40.0, 15.0, 55.0), true);
  TEST_EQUAL(countries.size(), 1, ());
  TEST_EQUAL(countries[0], "CanonicalCountry", ());

  // Extended query rect shifted +360 — viewport one world-width east.
  countries = getter.GetRegionsCountryIdByRect(m2::RectD(350.0, 40.0, 375.0, 55.0), true);
  TEST_EQUAL(countries.size(), 1, ());
  TEST_EQUAL(countries[0], "CanonicalCountry", ());

  // Extended query rect shifted -360 — viewport one world-width west.
  countries = getter.GetRegionsCountryIdByRect(m2::RectD(-370.0, 40.0, -345.0, 55.0), true);
  TEST_EQUAL(countries.size(), 1, ());
  TEST_EQUAL(countries[0], "CanonicalCountry", ());

  // Non-overlapping extended rect — should not match.
  countries = getter.GetRegionsCountryIdByRect(m2::RectD(200.0, 40.0, 220.0, 55.0), true);
  TEST(countries.empty(), ());
}

UNIT_TEST(CountryInfoGetter_CanonicalCountry_ExtendedPoint)
{
  // Canonical country must be found when the query point has extended coordinates.
  CountryInfoGetterForTesting getter;
  getter.AddCountry(CountryDef("CanonicalCountry", m2::RectD(130.0, 30.0, 145.0, 45.0)));

  // Canonical point — should match.
  TEST_EQUAL(getter.GetRegionCountryId({135.0, 35.0}), "CanonicalCountry", ());

  // Extended point shifted +360 — should match via WrapX in GetRegionCountryId
  // and via -360 shift in IsPointInsideRect.
  TEST_EQUAL(getter.GetRegionCountryId({495.0, 35.0}), "CanonicalCountry", ());

  // Extended point shifted -360.
  TEST_EQUAL(getter.GetRegionCountryId({-225.0, 35.0}), "CanonicalCountry", ());
}

UNIT_TEST(CountryInfoGetter_ExtendedRect_ExactLookup)
{
  // Verify that GetRegionsCountryIdByRect with rough=false works with extended rects.
  // CountryInfoGetterForTesting uses rect-only checks, so this validates the wrapping
  // logic in GetRegionsCountryIdByRect itself.
  CountryInfoGetterForTesting getter;
  getter.AddCountry(CountryDef("France", m2::RectD(-5.0, 42.0, 9.0, 52.0)));

  // Canonical rect — should match with exact check.
  auto countries = getter.GetRegionsCountryIdByRect(m2::RectD(-10.0, 40.0, 15.0, 55.0), false);
  TEST_EQUAL(countries.size(), 1, ());
  TEST_EQUAL(countries[0], "France", ());

  // Extended rect shifted +360 — should match after wrapping.
  countries = getter.GetRegionsCountryIdByRect(m2::RectD(350.0, 40.0, 375.0, 55.0), false);
  TEST_EQUAL(countries.size(), 1, ());
  TEST_EQUAL(countries[0], "France", ());

  // Extended rect shifted -360.
  countries = getter.GetRegionsCountryIdByRect(m2::RectD(-370.0, 40.0, -345.0, 55.0), false);
  TEST_EQUAL(countries.size(), 1, ());
  TEST_EQUAL(countries[0], "France", ());

  // Non-overlapping extended rect — should not match.
  countries = getter.GetRegionsCountryIdByRect(m2::RectD(200.0, 40.0, 220.0, 55.0), false);
  TEST(countries.empty(), ());
}

UNIT_TEST(Storage_TerrainOutOfDateStatus)
{
  WritableDirChanger const writableDirChanger(kTerrainTestDir, WritableDirChanger::SettingsDirPolicy::UseWritableDir);
  ScopedTerrainSettings const guardSettings;
  Storage storage;
  ScanTerrain(storage);

  // The injected has-older callback flips a region with no current blocks on disk
  // between NotDownloaded and OnDiskOutOfDate (cf. TerrainProvider::HasOlderTerrain).
  bool hasOlder = false;
  storage.SetTerrainCallbacks({}, [&](m2::RectD const &, int64_t /* version */) { return hasOlder; });

  // Madagascar has no local terrain blocks in the test environment.
  TEST_EQUAL(storage.GetTerrainAttrs("Madagascar").m_status, Storage::TerrainStatus::NotDownloaded, ());
  hasOlder = true;
  TEST_EQUAL(storage.GetTerrainAttrs("Madagascar").m_status, Storage::TerrainStatus::OnDiskOutOfDate, ());
}

UNIT_TEST(Storage_TerrainAttrsAllNodes)
{
  WritableDirChanger const writableDirChanger(kTerrainTestDir, WritableDirChanger::SettingsDirPolicy::UseWritableDir);
  ScopedTerrainSettings const guardSettings;
  Storage storage;
  ScanTerrain(storage);
  storage.SetTerrainCallbacks({}, {});

  // The downloader UI queries the terrain attrs of every row: every node, the groups
  // via the deduplicated union of their leafs, must resolve without a hiccup. Every
  // downloadable leaf must have coverage in data/twm_grid.json (the bundle guard: the
  // grid and data/borders drift independently), World/WorldCoasts must have none.
  size_t checked = 0;
  storage.ForEachInSubtree(storage.GetRootId(), [&](CountryId const & id, bool groupNode)
  {
    auto const attrs = storage.GetTerrainAttrs(id);
    if (!groupNode && id != WORLD_FILE_NAME && id != WORLD_COASTS_FILE_NAME)
      TEST_NOT_EQUAL(attrs.m_status, Storage::TerrainStatus::NotAvailable, (id));
    ++checked;
  });
  TEST_GREATER(checked, 1000, (checked));

  TEST_EQUAL(storage.GetTerrainAttrs(WORLD_FILE_NAME).m_status, Storage::TerrainStatus::NotAvailable, ());
}

UNIT_TEST(Storage_TerrainDelete)
{
  WritableDirChanger const writableDirChanger(kTerrainTestDir, WritableDirChanger::SettingsDirPolicy::UseWritableDir);
  ScopedTerrainSettings const guardSettings;
  Storage storage;

  std::vector<m2::RectD> deleted;
  storage.SetTerrainCallbacks({}, {}, [&](std::vector<m2::RectD> const & rects) { deleted = rects; });

  storage.DeleteTerrain("Madagascar");
  // The exact rects of the covering blocks from the twm_grid.json "mwms" list.
  TEST(!deleted.empty(), ());
  for (auto const & rect : deleted)
    TEST(rect.IsValid(), ());

  // A group resolves to the deduplicated union of its leafs: the group rect count must
  // be no less than any single leaf's and strictly less than the leafs' sum (the Norway
  // leafs share the blocks along their common borders).
  size_t leafsSum = 0;
  size_t leafMax = 0;
  storage.ForEachInSubtree("Norway", [&](CountryId const & id, bool groupNode)
  {
    if (groupNode)
      return;
    deleted.clear();
    storage.DeleteTerrain(id);
    leafsSum += deleted.size();
    leafMax = std::max(leafMax, deleted.size());
  });
  deleted.clear();
  storage.DeleteTerrain("Norway");
  TEST_GREATER_OR_EQUAL(deleted.size(), leafMax, ());
  TEST_LESS(deleted.size(), leafsSum, ());
}

UNIT_TEST(Storage_TerrainParseTwmGridJson)
{
  int64_t version = 42;
  std::vector<Storage::TerrainBlock> blocks;
  std::map<CountryId, std::vector<uint32_t>> coverage;

  // The per-block "v" falls back to the index version; the coverage resolves to the
  // deduplicated sorted indices.
  Storage::ParseTwmGridJson(R"({"v": 260729, "blocks": [
      {"id": "N45E008", "sx": 3, "sy": 2, "s": 10, "h": "aGFzaDE"},
      {"id": "N45E011", "sx": 2, "sy": 2, "s": 20, "h": "aGFzaDI", "v": 260315}],
      "mwms": {"A": ["N45E011", "N45E008", "N45E011"], "B": ["N45E008"]}})",
                            version, blocks, coverage);
  TEST_EQUAL(version, 260729, ());
  TEST_EQUAL(blocks.size(), 2, ());
  TEST_EQUAL(blocks[0].m_version, 260729, ());
  TEST_EQUAL(blocks[1].m_version, 260315, ());
  TEST_EQUAL(coverage.size(), 2, ());
  TEST_EQUAL(coverage["A"], (std::vector<uint32_t>{0, 1}), ());
  TEST_EQUAL(coverage["B"], (std::vector<uint32_t>{0}), ());

  // Any inconsistency throws and leaves the out params untouched: a truncated grid
  // must not half-configure the storage.
  for (char const * bad : {
           R"({"blocks": [{"id": "N45E008", "sx": 1, "sy": 1, "s": 1, "h": "aA"}], "mwms": {"A": ["N45E008"]}})",
           R"({"v": 1, "blocks": [{"id": "N45E008", "sx": 1, "sy": 1, "s": 1, "h": "aA"}]})",
           R"({"v": 1, "blocks": [{"id": "N45E008", "sx": 1, "sy": 1, "s": 1, "h": "aA"}], "mwms": {"A": []}})",
           R"({"v": 1, "blocks": [{"id": "N45E008", "sx": 1, "sy": 1, "s": 1, "h": "aA"}], "mwms": {"A": ["X"]}})",
           R"({"v": 1, "blocks": [{"id": "N45E008", "sx": 1, "sy": 1, "s": 1, "h": "aA"},
                                  {"id": "N45E008", "sx": 1, "sy": 1, "s": 1, "h": "aA"}],
               "mwms": {"A": ["N45E008"]}})",
           R"({"v": 1, "blocks": [{"id": "N45E008", "sx": 1, "sy": 1, "s": 1, "h": "aA", "v": -1}],
               "mwms": {"A": ["N45E008"]}})",
       })
  {
    bool thrown = false;
    try
    {
      Storage::ParseTwmGridJson(bad, version, blocks, coverage);
    }
    catch (RootException const &)
    {
      thrown = true;
    }
    TEST(thrown, (bad));
    TEST_EQUAL(version, 260729, (bad));
    TEST_EQUAL(blocks.size(), 2, (bad));
    TEST_EQUAL(coverage.size(), 2, (bad));
  }
}

UNIT_TEST(Storage_TerrainStatusPrecedence)
{
  WritableDirChanger const writableDirChanger(kTerrainTestDir, WritableDirChanger::SettingsDirPolicy::UseWritableDir);
  ScopedTerrainSettings const guardSettings;
  // A region with some blocks on disk: Partly; a stale block whose area still renders
  // from an older file ranks OnDiskOutOfDate above Partly (a partial-world grid update
  // reads "update available", not "partly downloaded").
  int64_t version = 0;
  std::vector<Storage::TerrainBlock> blocks;
  std::map<CountryId, std::vector<uint32_t>> coverage;
  {
    std::string content;
    GetPlatform().GetReader(TERRAIN_GRID_FILE)->ReadAsString(content);
    Storage::ParseTwmGridJson(content, version, blocks, coverage);
  }

  // Any multi-block region works the same; pick a deterministic one.
  CountryId region;
  for (auto const & [id, indices] : coverage)
    if (indices.size() >= 2)
    {
      region = id;
      break;
    }
  TEST(!region.empty(), ());

  auto const & block = blocks[coverage[region].front()];
  tests_support::ScopedDir terrainDir(TERRAIN_DIR);
  tests_support::ScopedDir versionDir(terrainDir, strings::to_string(block.m_version));
  tests_support::ScopedFile blockFile(base::JoinPath(versionDir.GetRelativePath(), block.m_name + TERRAIN_FILE_EXT),
                                      "twm");

  Storage storage;
  ScanTerrain(storage);
  bool hasOlder = false;
  storage.SetTerrainCallbacks({}, [&](m2::RectD const &, int64_t /* version */) { return hasOlder; });

  TEST_EQUAL(storage.GetTerrainAttrs(region).m_status, Storage::TerrainStatus::Partly, (region));
  hasOlder = true;
  TEST_EQUAL(storage.GetTerrainAttrs(region).m_status, Storage::TerrainStatus::OnDiskOutOfDate, (region));
}

UNIT_TEST(Storage_TerrainRefcountDelete)
{
  WritableDirChanger const writableDirChanger(kTerrainTestDir, WritableDirChanger::SettingsDirPolicy::UseWritableDir);
  ScopedTerrainSettings const guardSettings;

  int64_t version = 0;
  std::vector<Storage::TerrainBlock> blocks;
  std::map<CountryId, std::vector<uint32_t>> coverage;
  {
    std::string content;
    GetPlatform().GetReader(TERRAIN_GRID_FILE)->ReadAsString(content);
    Storage::ParseTwmGridJson(content, version, blocks, coverage);
  }

  // Two regions sharing a block, the first one with a globally exclusive block too.
  std::map<uint32_t, std::vector<CountryId>> owners;
  for (auto const & [region, indices] : coverage)
    for (auto const index : indices)
      owners[index].push_back(region);

  CountryId regionA, regionB;
  uint32_t sharedBlock = 0, exclusiveBlock = 0;
  for (auto const & [index, regions] : owners)
  {
    if (regions.size() != 2)
      continue;
    for (auto const & region : regions)
    {
      auto const & indices = coverage[region];
      auto const exclusive =
          std::find_if(indices.begin(), indices.end(), [&owners](uint32_t i) { return owners[i].size() == 1; });
      if (exclusive == indices.end())
        continue;
      regionA = region;
      regionB = regions[0] == region ? regions[1] : regions[0];
      sharedBlock = index;
      exclusiveBlock = *exclusive;
      break;
    }
    if (!regionA.empty())
      break;
  }
  TEST(!regionA.empty() && !regionB.empty(), ());

  // All the blocks of both regions on disk (empty stub files are enough for the stats).
  std::set<uint32_t> created(coverage[regionA].begin(), coverage[regionA].end());
  created.insert(coverage[regionB].begin(), coverage[regionB].end());
  tests_support::ScopedDir terrainDir(TERRAIN_DIR);
  std::list<tests_support::ScopedDir> versionDirs;
  std::list<tests_support::ScopedFile> files;
  std::set<std::string> versionNames;
  for (auto const index : created)
    if (versionNames.insert(strings::to_string(blocks[index].m_version)).second)
      versionDirs.emplace_back(terrainDir, strings::to_string(blocks[index].m_version));
  for (auto const index : created)
    files.emplace_back(base::JoinPath(TERRAIN_DIR, strings::to_string(blocks[index].m_version),
                                      blocks[index].m_name + TERRAIN_FILE_EXT),
                       "twm");

  // The terrain follows the maps: only regionB is downloaded, so its coverage is the
  // protection set of the ref-counted delete.
  Storage storage;
  tests_support::ScopedDir mapsDir(strings::to_string(storage.GetCurrentDataVersion()));
  tests_support::ScopedFile mapB(mapsDir, platform::CountryFile(regionB), MapFileType::Map);
  ResizeToRemote(mapB, storage, regionB);
  storage.RegisterAllLocalMaps();
  ScanTerrain(storage);
  std::vector<m2::RectD> deleted;
  storage.SetTerrainCallbacks({}, {}, [&](std::vector<m2::RectD> const & rects) { deleted = rects; });
  TEST_EQUAL(storage.GetTerrainAttrs(regionA).m_status, Storage::TerrainStatus::OnDisk, (regionA));

  // Deleting regionA keeps the block shared with the downloaded regionB and drops the
  // globally exclusive one.
  std::set<uint32_t> wantedByB(coverage[regionB].begin(), coverage[regionB].end());
  size_t expectedA = 0;
  for (auto const index : coverage[regionA])
    if (wantedByB.count(index) == 0)
      ++expectedA;
  TEST_GREATER(expectedA, 0, ());
  deleted.clear();
  storage.DeleteTerrain(regionA);
  TEST_EQUAL(deleted.size(), expectedA, (regionA));
  auto const contains = [&deleted](m2::RectD const & rect)
  {
    return std::any_of(deleted.begin(), deleted.end(),
                       [&rect](m2::RectD const & r) { return AlmostEqualAbs(r, rect, kRectCompareEpsilon); });
  };
  TEST(contains(blocks[exclusiveBlock].m_rect), ());
  TEST(!contains(blocks[sharedBlock].m_rect), ());

  // A deleted region does not protect itself: every block of regionB goes, the shared
  // one loses its last downloaded owner.
  deleted.clear();
  storage.DeleteTerrain(regionB);
  TEST_EQUAL(deleted.size(), coverage[regionB].size(), (regionB));
  TEST(contains(blocks[sharedBlock].m_rect), ());
}

UNIT_TEST(Storage_TerrainWithMapsSettingAndDeleteAll)
{
  WritableDirChanger const writableDirChanger(kTerrainTestDir, WritableDirChanger::SettingsDirPolicy::UseWritableDir);
  ScopedTerrainSettings const guardSettings;

  int64_t version = 0;
  std::vector<Storage::TerrainBlock> blocks;
  std::map<CountryId, std::vector<uint32_t>> coverage;
  {
    std::string content;
    GetPlatform().GetReader(TERRAIN_GRID_FILE)->ReadAsString(content);
    Storage::ParseTwmGridJson(content, version, blocks, coverage);
  }
  // A block that is some region's whole coverage: one file on disk, simple sizes.
  auto const it =
      std::find_if(coverage.begin(), coverage.end(), [](auto const & entry) { return entry.second.size() == 1; });
  TEST(it != coverage.end(), ());
  auto const & block = blocks[it->second.front()];

  tests_support::ScopedDir terrainDir(TERRAIN_DIR);
  tests_support::ScopedDir versionDir(terrainDir, strings::to_string(block.m_version));
  tests_support::ScopedFile blockFile(
      base::JoinPath(TERRAIN_DIR, strings::to_string(block.m_version), block.m_name + TERRAIN_FILE_EXT), "twm-bytes");
  tests_support::ScopedFile orphan(
      base::JoinPath(TERRAIN_DIR, strings::to_string(block.m_version), "N00E000" TERRAIN_FILE_EXT ".ready"), "partial");

  Storage storage;
  ScanTerrain(storage);
  TEST(storage.IsTerrainWithMaps(), ());  // The default is ON.
  TEST_EQUAL(storage.GetTerrainOnDiskSize(), strlen("twm-bytes") + strlen("partial"), ());

  std::vector<m2::RectD> deleted;
  storage.SetTerrainCallbacks({}, {}, [&](std::vector<m2::RectD> const & rects) { deleted = rects; });
  storage.DeleteAllTerrain();
  TEST_EQUAL(deleted.size(), 1, ());  // The world rect for the registered blocks.
  TEST(!blockFile.Exists() && !orphan.Exists(), ());
  // The whole terrain tree is gone; disarm the scoped cleanups.
  blockFile.Reset();
  orphan.Reset();
  versionDir.Reset();
  terrainDir.Reset();
  TEST_EQUAL(storage.GetTerrainOnDiskSize(), 0, ());

  // The setting turns off durably.
  storage.SetTerrainWithMaps(false);
  TEST(!storage.IsTerrainWithMaps(), ());
  Storage const restarted;
  TEST(!restarted.IsTerrainWithMaps(), ());
}

UNIT_TEST(Storage_TerrainOrphanReadySweep)
{
  WritableDirChanger const writableDirChanger(kTerrainTestDir, WritableDirChanger::SettingsDirPolicy::UseWritableDir);
  ScopedTerrainSettings const guardSettings;

  // An interrupted download leaves the downloader artifacts behind; with no downloaded
  // map wanting the block, the startup restore sweeps them (see RestoreTerrain: the
  // resume runs after both the scan and the queue restore have landed).
  tests_support::ScopedDir terrainDir(TERRAIN_DIR);
  tests_support::ScopedDir versionDir(terrainDir, "260729");
  tests_support::ScopedFile ready(base::JoinPath(TERRAIN_DIR, "260729", "N45E008.twm.ready"), "partial");
  tests_support::ScopedFile resume(base::JoinPath(TERRAIN_DIR, "260729", "N45E008.twm.ready.resume"), "state");

  Storage storage;
  ScanTerrain(storage);
  storage.RestoreDownloadQueue();

  TEST(!ready.Exists(), ());
  TEST(!resume.Exists(), ());
  // The sweep already deleted them; a double delete in the dtor would log an error.
  ready.Reset();
  resume.Reset();
}

UNIT_TEST(Storage_TerrainArtifactResumeAndCancel)
{
  WritableDirChanger const writableDirChanger(kTerrainTestDir, WritableDirChanger::SettingsDirPolicy::UseWritableDir);
  ScopedTerrainSettings const guardSettings;

  // The artifacts of an interrupted block under a downloaded map are the resume
  // record: the restore re-enqueues the region and keeps the artifacts (exercised in
  // the production iOS/macOS order - the queue restore lands first, the scan second);
  // a cancel deletes them, so the stopped download does not come back at a next start.
  int64_t version = 0;
  std::vector<Storage::TerrainBlock> blocks;
  std::map<CountryId, std::vector<uint32_t>> coverage;
  {
    std::string content;
    GetPlatform().GetReader(TERRAIN_GRID_FILE)->ReadAsString(content);
    Storage::ParseTwmGridJson(content, version, blocks, coverage);
  }
  auto const it =
      std::find_if(coverage.begin(), coverage.end(), [](auto const & entry) { return entry.second.size() == 1; });
  TEST(it != coverage.end(), ());
  CountryId const region = it->first;
  auto const & block = blocks[it->second.front()];

  tests_support::ScopedDir terrainDir(TERRAIN_DIR);
  tests_support::ScopedDir versionDir(terrainDir, strings::to_string(block.m_version));
  std::string const readyRel = base::JoinPath(TERRAIN_DIR, strings::to_string(block.m_version),
                                              block.m_name + TERRAIN_FILE_EXT READY_FILE_EXTENSION);
  tests_support::ScopedFile ready(readyRel, "partial");
  tests_support::ScopedFile resume(readyRel + RESUME_FILE_EXTENSION, "state");

  Storage storage;
  tests_support::ScopedDir mapsDir(strings::to_string(storage.GetCurrentDataVersion()));
  tests_support::ScopedFile map(mapsDir, platform::CountryFile(region), MapFileType::Map);
  ResizeToRemote(map, storage, region);
  storage.RegisterAllLocalMaps();
  TaskRunner runner;  // Never run: the resumed block must stay queued.
  storage.SetDownloaderForTesting(std::make_unique<FakeMapFilesDownloader>(runner));

  storage.RestoreDownloadQueue();
  ScanTerrain(storage);

  TEST_EQUAL(storage.GetTerrainAttrs(region).m_status, Storage::TerrainStatus::Downloading, (region));
  TEST(ready.Exists(), ());
  TEST(resume.Exists(), ());

  storage.CancelDownloadNode(region);
  TEST_EQUAL(storage.GetTerrainAttrs(region).m_status, Storage::TerrainStatus::NotDownloaded, (region));
  TEST(!ready.Exists(), ());
  TEST(!resume.Exists(), ());
  ready.Reset();
  resume.Reset();
}

UNIT_TEST(Storage_TerrainUpdateInfo)
{
  WritableDirChanger const writableDirChanger(kTerrainTestDir, WritableDirChanger::SettingsDirPolicy::UseWritableDir);
  ScopedTerrainSettings const guardSettings;

  int64_t version = 0;
  std::vector<Storage::TerrainBlock> blocks;
  std::map<CountryId, std::vector<uint32_t>> coverage;
  {
    std::string content;
    GetPlatform().GetReader(TERRAIN_GRID_FILE)->ReadAsString(content);
    Storage::ParseTwmGridJson(content, version, blocks, coverage);
  }

  // A block that is the WHOLE coverage of several regions: one file on disk completes
  // them all, so losing it leaves every one of them missing the very same block - which is
  // what the dedup has to collapse into a single size.
  std::map<uint32_t, std::vector<CountryId>> soleOwners;
  for (auto const & [region, indices] : coverage)
    if (indices.size() == 1)
      soleOwners[indices.front()].push_back(region);
  auto const it =
      std::find_if(soleOwners.begin(), soleOwners.end(), [](auto const & entry) { return entry.second.size() > 1; });
  TEST(it != soleOwners.end(), ());
  auto const & block = blocks[it->first];

  // The terrain follows the maps: two downloaded co-owner regions, no terrain files -
  // the shared block is missing for both. The fake maps carry the CURRENT data
  // version, an older one would read as out-of-date maps and pollute the counters.
  Storage storage;
  tests_support::ScopedDir mapsDir(strings::to_string(storage.GetCurrentDataVersion()));
  tests_support::ScopedFile map1(mapsDir, platform::CountryFile(it->second[0]), MapFileType::Map);
  tests_support::ScopedFile map2(mapsDir, platform::CountryFile(it->second[1]), MapFileType::Map);
  ResizeToRemote(map1, storage, it->second[0]);
  ResizeToRemote(map2, storage, it->second[1]);
  ScanTerrain(storage);
  storage.RegisterAllLocalMaps();
  Storage::UpdateInfo updateInfo;
  TEST(storage.GetUpdateInfo(storage.GetRootId(), updateInfo), ());
  // The fake maps are current, but both regions have their terrain to fetch: the
  // update badges must not read "nothing to update" on a terrain-only refresh.
  TEST_EQUAL(updateInfo.m_numberOfMwmFilesToUpdate, 2, ());
  // Counted once for all the co-owners, and nothing older on disk to be replaced.
  TEST_EQUAL(updateInfo.m_totalDownloadSizeInBytes, block.m_size, (it->second));
  TEST_EQUAL(updateInfo.m_maxFileSizeInBytes, block.m_size, ());
  TEST_EQUAL(updateInfo.m_sizeDifference, static_cast<int64_t>(block.m_size), ());

  // Scoped to the subtree: the co-owner leaf carries the block, a not-downloaded
  // region must not carry another region's terrain.
  Storage::UpdateInfo leafInfo;
  TEST(storage.GetUpdateInfo(it->second.front(), leafInfo), ());
  TEST_EQUAL(leafInfo.m_totalDownloadSizeInBytes, block.m_size, (it->second.front()));
  TEST_EQUAL(leafInfo.m_numberOfMwmFilesToUpdate, 1, (it->second.front()));
  Storage::UpdateInfo otherInfo;
  TEST(storage.GetUpdateInfo("Madagascar", otherInfo), ());
  TEST_EQUAL(otherInfo.m_totalDownloadSizeInBytes, 0, ());
  TEST_EQUAL(otherInfo.m_numberOfMwmFilesToUpdate, 0, ());

  // An older file still rendering the area is replaced, not added: the download size
  // stays, the disk does not grow.
  storage.SetTerrainCallbacks({}, [](m2::RectD const &, int64_t /* version */) { return true; });
  Storage::UpdateInfo replacingInfo;
  TEST(storage.GetUpdateInfo(storage.GetRootId(), replacingInfo), ());
  TEST_EQUAL(replacingInfo.m_totalDownloadSizeInBytes, block.m_size, ());
  TEST_EQUAL(replacingInfo.m_sizeDifference, 0, ());

  // "Download terrain with maps" off: the terrain drops out of the update entirely.
  storage.SetTerrainWithMaps(false);
  Storage::UpdateInfo offInfo;
  TEST(storage.GetUpdateInfo(storage.GetRootId(), offInfo), ());
  TEST_EQUAL(offInfo.m_totalDownloadSizeInBytes, 0, ());
  TEST_EQUAL(offInfo.m_numberOfMwmFilesToUpdate, 0, ());
}

UNIT_TEST(Storage_TerrainNodeAttrsFusion)
{
  WritableDirChanger const writableDirChanger(kTerrainTestDir, WritableDirChanger::SettingsDirPolicy::UseWritableDir);
  ScopedTerrainSettings const guardSettings;

  int64_t version = 0;
  std::vector<Storage::TerrainBlock> blocks;
  std::map<CountryId, std::vector<uint32_t>> coverage;
  {
    std::string content;
    GetPlatform().GetReader(TERRAIN_GRID_FILE)->ReadAsString(content);
    Storage::ParseTwmGridJson(content, version, blocks, coverage);
  }
  auto const it =
      std::find_if(coverage.begin(), coverage.end(), [](auto const & entry) { return entry.second.size() == 1; });
  TEST(it != coverage.end(), ());
  CountryId const & region = it->first;
  auto const & block = blocks[it->second.front()];

  // A downloaded map with its one covering block missing.
  Storage storage;
  tests_support::ScopedDir mapsDir(strings::to_string(storage.GetCurrentDataVersion()));
  tests_support::ScopedFile map(mapsDir, platform::CountryFile(region), MapFileType::Map);
  ResizeToRemote(map, storage, region);
  storage.RegisterAllLocalMaps();
  ScanTerrain(storage);

  // The region size is the map plus its terrain coverage; the missing terrain of a
  // map-complete region reads "update available".
  uint64_t const mapSize = storage.GetCountryFile(region).GetRemoteSize();
  NodeAttrs attrs;
  storage.GetNodeAttrs(region, attrs);
  TEST_EQUAL(attrs.m_status, NodeStatus::OnDiskOutOfDate, (region));
  TEST_EQUAL(attrs.m_mwmSize, mapSize + block.m_size, (region));

  // A not-downloaded region advertises its full fused size too (the user decides by
  // the real download cost), but its status stays the map one.
  CountryId const other = "Madagascar";
  uint64_t otherCoverage = 0;
  for (auto const index : coverage[other])
    otherCoverage += blocks[index].m_size;
  NodeAttrs otherAttrs;
  storage.GetNodeAttrs(other, otherAttrs);
  TEST_EQUAL(otherAttrs.m_status, NodeStatus::NotDownloaded, ());
  TEST_EQUAL(otherAttrs.m_mwmSize, storage.GetCountryFile(other).GetRemoteSize() + otherCoverage, ());

  // The setting off: the region reads map-only and complete again.
  storage.SetTerrainWithMaps(false);
  storage.GetNodeAttrs(region, attrs);
  TEST_EQUAL(attrs.m_status, NodeStatus::OnDisk, (region));
  TEST_EQUAL(attrs.m_mwmSize, mapSize, (region));
  TEST_EQUAL(attrs.m_downloadingProgress.m_bytesDownloaded, attrs.m_downloadingProgress.m_bytesTotal, ());
  storage.SetTerrainWithMaps(true);

  // The complete state: the on-disk terrain keeps the OnDisk status, joins the local
  // size and the progress stays full - the fused size is not only the missing bytes.
  tests_support::ScopedDir terrainDir(TERRAIN_DIR);
  tests_support::ScopedDir versionDir(terrainDir, strings::to_string(block.m_version));
  tests_support::ScopedFile blockFile(
      base::JoinPath(TERRAIN_DIR, strings::to_string(block.m_version), block.m_name + TERRAIN_FILE_EXT), "twm");
  Storage complete;
  complete.RegisterAllLocalMaps();
  ScanTerrain(complete);
  complete.GetNodeAttrs(region, attrs);
  TEST_EQUAL(attrs.m_status, NodeStatus::OnDisk, (region));
  TEST_EQUAL(attrs.m_mwmSize, mapSize + block.m_size, (region));
  TEST_EQUAL(attrs.m_localMwmSize, mapSize + block.m_size, (region));
  // The iOS downloadingSize is the unsigned difference of the two: no wrap allowed.
  TEST_GREATER_OR_EQUAL(attrs.m_downloadingMwmSize, attrs.m_localMwmSize, (region));
  TEST_EQUAL(attrs.m_downloadingProgress.m_bytesDownloaded, attrs.m_downloadingProgress.m_bytesTotal, ());
  TEST_EQUAL(attrs.m_downloadingProgress.m_bytesTotal, mapSize + block.m_size, (region));
}

UNIT_TEST(Storage_TerrainNodeAttrsGroupAndInFlight)
{
  WritableDirChanger const writableDirChanger(kTerrainTestDir, WritableDirChanger::SettingsDirPolicy::UseWritableDir);
  ScopedTerrainSettings const guardSettings;

  int64_t version = 0;
  std::vector<Storage::TerrainBlock> blocks;
  std::map<CountryId, std::vector<uint32_t>> coverage;
  {
    std::string content;
    GetPlatform().GetReader(TERRAIN_GRID_FILE)->ReadAsString(content);
    Storage::ParseTwmGridJson(content, version, blocks, coverage);
  }

  // One downloaded Norway leaf: the group is Partly (its other leafs are not an
  // update), the group size counts every block of the subtree once.
  CountryId const group = "Norway";
  CountryId leaf;
  TaskRunner runner;  // Never run: the enqueued terrain must stay in flight.
  Storage storage;
  ScanTerrain(storage);
  storage.SetDownloaderForTesting(std::make_unique<FakeMapFilesDownloader>(runner));
  storage.ForEachInSubtree(group, [&leaf, &coverage](CountryId const & id, bool groupNode)
  {
    if (!groupNode && leaf.empty() && coverage.count(id) > 0)
      leaf = id;
  });
  TEST(!leaf.empty(), ());

  tests_support::ScopedDir mapsDir(strings::to_string(storage.GetCurrentDataVersion()));
  tests_support::ScopedFile map(mapsDir, platform::CountryFile(leaf), MapFileType::Map);
  ResizeToRemote(map, storage, leaf);
  storage.RegisterAllLocalMaps();

  std::set<uint32_t> groupBlocks;
  storage.ForEachInSubtree(group, [&](CountryId const & id, bool groupNode)
  {
    if (groupNode)
      return;
    if (auto const it = coverage.find(id); it != coverage.end())
      groupBlocks.insert(it->second.begin(), it->second.end());
  });
  uint64_t groupCoverage = 0;
  for (auto const index : groupBlocks)
    groupCoverage += blocks[index].m_size;

  NodeAttrs attrs;
  storage.GetNodeAttrs(group, attrs);
  TEST_EQUAL(attrs.m_status, NodeStatus::Partly, ());  // Not lifted: missing leafs are not an update.
  NodeAttrs groupBase;
  storage.SetTerrainWithMaps(false);
  storage.GetNodeAttrs(group, groupBase);
  storage.SetTerrainWithMaps(true);
  TEST_EQUAL(attrs.m_mwmSize, groupBase.m_mwmSize + groupCoverage, ());

  // Queued terrain of a map-complete leaf lifts it to Downloading; the progress totals
  // the map plus the whole enqueued coverage (never started: the runner is not run).
  storage.DownloadTerrain(leaf);
  uint64_t leafCoverage = 0;
  for (auto const index : coverage[leaf])
    leafCoverage += blocks[index].m_size;
  NodeAttrs leafAttrs;
  storage.GetNodeAttrs(leaf, leafAttrs);
  TEST_EQUAL(leafAttrs.m_status, NodeStatus::Downloading, (leaf));
  TEST_EQUAL(leafAttrs.m_downloadingProgress.m_bytesTotal, storage.GetCountryFile(leaf).GetRemoteSize() + leafCoverage,
             (leaf));
  TEST_EQUAL(leafAttrs.m_downloadingProgress.m_bytesDownloaded, storage.GetCountryFile(leaf).GetRemoteSize(), (leaf));
}
}  // namespace country_info_getter_tests
