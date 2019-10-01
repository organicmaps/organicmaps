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
#include <string>

#include "defines.hpp"

using namespace generator_integration_tests;

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
    TEST_EQUAL(geomeriesCnt, 6832, ());
    TEST_EQUAL(geometryPointsCnt, 512330, ());
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
    TestCountry(world, 945 /* fbsCnt */, 364406 /* geometryPointsCnt */, 334 /* pointCnt */,
                598 /* lineCnt */, 13 /* areaCnt */, 428 /* poiCnt */,
                172 /* cityTownOrVillageCnt */, 0 /* bookingHotelsCnt */);
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

    TestCountry(northAuckland, 1812066 /* fbsCnt */, 12195243 /* geometryPointsCnt */,
                1007483 /* pointCnt */, 205469 /* lineCnt */, 599114 /* areaCnt */,
                212188 /* poiCnt */, 521 /* cityTownOrVillageCnt */, 3557 /* bookingHotelsCnt */);
    TestCountry(northWellington, 797849 /* fbsCnt */, 7772223 /* geometryPointsCnt */,
                460516 /* pointCnt */, 87058 /* lineCnt */, 250275 /* areaCnt */,
                95705 /* poiCnt */, 297 /* cityTownOrVillageCnt */, 1062 /* bookingHotelsCnt */);
    TestCountry(southCanterbury, 637239 /* fbsCnt */, 6984529 /* geometryPointsCnt */,
                397939 /* pointCnt */, 81712 /* lineCnt */, 157588 /* areaCnt */,
                89491 /* poiCnt */, 331 /* cityTownOrVillageCnt */, 2085 /* bookingHotelsCnt */);
    TestCountry(southSouthland, 340630 /* fbsCnt */, 5343003 /* geometryPointsCnt */, 185980 /* pointCnt */,
                40124 /* lineCnt */, 114526 /* areaCnt */, 40630 /* poiCnt */,
                297 /* cityTownOrVillageCnt */, 1621 /* bookingHotelsCnt */);
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

    TestCountry(northAuckland, 1812066 /* fbsCnt */, 12195243 /* geometryPointsCnt */,
                1007483 /* pointCnt */, 205469 /* lineCnt */, 599114 /* areaCnt */,
                212188 /* poiCnt */, 521 /* cityTownOrVillageCnt */, 3557 /* bookingHotelsCnt */);
    TestCountry(northWellington, 797849 /* fbsCnt */, 7772223 /* geometryPointsCnt */,
                460516 /* pointCnt */, 87058 /* lineCnt */, 250275 /* areaCnt */,
                95705 /* poiCnt */, 297 /* cityTownOrVillageCnt */, 1062 /* bookingHotelsCnt */);
    TestCountry(southCanterbury, 637239 /* fbsCnt */, 6984529 /* geometryPointsCnt */,
                397939 /* pointCnt */, 81712 /* lineCnt */, 157588 /* areaCnt */,
                89491 /* poiCnt */, 331 /* cityTownOrVillageCnt */, 2085 /* bookingHotelsCnt */);
    size_t partner1CntReal = 0;
    TestCountry(southSouthland, 340632 /* fbsCnt */, 5343005 /* geometryPointsCnt */, 185982 /* pointCnt */,
                40124 /* lineCnt */, 114526 /* areaCnt */, 40630 /* poiCnt */,
                297 /* cityTownOrVillageCnt */, 1621 /* bookingHotelsCnt */, [&](auto const & fb) {
                  static auto const partner1 = classif().GetTypeByPath({"sponsored", "partner1"});
                  if (fb.HasType(partner1))
                    ++partner1CntReal;
                });
    TEST_EQUAL(partner1CntReal, 4, ());
    TestCountry(world, 945 /* fbsCnt */, 364406 /* geometryPointsCnt */, 334 /* pointCnt */,
                598 /* lineCnt */, 13 /* areaCnt */, 428 /* poiCnt */,
                172 /* cityTownOrVillageCnt */, 0 /* bookingHotelsCnt */);
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
    TestGeneratedFile(restrictions, 273283 /* fileSize */);
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
      std::string const & path, size_t fbsCnt, size_t geometryPointsCnt, size_t pointCnt, size_t lineCnt,
      size_t areaCnt, size_t poiCnt, size_t cityTownOrVillageCnt, size_t bookingHotelsCnt,
      std::function<void(feature::FeatureBuilder const &)> const & fn =
          [](feature::FeatureBuilder const &) {})
  {
    CHECK(Platform::IsFileExistsByFullPath(path), ());
    auto const fbs = feature::ReadAllDatRawFormat(path);
    size_t geometryPointsCntReal = 0;
    size_t pointCntReal = 0;
    size_t lineCntReal = 0;
    size_t areaCntReal = 0;
    size_t poiCntReal = 0;
    size_t cityTownOrVillageCntReal = 0;
    size_t bookingHotelsCntReal = 0;
    for (auto const & fb : fbs)
    {
      geometryPointsCntReal += fb.IsPoint() ? 1 : fb.GetPointsCount();
      if (fb.IsPoint())
        ++pointCntReal;
      else if (fb.IsLine())
        ++lineCntReal;
      else if (fb.IsArea())
        ++areaCntReal;

      auto static const & poiChecker = ftypes::IsPoiChecker::Instance();
      if (poiChecker(fb.GetTypes()))
        ++poiCntReal;

      if (ftypes::IsCityTownOrVillage(fb.GetTypes()))
        ++cityTownOrVillageCntReal;

      auto static const & bookingChecker = ftypes::IsBookingHotelChecker::Instance();
      if (bookingChecker(fb.GetTypes()))
        ++bookingHotelsCntReal;

      fn(fb);
    }

    TEST_EQUAL(fbs.size(), fbsCnt, ());
    TEST_EQUAL(geometryPointsCntReal, geometryPointsCnt, ());
    TEST_EQUAL(pointCntReal, pointCnt, ());
    TEST_EQUAL(lineCntReal, lineCnt, ());
    TEST_EQUAL(areaCntReal, areaCnt, ());
    TEST_EQUAL(poiCntReal, poiCnt, ());
    TEST_EQUAL(cityTownOrVillageCntReal, cityTownOrVillageCnt, ());
    TEST_EQUAL(bookingHotelsCntReal, bookingHotelsCnt, ());
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
