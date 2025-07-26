#include "testing/testing.hpp"

#include "indexer/classificator.hpp"
#include "indexer/classificator_loader.hpp"
#include "indexer/data_source.hpp"
#include "indexer/feature.hpp"

#include "coding/string_utf8_multilang.hpp"

#include <iostream>

UNIT_TEST(World_Capitals)
{
  classificator::Load();
  auto const capitalType = classif().GetTypeByPath({"place", "city", "capital", "2"});
  std::set<std::string_view> testCapitals = {"Lisbon", "Warsaw", "Kyiv", "Roseau"};

  platform::LocalCountryFile localFile(platform::LocalCountryFile::MakeForTesting(WORLD_FILE_NAME));

  FrozenDataSource dataSource;
  auto const res = dataSource.RegisterMap(localFile);
  TEST_EQUAL(res.second, MwmSet::RegResult::Success, ());

  size_t capitalsCount = 0;

  FeaturesLoaderGuard guard(dataSource, res.first);
  size_t const count = guard.GetNumFeatures();
  for (size_t id = 0; id < count; ++id)
  {
    auto ft = guard.GetFeatureByIndex(id);
    if (ft->GetGeomType() != feature::GeomType::Point)
      continue;

    bool found = false;
    ft->ForEachType([&found, capitalType](uint32_t t)
    {
      if (t == capitalType)
        found = true;
    });

    if (found)
      ++capitalsCount;

    std::string_view const name = ft->GetName(StringUtf8Multilang::kEnglishCode);
    if (testCapitals.count(name) > 0)
      TEST(found, (name));
  }

  // Got 225 values from the first launch. May vary slightly ..
  TEST_GREATER_OR_EQUAL(capitalsCount, 215, ());
}
