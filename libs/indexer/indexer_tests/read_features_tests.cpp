#include "testing/testing.hpp"

#include "indexer/data_source.hpp"

#include "generator/generator_tests_support/test_with_classificator.hpp"

#include "platform/local_country_file.hpp"

#include <cstdint>
#include <memory>
#include <vector>

namespace read_features_tests
{
using namespace generator::tests_support;
using namespace std;

UNIT_CLASS_TEST(TestWithClassificator, ReadFeatures_Smoke)
{
  FrozenDataSource dataSource;
  dataSource.RegisterMap(platform::LocalCountryFile::MakeForTesting("minsk-pass"));

  vector<shared_ptr<MwmInfo>> infos;
  dataSource.GetMwmsInfo(infos);
  CHECK_EQUAL(infos.size(), 1, ());

  auto handle = dataSource.GetMwmHandleById(MwmSet::MwmId(infos[0]));

  FeaturesLoaderGuard const guard(dataSource, handle.GetId());
  LOG(LINFO, (guard.GetNumFeatures()));
  for (uint32_t i = 0; i + 1 < guard.GetNumFeatures(); ++i)
  {
    auto ft1 = guard.GetFeatureByIndex(i);
    auto ft2 = guard.GetFeatureByIndex(i + 1);

    ft2->ForEachType([](auto const /* t */) {});
    ft1->ForEachType([](auto const /* t */) {});
  }
}
}  // namespace read_features_tests
