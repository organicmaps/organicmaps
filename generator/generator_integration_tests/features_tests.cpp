// ~/Projects/omim_copy/data/
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

#include <string>

#include "defines.hpp"

using namespace generator_integration_tests;

class FeatureIntegrationTests
{
public:
  FeatureIntegrationTests()
  {
    Init("features-2019_07_17__13_39_20" /* arhiveName */);
  }

  ~FeatureIntegrationTests()
  {
    CHECK(Platform::RmDirRecursively(m_testPath), ());
  }

  void BuildCoasts()
  {
    auto const worldCoastsGeom = m_genInfo.GetIntermediateFileName(WORLD_COASTS_FILE_NAME".geom");
    auto const worldCoastsRawGeom = m_genInfo.GetIntermediateFileName(WORLD_COASTS_FILE_NAME".rawgeom");

    CHECK(!Platform::IsFileExistsByFullPath(worldCoastsGeom), ());
    CHECK(!Platform::IsFileExistsByFullPath(worldCoastsRawGeom), ());

    generator::RawGenerator rawGenerator(m_genInfo, m_threadCount);
    rawGenerator.GenerateCoasts();
    TEST(rawGenerator.Execute(), ());

    TEST(Platform::IsFileExistsByFullPath(worldCoastsGeom), ());
    TEST(Platform::IsFileExistsByFullPath(worldCoastsRawGeom), ());
    size_t fileSize = 0;
    CHECK(Platform::GetFileSizeByFullPath(worldCoastsGeom, fileSize), ());
    TEST_GREATER(fileSize, 0, ());
    CHECK(Platform::GetFileSizeByFullPath(worldCoastsRawGeom, fileSize), ());
    TEST_GREATER(fileSize, 0, ());

    auto const fbs = feature::ReadAllDatRawFormat(worldCoastsGeom);
    size_t geomeriesCnt = 0;
    size_t pointsCnt = 0;
    for (auto const & fb : fbs)
    {
      geomeriesCnt += fb.GetGeometry().size();
      pointsCnt += fb.GetPointsCount();
    }

    TEST_EQUAL(fbs.size(), 340, ());
    TEST_EQUAL(geomeriesCnt, 6832, ());
    TEST_EQUAL(pointsCnt, 512330, ());
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

    auto const fbs = feature::ReadAllDatRawFormat(world);
    size_t pointsCnt = 0;
    size_t airportCnt = 0;
    size_t cityTownOrVillageCnt = 0;
    size_t popularAttractionCnt = 0;
    for (auto const & fb : fbs)
    {
      pointsCnt += fb.IsPoint() ? 1 : fb.GetPointsCount();
      if (generator::FilterWorld::IsInternationalAirport(fb))
        ++airportCnt;

      if (generator::FilterWorld::IsPopularAttraction(fb, m_genInfo.m_popularPlacesFilename))
        ++popularAttractionCnt;

      if (ftypes::IsCityTownOrVillage(fb.GetTypes()))
        ++cityTownOrVillageCnt;
    }

    TEST_EQUAL(fbs.size(), 949, ());
    TEST_EQUAL(pointsCnt, 364410, ());
    TEST_EQUAL(airportCnt, 3, ());
    TEST_EQUAL(cityTownOrVillageCnt, 172, ());
    TEST_EQUAL(popularAttractionCnt, 146, ());
  }

  void BuildCountries()
  {
    m_genInfo.m_emitCoasts = true;
    m_genInfo.m_citiesBoundariesFilename = m_genInfo.GetIntermediateFileName("citiesboundaries.bin");
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

    TestCountry(northAuckland, 1811915 /* fbsCnt */, 12195892 /* pointsCnt */, 1007316 /* pointCnt */,
                205469 /* lineCnt */, 599130 /* areaCnt */, 212087 /* poiCnt */,
                521 /* cityTownOrVillageCnt */, 47 /* popularAttractionCnt */, 3557 /* bookingHotelsCnt */);
    TestCountry(northWellington, 797769 /* fbsCnt */, 7772261 /* pointsCnt */, 460437 /* pointCnt */,
                87058 /* lineCnt */, 250274 /* areaCnt */, 95651 /* poiCnt */,
                297 /* cityTownOrVillageCnt */, 17 /* popularAttractionCnt */, 1062 /* bookingHotelsCnt */);
    TestCountry(southCanterbury, 636922 /* fbsCnt */, 6984348 /* pointsCnt */, 397622 /* pointCnt */,
                81712 /* lineCnt */, 157588 /* areaCnt */, 89249 /* poiCnt */,
                331 /* cityTownOrVillageCnt */, 42 /* popularAttractionCnt */, 2085 /* bookingHotelsCnt */);
    TestCountry(southSouthland, 340491 /* fbsCnt */, 5342804 /* pointsCnt */, 185845 /* pointCnt */,
                40124 /* lineCnt */, 114522 /* areaCnt */, 40497 /* poiCnt */,
                297 /* cityTownOrVillageCnt */, 41 /* popularAttractionCnt */, 1621 /* bookingHotelsCnt */);
  }

