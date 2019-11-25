#include "testing/testing.hpp"

#include "generator/generator_integration_tests/helpers.hpp"

#include "generator/feature_builder.hpp"
#include "generator/filter_world.hpp"
#include "generator/generate_info.hpp"
#include "generator/raw_generator.hpp"

#include "indexer/classificator_loader.hpp"
#include "indexer/ftypes_matcher.hpp"

#include "platform/platform.hpp"

#include "base/assert.hpp"
#include "base/file_name_utils.hpp"
#include "base/scope_guard.hpp"

#include <cstdio>
#include <sstream>
#include <string>

#include "defines.hpp"

using namespace generator_integration_tests;

struct CountryFeaturesCounters
{
  size_t m_fbs = 0;
  size_t m_geometryPoints = 0;
  size_t m_point = 0;
  size_t m_line = 0;
  size_t m_area = 0;
  size_t m_poi = 0;
  size_t m_cityTownOrVillage = 0;
  size_t m_bookingHotels = 0;

  CountryFeaturesCounters() = default;

  constexpr CountryFeaturesCounters(size_t fbs, size_t geometryPoints, size_t point, size_t line,
                                    size_t area, size_t poi, size_t cityTownOrVillage,
                                    size_t bookingHotels)
    : m_fbs(fbs)
    , m_geometryPoints(geometryPoints)
    , m_point(point)
    , m_line(line)
    , m_area(area)
    , m_poi(poi)
    , m_cityTownOrVillage(cityTownOrVillage)
    , m_bookingHotels(bookingHotels)
  {
  }

  CountryFeaturesCounters operator+(CountryFeaturesCounters const & rhs) const
  {
    return CountryFeaturesCounters(m_fbs + rhs.m_fbs, m_geometryPoints + rhs.m_geometryPoints,
                                   m_point + rhs.m_point, m_line + rhs.m_line, m_area + rhs.m_area,
                                   m_poi + rhs.m_poi, m_cityTownOrVillage + rhs.m_cityTownOrVillage,
                                   m_bookingHotels + rhs.m_bookingHotels);
  }

  bool operator==(CountryFeaturesCounters const & rhs) const
  {
    return m_fbs == rhs.m_fbs && m_geometryPoints == rhs.m_geometryPoints &&
           m_point == rhs.m_point && m_line == rhs.m_line && m_area == rhs.m_area &&
           m_poi == rhs.m_poi && m_cityTownOrVillage == rhs.m_cityTownOrVillage &&
           m_bookingHotels == rhs.m_bookingHotels;
  }
};

std::string DebugPrint(CountryFeaturesCounters const & cnt)
{
  std::ostringstream out;
  out << "CountryFeaturesCount(fbs = " << cnt.m_fbs << ", geometryPoints = " << cnt.m_geometryPoints
      << ", point = " << cnt.m_point << ", line = " << cnt.m_line << ", area = " << cnt.m_area
      << ", poi = " << cnt.m_poi << ", cityTownOrVillage = " << cnt.m_cityTownOrVillage
      << ", bookingHotels = " << cnt.m_bookingHotels << ")";
  return out.str();
}

CountryFeaturesCounters constexpr kWorldCounters(945 /* fbs */, 364406 /* geometryPoints */,
                                                 334 /* point */, 598 /* line */, 13 /* area */,
                                                 428 /* poi */, 172 /* cityTownOrVillage */,
                                                 0 /* bookingHotels */);

CountryFeaturesCounters constexpr kNorthAucklandCounters(
    1812220 /* fbs */, 12197083 /* geometryPoints */, 1007483 /* point */, 205623 /* line */,
    599114 /* area */, 212342 /* poi */, 521 /* cityTownOrVillage */, 3557 /* bookingHotels */);

CountryFeaturesCounters constexpr kNorthWellingtonCounters(
    797963 /* fbs */, 7773101 /* geometryPoints */, 460516 /* point */, 87172 /* line */,
    250275 /* area */, 95819 /* poi */, 297 /* cityTownOrVillage */, 1062 /* bookingHotels */);

