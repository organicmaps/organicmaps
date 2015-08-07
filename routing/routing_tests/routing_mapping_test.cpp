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
      : m_countryFile(fileName),
        m_testMapFile(m_countryFile.GetNameWithExt(MapOptions::Map), "map"),
        m_testRoutingFile(m_countryFile.GetNameWithExt(MapOptions::CarRouting), "routing"),
        m_localFile(GetPlatform().WritableDir(), m_countryFile, 0 /* version */)
  {
    m_localFile.SyncWithDisk();
    TEST(m_localFile.OnDisk(MapOptions::MapWithCarRouting), ());
    GenerateVersionSections(m_localFile);

    m_result = m_testSet.Register(m_localFile);
    TEST_EQUAL(m_result.second, MwmSet::RegResult::Success,
               ("Can't register temporary localFile map"));
  }

  TestMwmSet & GetMwmSet() { return m_testSet; }

  CountryFile const & GetCountryFile() { return m_countryFile; }

  size_t GetNumRefs() { return m_result.first.GetInfo()->GetNumRefs(); }

private:
  void GenerateVersionSections(LocalCountryFile const & localFile)
  {
    FilesContainerW routingCont(localFile.GetPath(MapOptions::CarRouting));
    // Write version for routing file that is equal to correspondent mwm file.
    FilesContainerW mwmCont(localFile.GetPath(MapOptions::Map));

    FileWriter w1 = routingCont.GetWriter(VERSION_FILE_TAG);
    FileWriter w2 = mwmCont.GetWriter(VERSION_FILE_TAG);
    version::WriteVersion(w1);
    version::WriteVersion(w2);
  }

  CountryFile m_countryFile;
  ScopedFile m_testMapFile;
  ScopedFile m_testRoutingFile;
  LocalCountryFile m_localFile;
  TestMwmSet m_testSet;
  pair<MwmSet::MwmHandle, MwmSet::RegResult> m_result;
};

UNIT_TEST(RoutingMappingCountryFileLockTest)
{
  LocalFileGenerator generator("1TestCountry");
  {
    RoutingMapping testMapping(generator.GetCountryFile(), (&generator.GetMwmSet()));
    TEST(testMapping.IsValid(), ());
    TEST_EQUAL(generator.GetNumRefs(), 2, ());
  }
  // Routing mapping must unlock the file after destruction.
  TEST_EQUAL(generator.GetNumRefs(), 1, ());
}

UNIT_TEST(IndexManagerLockManagementTest)
{
  string const fileName("1TestCountry");
  LocalFileGenerator generator(fileName);
  RoutingIndexManager manager([&fileName](m2::PointD const & q) { return fileName; },
                              &generator.GetMwmSet());
  {
    auto testMapping = manager.GetMappingByName(fileName);
    TEST(testMapping->IsValid(), ());
    TEST_EQUAL(generator.GetNumRefs(), 2, ());
  }
  // We freed mapping, but it still persists inside the manager cache.
  TEST_EQUAL(generator.GetNumRefs(), 2, ());

  // Test cache clearing.
  manager.Clear();
  TEST_EQUAL(generator.GetNumRefs(), 1, ());
}
}  // namespace
