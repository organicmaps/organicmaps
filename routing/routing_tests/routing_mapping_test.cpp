#include "testing/testing.hpp"
#include "indexer/indexer_tests/test_mwm_set.hpp"

#include "map/feature_vec_model.hpp"

#include "routing/routing_mapping.hpp"

#include "platform/country_file.hpp"
#include "platform/local_country_file.hpp"
#include "platform/platform.hpp"
#include "platform/platform_tests_support/scoped_file.hpp"

#include "coding/file_name_utils.hpp"
#include "coding/file_writer.hpp"
#include "coding/internal/file_data.hpp"

using namespace routing;
using namespace tests;
using namespace platform;
using namespace platform::tests_support;

namespace
{
class LocalFileGenerator
{
public:
  LocalFileGenerator(string const & fileName)
    : m_countryFile(fileName)
    , m_testDataFile(platform::GetFileName(m_countryFile.GetName(), MapOptions::MapWithCarRouting,
                                           version::FOR_TESTING_SINGLE_MWM1),
                     ScopedFile::Mode::Create)
    , m_localFile(GetPlatform().WritableDir(), m_countryFile, version::FOR_TESTING_SINGLE_MWM1)
  {
    m_localFile.SyncWithDisk();
    TEST(m_localFile.OnDisk(MapOptions::MapWithCarRouting), ());
    GenerateNecessarySections(m_localFile);

    m_result = m_testSet.Register(m_localFile);
    TEST_EQUAL(m_result.second, MwmSet::RegResult::Success,
               ("Can't register temporary localFile map"));
  }

  TestMwmSet & GetMwmSet() { return m_testSet; }

  string const & GetCountryName() { return m_countryFile.GetName(); }

  size_t GetNumRefs() { return m_result.first.GetInfo()->GetNumRefs(); }

private:

  void GenerateNecessarySections(LocalCountryFile const & localFile)
  {
    FilesContainerW dataCont(localFile.GetPath(MapOptions::CarRouting));

    FileWriter w1 = dataCont.GetWriter(VERSION_FILE_TAG);
    version::WriteVersion(w1, my::SecondsSinceEpoch());
    FileWriter w2 = dataCont.GetWriter(ROUTING_MATRIX_FILE_TAG);
    w2.Write("smth", 4);
  }

  CountryFile m_countryFile;
  ScopedFile m_testDataFile;
  LocalCountryFile m_localFile;
  TestMwmSet m_testSet;
  pair<MwmSet::MwmId, MwmSet::RegResult> m_result;
};

UNIT_TEST(RoutingMappingCountryFileLockTest)
{
  LocalFileGenerator generator("1TestCountry");
  {
    RoutingMapping testMapping(generator.GetCountryName(), (generator.GetMwmSet()));
    TEST(testMapping.IsValid(), ());
    TEST_EQUAL(generator.GetNumRefs(), 1, ());
  }
  // Routing mapping must unlock the file after destruction.
  TEST_EQUAL(generator.GetNumRefs(), 0, ());
}

UNIT_TEST(IndexManagerLockManagementTest)
{
  string const fileName("1TestCountry");
  LocalFileGenerator generator(fileName);
  RoutingIndexManager manager([&fileName](m2::PointD const & q) { return fileName; },
                              generator.GetMwmSet());
  {
    auto testMapping = manager.GetMappingByName(fileName);
    TEST(testMapping->IsValid(), ());
    TEST_EQUAL(generator.GetNumRefs(), 1, ());
  }
  // We freed mapping, but it still persists inside the manager cache.
  TEST_EQUAL(generator.GetNumRefs(), 1, ());

  // Test cache clearing.
  manager.Clear();
  TEST_EQUAL(generator.GetNumRefs(), 0, ());
}

UNIT_TEST(FtSegIsInsideTest)
{
  OsrmMappingTypes::FtSeg seg(123, 1, 5);
  OsrmMappingTypes::FtSeg inside(123, 1, 2);
  TEST(OsrmMappingTypes::IsInside(seg, inside), ());
  OsrmMappingTypes::FtSeg inside2(123, 3, 4);
  TEST(OsrmMappingTypes::IsInside(seg, inside2), ());
  OsrmMappingTypes::FtSeg inside3(123, 4, 5);
  TEST(OsrmMappingTypes::IsInside(seg, inside3), ());
  OsrmMappingTypes::FtSeg bseg(123, 5, 1);
  TEST(OsrmMappingTypes::IsInside(bseg, inside), ());
  TEST(OsrmMappingTypes::IsInside(bseg, inside2), ());
  TEST(OsrmMappingTypes::IsInside(bseg, inside3), ());
}

UNIT_TEST(FtSegSplitSegmentiTest)
{
  OsrmMappingTypes::FtSeg seg(123, 1, 5);
  OsrmMappingTypes::FtSeg bseg(123, 5, 1);
  OsrmMappingTypes::FtSeg splitter(123, 2, 3);

  OsrmMappingTypes::FtSeg res1(123, 1, 3);
  TEST_EQUAL(res1, OsrmMappingTypes::SplitSegment(seg, splitter), ());

  OsrmMappingTypes::FtSeg res2(123, 5, 2);
  TEST_EQUAL(res2, OsrmMappingTypes::SplitSegment(bseg, splitter), ());
}
}  // namespace