CountryFeaturesCounters constexpr kSouthCanterburyCounters(
    637282 /* fbs */, 6985098 /* geometryPoints */, 397939 /* point */, 81755 /* line */,
    157588 /* area */, 89534 /* poi */, 331 /* cityTownOrVillage */, 2085 /* bookingHotels */);

CountryFeaturesCounters constexpr kSouthSouthlandCounters(
    340647 /* fbs */, 5342775 /* geometryPoints */, 185980 /* point */, 40141 /* line */,
    114526 /* area */, 40647 /* poi */, 297 /* cityTownOrVillage */, 1621 /* bookingHotels */);

CountryFeaturesCounters constexpr kSouthSouthlandMixedNodesCounters(
    2 /* fbs */, 2 /* geometryPoints */, 2 /* point */, 0 /* line */, 0 /* area */, 0 /* poi */,
    0 /* cityTownOrVillage */, 0 /* bookingHotels */);

class FeatureIntegrationTests
{
public:
  FeatureIntegrationTests()
  {
    // You can get features-2019_07_17__13_39_20 by running:
    // rsync -v -p testdata.mapsme.cloud.devmail.ru::testdata/features-2019_07_17__13_39_20.zip .
    Init("features-2019_07_17__13_39_20" /* archiveName */);
  }

  ~FeatureIntegrationTests()
  {
    CHECK(Platform::RmDirRecursively(m_testPath), ());
    CHECK(Platform::RemoveFileIfExists(m_mixedNodesFilenames.first), ());
    CHECK(Platform::RemoveFileIfExists(m_mixedTagsFilenames.first), ());
    CHECK_EQUAL(
        std::rename(m_mixedNodesFilenames.second.c_str(), m_mixedNodesFilenames.first.c_str()), 0,
        ());
    CHECK_EQUAL(
        std::rename(m_mixedTagsFilenames.second.c_str(), m_mixedTagsFilenames.first.c_str()), 0,
        ());
  }

  void BuildCoasts()
  {
    auto const worldCoastsGeom = m_genInfo.GetIntermediateFileName(WORLD_COASTS_FILE_NAME ".geom");
    auto const worldCoastsRawGeom =
        m_genInfo.GetIntermediateFileName(WORLD_COASTS_FILE_NAME ".rawgeom");

    CHECK(!Platform::IsFileExistsByFullPath(worldCoastsGeom), ());
    CHECK(!Platform::IsFileExistsByFullPath(worldCoastsRawGeom), ());

    generator::RawGenerator rawGenerator(m_genInfo, m_threadCount);
    rawGenerator.GenerateCoasts();
    TEST(rawGenerator.Execute(), ());

    TEST(Platform::IsFileExistsByFullPath(worldCoastsGeom), ());
    TEST(Platform::IsFileExistsByFullPath(worldCoastsRawGeom), ());
    uint64_t fileSize = 0;
    CHECK(Platform::GetFileSizeByFullPath(worldCoastsGeom, fileSize), ());
    TEST_GREATER(fileSize, 0, ());
    CHECK(Platform::GetFileSizeByFullPath(worldCoastsRawGeom, fileSize), ());
    TEST_GREATER(fileSize, 0, ());

    auto const fbs = feature::ReadAllDatRawFormat(worldCoastsGeom);
    size_t geomeriesCnt = 0;
    size_t geometryPointsCnt = 0;
    for (auto const & fb : fbs)
    {
      geomeriesCnt += fb.GetGeometry().size();
      geometryPointsCnt += fb.GetPointsCount();
    }

    TEST_EQUAL(fbs.size(), 340, ());
    TEST_EQUAL(geomeriesCnt, 6814, ());
    TEST_EQUAL(geometryPointsCnt, 512102, ());
  }

  void BuildWorld()
  {
    auto const world = m_genInfo.GetTmpFileName(WORLD_FILE_NAME);
    CHECK(!Platform::IsFileExistsByFullPath(world), ());

    generator::RawGenerator rawGenerator(m_genInfo, m_threadCount);
    rawGenerator.GenerateCoasts();
    rawGenerator.GenerateWorld();
    rawGenerator.GenerateCountries();
    TEST(rawGenerator.Execute(), ());

    TEST(Platform::IsFileExistsByFullPath(world), ());

    TestCountry(world, kWorldCounters);
  }