  void CheckGeneratedData()
  {
    m_genInfo.m_emitCoasts = true;
    m_genInfo.m_citiesBoundariesFilename = m_genInfo.GetIntermediateFileName("citiesboundaries.bin");
    auto const cameraToWays = m_genInfo.GetIntermediateFileName(CAMERAS_TO_WAYS_FILENAME);
    auto const citiesAreas = m_genInfo.GetIntermediateFileName(CITIES_AREAS_TMP_FILENAME);
    auto const maxSpeeds = m_genInfo.GetIntermediateFileName(MAXSPEEDS_FILENAME);
    auto const metalines = m_genInfo.GetIntermediateFileName(METALINES_FILENAME);
    auto const restrictions = m_genInfo.GetIntermediateFileName(RESTRICTIONS_FILENAME);
    auto const roadAccess = m_genInfo.GetIntermediateFileName(ROAD_ACCESS_FILENAME);

    for (auto const & generatedFile : {cameraToWays, citiesAreas, maxSpeeds, metalines,
         restrictions, roadAccess, m_genInfo.m_citiesBoundariesFilename})
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
    TestGeneratedFile(metalines, 306228 /* fileSize */);
    TestGeneratedFile(restrictions, 273283 /* fileSize */);
    TestGeneratedFile(roadAccess, 1918315 /* fileSize */);
    TestGeneratedFile(m_genInfo.m_citiesBoundariesFilename, 2435 /* fileSize */);
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
  void TestCountry(std::string const & path, size_t fbsCnt, size_t pointsCnt, size_t pointCnt,
                   size_t lineCnt, size_t areaCnt, size_t poiCnt, size_t cityTownOrVillageCnt,
                   size_t popularAttractionCnt, size_t bookingHotelsCnt)
  {
    CHECK(Platform::IsFileExistsByFullPath(path), ());
    auto const fbs = feature::ReadAllDatRawFormat(path);
    size_t pointsCntReal = 0;
    size_t pointCntReal = 0;
    size_t lineCntReal = 0;
    size_t areaCntReal = 0;
    size_t poiCntReal = 0;
    size_t cityTownOrVillageCntReal = 0;
    size_t popularAttractionCntReal = 0;
    size_t bookingHotelsCntReal = 0;
    for (auto const & fb : fbs)
    {
      pointsCntReal += fb.IsPoint() ? 1 : fb.GetPointsCount();
      if (fb.IsPoint())
        ++pointCntReal;
      else if (fb.IsLine())
        ++lineCntReal;
      else if (fb.IsArea())
        ++areaCntReal;

      auto static const & poiChecker = ftypes::IsPoiChecker::Instance();
      if (poiChecker(fb.GetTypes()))
        ++poiCntReal;

      if (generator::FilterWorld::IsPopularAttraction(fb, m_genInfo.m_popularPlacesFilename))
        ++popularAttractionCntReal;

      if (ftypes::IsCityTownOrVillage(fb.GetTypes()))
        ++cityTownOrVillageCntReal;

      auto static const & bookingChecker = ftypes::IsBookingHotelChecker::Instance();
      if (bookingChecker(fb.GetTypes()))
        ++bookingHotelsCntReal;
    }

    TEST_EQUAL(fbs.size(), fbsCnt, ());
    TEST_EQUAL(pointsCntReal, pointsCnt, ());
    TEST_EQUAL(pointCntReal, pointCnt, ());
    TEST_EQUAL(lineCntReal, lineCnt, ());
    TEST_EQUAL(areaCntReal, areaCnt, ());
    TEST_EQUAL(poiCntReal, poiCnt, ());
    TEST_EQUAL(cityTownOrVillageCntReal, cityTownOrVillageCnt, ());
    TEST_EQUAL(popularAttractionCntReal, popularAttractionCnt, ());
    TEST_EQUAL(bookingHotelsCntReal, bookingHotelsCnt, ());
  }

  void TestGeneratedFile(std::string const & path, size_t fileSize)
  {
    TEST(Platform::IsFileExistsByFullPath(path), (path));
    size_t fileSizeReal = 0;
    CHECK(Platform::GetFileSizeByFullPath(path, fileSizeReal), (path));
    TEST_EQUAL(fileSizeReal, fileSize, (path));
  }

  void Init(std::string const & arhiveName)
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
    m_genInfo.m_intermediateDir = base::JoinPath(m_testPath, arhiveName, "intermediate_data");
    m_genInfo.m_targetDir = m_genInfo.m_intermediateDir;
    m_genInfo.m_tmpDir = base::JoinPath(m_genInfo.m_intermediateDir, "tmp");
    m_genInfo.m_osmFileName = base::JoinPath(m_testPath, "planet.o5m");
    m_genInfo.m_popularPlacesFilename = m_genInfo.GetIntermediateFileName("popular_places.csv");
    m_genInfo.m_idToWikidataFilename = m_genInfo.GetIntermediateFileName("wiki_urls.csv");
    DecompressZipArchive(base::JoinPath(options.m_dataPath, arhiveName + ".zip"), m_testPath);
  }

  size_t m_threadCount;
  std::string m_testPath;
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
