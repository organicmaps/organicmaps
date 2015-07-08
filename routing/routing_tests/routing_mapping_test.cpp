#include "testing/testing.hpp"

#include "indexer/indexer_tests/test_mwm_set.hpp"

#include "routing/routing_mapping.h"

#include "map/feature_vec_model.hpp"

#include "platform/country_file.hpp"
#include "platform/local_country_file.hpp"
#include "platform/platform_tests/scoped_local_file.hpp"
#include "platform/platform.hpp"

#include "coding/file_name_utils.hpp"
#include "coding/file_writer.hpp"
#include "coding/internal/file_data.hpp"

using namespace routing;
using namespace tests;
using namespace platform;
using namespace platform::tests;

// Underlying block is copypaste LocalFiles creation stub. I use a local copy to reduce including
// test facilities in project headers.
namespace
{
UNIT_TEST(RoutingMappingCountryFileLockTest)
{
  // Create the local files and the registered MwmSet.
  CountryFile countryFile("TestCountry");
  countryFile.SetRemoteSizes(1 /* mapSize */, 2 /* routingSize */);
  LocalCountryFile localFile(GetPlatform().WritableDir(), countryFile, 0 /* version */);
  ScopedTestFile testMapFile(countryFile.GetNameWithExt(TMapOptions::EMap), "map");
  ScopedTestFile testRoutingFile(countryFile.GetNameWithExt(TMapOptions::ECarRouting), "routing");
  localFile.SyncWithDisk();
  TEST(localFile.OnDisk(TMapOptions::EMapWithCarRouting), ());

  TestMwmSet mwmSet;
  pair<MwmSet::MwmHandle, MwmSet::RegResult> result = mwmSet.Register(localFile);
  TEST_EQUAL(result.second, MwmSet::RegResult::Success, ("Can't register temporary localFile map"));

  // Test RoutingMapping ctor and the file lock.
  {
    RoutingMapping testMapping(countryFile, (&mwmSet));
    TEST(testMapping.IsValid(), ());
    TEST_EQUAL(result.first.GetInfo()->GetLockCount(), 2, ());
  }
  // Routing mapping must unlock the file after destruction.
  TEST_EQUAL(result.first.GetInfo()->GetLockCount(), 1, ());
}

UNIT_TEST(IndexManagerLockManagementTest)
{
  // Create the local files and the registered MwmSet.
  CountryFile countryFile("TestCountry");
  countryFile.SetRemoteSizes(1 /* mapSize */, 2 /* routingSize */);
  LocalCountryFile localFile(GetPlatform().WritableDir(), countryFile, 0 /* version */);
  ScopedTestFile testMapFile(countryFile.GetNameWithExt(TMapOptions::EMap), "map");
  ScopedTestFile testRoutingFile(countryFile.GetNameWithExt(TMapOptions::ECarRouting), "routing");
  localFile.SyncWithDisk();
  TEST(localFile.OnDisk(TMapOptions::EMapWithCarRouting), ());

  TestMwmSet mwmSet;
  pair<MwmSet::MwmHandle, MwmSet::RegResult> result = mwmSet.Register(localFile);
  TEST_EQUAL(result.second, MwmSet::RegResult::Success, ("Can't register temporary localFile map"));

  RoutingIndexManager manager([](m2::PointD const & q){return "TestCountry";}, &mwmSet);
  {
    auto testMapping = manager.GetMappingByName("TestCountry");
    TEST(testMapping->IsValid(), ());
    TEST_EQUAL(result.first.GetInfo()->GetLockCount(), 2, ());
  }
  // We freed mapping, by it stil persist inside the manager cache.
  TEST_EQUAL(result.first.GetInfo()->GetLockCount(), 2, ());

  // Test cache clearing.
  manager.Clear();
  TEST_EQUAL(result.first.GetInfo()->GetLockCount(), 1, ());
}
}  // namespace