  void BuildCountries()
  {
    m_genInfo.m_emitCoasts = true;
    m_genInfo.m_citiesBoundariesFilename =
        m_genInfo.GetIntermediateFileName("citiesboundaries.bin");
    m_genInfo.m_bookingDataFilename = m_genInfo.GetIntermediateFileName("hotels.csv");

    auto const northAuckland = m_genInfo.GetTmpFileName("New Zealand North_Auckland");
    auto const northWellington = m_genInfo.GetTmpFileName("New Zealand North_Wellington");
    auto const southCanterbury = m_genInfo.GetTmpFileName("New Zealand South_Canterbury");
    auto const southSouthland = m_genInfo.GetTmpFileName("New Zealand South_Southland");
    for (auto const & mwmTmp : {northAuckland, northWellington, southCanterbury, southSouthland})
      CHECK(!Platform::IsFileExistsByFullPath(mwmTmp), (mwmTmp));

    generator::RawGenerator rawGenerator(m_genInfo, m_threadCount);
    rawGenerator.GenerateCoasts();
    rawGenerator.GenerateCountries();
    TEST(rawGenerator.Execute(), ());

    TestCountry(northAuckland, kNorthAucklandCounters);
    TestCountry(northWellington, kNorthWellingtonCounters);
    TestCountry(southCanterbury, kSouthCanterburyCounters);
    TestCountry(southSouthland, kSouthSouthlandCounters);
  }

  void CheckMixedTagsAndNodes()
  {
    m_genInfo.m_emitCoasts = true;
    m_genInfo.m_citiesBoundariesFilename =
        m_genInfo.GetIntermediateFileName("citiesboundaries.bin");
    m_genInfo.m_bookingDataFilename = m_genInfo.GetIntermediateFileName("hotels.csv");

    auto const northAuckland = m_genInfo.GetTmpFileName("New Zealand North_Auckland");
    auto const northWellington = m_genInfo.GetTmpFileName("New Zealand North_Wellington");
    auto const southCanterbury = m_genInfo.GetTmpFileName("New Zealand South_Canterbury");
    auto const southSouthland = m_genInfo.GetTmpFileName("New Zealand South_Southland");
    auto const world = m_genInfo.GetTmpFileName("World");
    auto const counties = {northAuckland, northWellington, southCanterbury, southSouthland, world};
    for (auto const & mwmTmp : counties)
      CHECK(!Platform::IsFileExistsByFullPath(mwmTmp), (mwmTmp));

    generator::RawGenerator rawGenerator(m_genInfo, m_threadCount);
    rawGenerator.GenerateCoasts();
    rawGenerator.GenerateCountries(true /* needMixTagsAndNodes */);
    rawGenerator.GenerateWorld(true /* needMixTags */);
    TEST(rawGenerator.Execute(), ());

    TestCountry(northAuckland, kNorthAucklandCounters);
    TestCountry(northWellington, kNorthWellingtonCounters);
    TestCountry(southCanterbury, kSouthCanterburyCounters);

    size_t partner1CntReal = 0;

    TestCountry(southSouthland, kSouthSouthlandCounters + kSouthSouthlandMixedNodesCounters,
                [&](auto const & fb) {
                  static auto const partner1 = classif().GetTypeByPath({"sponsored", "partner1"});
                  if (fb.HasType(partner1))
                    ++partner1CntReal;
                });

    TEST_EQUAL(partner1CntReal, 4, ());

    TestCountry(world, kWorldCounters);
  }

