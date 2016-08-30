#include "testing/testing.hpp"

#include "indexer/features_vector.hpp"
#include "indexer/index.hpp"
#include "indexer/mwm_set.hpp"

#include "platform/local_country_file.hpp"

using namespace platform;

namespace
{
UNIT_TEST(GetFeatureNames)
{
  LocalCountryFile localFile = LocalCountryFile::MakeForTesting("minsk-pass");

  Index index;
  auto result = index.RegisterMap(localFile);
  TEST_EQUAL(result.second, MwmSet::RegResult::Success, ());

  auto const & id = result.first;
  MwmSet::MwmHandle handle = index.GetMwmHandleById(id);
  TEST(handle.IsAlive(), ());

  auto const * value = handle.GetValue<MwmValue>();
  FeaturesVector fv(value->m_cont, value->GetHeader(), value->m_table.get());
  string primary, secondary, readable;

  fv.ForEach([&](FeatureType & ft, uint32_t /* index */)
  {
    ft.GetPreferredNames(primary, secondary);
    if (!secondary.empty())
    {
      ft.GetReadableName(readable);
      TEST_EQUAL(secondary, readable, ());
    }
  });
}
}  // namespace