  void CheckGeneratedData()
  {
    m_genInfo.m_emitCoasts = true;
    m_genInfo.m_citiesBoundariesFilename =
        m_genInfo.GetIntermediateFileName("citiesboundaries.bin");
    auto const cameraToWays = m_genInfo.GetIntermediateFileName(CAMERAS_TO_WAYS_FILENAME);
    auto const citiesAreas = m_genInfo.GetIntermediateFileName(CITIES_AREAS_TMP_FILENAME);
    auto const maxSpeeds = m_genInfo.GetIntermediateFileName(MAXSPEEDS_FILENAME);
    auto const metalines = m_genInfo.GetIntermediateFileName(METALINES_FILENAME);
    auto const restrictions = m_genInfo.GetIntermediateFileName(RESTRICTIONS_FILENAME);
    auto const roadAccess = m_genInfo.GetIntermediateFileName(ROAD_ACCESS_FILENAME);

    for (auto const & generatedFile :
         {cameraToWays, citiesAreas, maxSpeeds, metalines, restrictions, roadAccess,
          m_genInfo.m_citiesBoundariesFilename})
    {
      CHECK(!Platform::IsFileExistsByFullPath(generatedFile), (generatedFile));
    }

    generator::RawGenerator rawGenerator(m_genInfo, m_threadCount);
    rawGenerator.GenerateCoasts();
    rawGenerator.GenerateCountries();
    TEST(rawGenerator.Execute(), ());

    TestGeneratedFile(cameraToWays, 0 /* fileSize */);
    TestGeneratedFile(citiesAreas, 18601 /* fileSize */);
    TestGeneratedFile(maxSpeeds, 1301515 /* fileSize */);
    TestGeneratedFile(metalines, 288032 /* fileSize */);
    TestGeneratedFile(restrictions, 371110 /* fileSize */);
    TestGeneratedFile(roadAccess, 1969045 /* fileSize */);
    TestGeneratedFile(m_genInfo.m_citiesBoundariesFilename, 87 /* fileSize */);
  }

  void BuildWorldOneThread()
  {
    m_threadCount = 1;
    BuildWorld();
  }

  void BuildCountriesOneThread()
  {
    m_threadCount = 1;
    BuildCountries();
  }

  void CheckGeneratedDataOneThread()
  {
    m_threadCount = 1;
    CheckGeneratedData();
  }

private:
  void TestCountry(
      std::string const & path, CountryFeaturesCounters const & expected,
      std::function<void(feature::FeatureBuilder const &)> const & fn =
          [](feature::FeatureBuilder const &) {})
  {
    CHECK(Platform::IsFileExistsByFullPath(path), ());
    auto const fbs = feature::ReadAllDatRawFormat(path);
    CountryFeaturesCounters actual;
    actual.m_fbs = fbs.size();
    for (auto const & fb : fbs)
    {
      actual.m_geometryPoints += fb.IsPoint() ? 1 : fb.GetPointsCount();
      if (fb.IsPoint())
        ++actual.m_point;
      else if (fb.IsLine())
        ++actual.m_line;
      else if (fb.IsArea())
        ++actual.m_area;

      auto static const & poiChecker = ftypes::IsPoiChecker::Instance();
      if (poiChecker(fb.GetTypes()))
        ++actual.m_poi;

      if (ftypes::IsCityTownOrVillage(fb.GetTypes()))
        ++actual.m_cityTownOrVillage;

      auto static const & bookingChecker = ftypes::IsBookingHotelChecker::Instance();
      if (bookingChecker(fb.GetTypes()))
        ++actual.m_bookingHotels;

      fn(fb);
    }

    TEST_EQUAL(actual, expected, ());
  }

  void TestGeneratedFile(std::string const & path, size_t fileSize)
  {
    TEST(Platform::IsFileExistsByFullPath(path), (path));
    uint64_t fileSizeReal = 0;
    CHECK(Platform::GetFileSizeByFullPath(path, fileSizeReal), (path));
    TEST_EQUAL(fileSizeReal, fileSize, (path));
  }

  void Init(std::string const & archiveName)
  {
    classificator::Load();
    auto const & options = GetTestingOptions();
    auto & platform = GetPlatform();
    platform.SetResourceDir(options.m_resourcePath);
    platform.SetSettingsDir(options.m_resourcePath);
    m_threadCount = static_cast<size_t>(platform.CpuCores());
    m_testPath = base::JoinPath(platform.WritableDir(), "gen-test");
    m_genInfo.SetNodeStorageType("map");
    m_genInfo.SetOsmFileType("o5m");
    m_genInfo.m_intermediateDir = base::JoinPath(m_testPath, archiveName, "intermediate_data");
    m_genInfo.m_targetDir = m_genInfo.m_intermediateDir;
    m_genInfo.m_tmpDir = base::JoinPath(m_genInfo.m_intermediateDir, "tmp");
    m_genInfo.m_osmFileName = base::JoinPath(m_testPath, "planet.o5m");
    m_genInfo.m_popularPlacesFilename = m_genInfo.GetIntermediateFileName("popular_places.csv");
    m_genInfo.m_idToWikidataFilename = m_genInfo.GetIntermediateFileName("wiki_urls.csv");
    DecompressZipArchive(base::JoinPath(options.m_dataPath, archiveName + ".zip"), m_testPath);

    m_mixedNodesFilenames.first = base::JoinPath(platform.ResourcesDir(), MIXED_NODES_FILE);
    m_mixedNodesFilenames.second = base::JoinPath(platform.ResourcesDir(), MIXED_NODES_FILE "_");
    m_mixedTagsFilenames.first = base::JoinPath(platform.ResourcesDir(), MIXED_TAGS_FILE);
    m_mixedTagsFilenames.second = base::JoinPath(platform.ResourcesDir(), MIXED_TAGS_FILE "_");

    CHECK_EQUAL(
        std::rename(m_mixedNodesFilenames.first.c_str(), m_mixedNodesFilenames.second.c_str()), 0,
        ());
    CHECK_EQUAL(
        std::rename(m_mixedTagsFilenames.first.c_str(), m_mixedTagsFilenames.second.c_str()), 0,
        ());
    std::string const fakeNodes =
        "sponsored=partner1\n"
        "lat=-46.43525\n"
        "lon=168.35674\n"
        "banner_url=https://localads.maps.me/redirects/test\n"
        "name=Test name1\n"
        "\n"
        "sponsored=partner1\n"
        "lat=-46.43512\n"
        "lon=168.35359\n"
        "banner_url=https://localads.maps.me/redirects/test\n"
        "name=Test name2\n";
    WriteToFile(m_mixedNodesFilenames.first, fakeNodes);

    std::string const mixesTags =
        "way,548504067,sponsored=partner1,banner_url=https://localads.maps.me/redirects/"
        "test,name=Test name3\n"
        "way,548504066,sponsored=partner1,banner_url=https://localads.maps.me/redirects/"
        "test,name=Test name4\n";
    WriteToFile(m_mixedTagsFilenames.first, mixesTags);
  }

  void WriteToFile(std::string const & filename, std::string const & data)
  {
    std::ofstream stream;
    stream.exceptions(std::fstream::failbit | std::fstream::badbit);
    stream.open(filename);
    stream << data;
  }

  size_t m_threadCount;
  std::string m_testPath;
  // m_mixedNodesFilenames and m_mixedTagsFilenames contain old and new filenames.
  // This is necessary to replace a file and then restore an old one back.
  std::pair<std::string, std::string> m_mixedNodesFilenames;
  std::pair<std::string, std::string> m_mixedTagsFilenames;
  feature::GenerateInfo m_genInfo;
};

UNIT_CLASS_TEST(FeatureIntegrationTests, BuildCoasts)
{
  FeatureIntegrationTests::BuildCoasts();
}

UNIT_CLASS_TEST(FeatureIntegrationTests, BuildWorld)
{
  FeatureIntegrationTests::BuildWorld();
}

UNIT_CLASS_TEST(FeatureIntegrationTests, BuildCountries)
{
  FeatureIntegrationTests::BuildCountries();
}

UNIT_CLASS_TEST(FeatureIntegrationTests, CheckMixedTagsAndNodes)
{
  FeatureIntegrationTests::CheckMixedTagsAndNodes();
}

UNIT_CLASS_TEST(FeatureIntegrationTests, CheckGeneratedData)
{
  FeatureIntegrationTests::CheckGeneratedData();
}

UNIT_CLASS_TEST(FeatureIntegrationTests, BuildWorldOneThread)
{
  FeatureIntegrationTests::BuildWorldOneThread();
}

UNIT_CLASS_TEST(FeatureIntegrationTests, BuildCountriesOneThread)
{
  FeatureIntegrationTests::BuildCountriesOneThread();
}

UNIT_CLASS_TEST(FeatureIntegrationTests, CheckGeneratedDataOneThread)
{
  FeatureIntegrationTests::CheckGeneratedDataOneThread();
}
