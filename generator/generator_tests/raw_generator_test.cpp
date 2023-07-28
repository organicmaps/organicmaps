#include "testing/testing.hpp"

#include "generator/generator_tests_support/test_generator.hpp"

#include "search/cities_boundaries_table.hpp"

#include "indexer/classificator.hpp"
#include "indexer/data_source.hpp"


namespace raw_generator_tests
{
using TestRawGenerator = generator::tests_support::TestRawGenerator;

uint32_t GetFeatureType(FeatureType & ft)
{
  uint32_t res = 0;
  ft.ForEachType([&res](uint32_t t)
  {
    TEST_EQUAL(res, 0, ());
    res = t;
  });
  return res;
}

// https://github.com/organicmaps/organicmaps/issues/2035
UNIT_CLASS_TEST(TestRawGenerator, Towns)
{
  uint32_t const townType = classif().GetTypeByPath({"place", "town"});
  uint32_t const villageType = classif().GetTypeByPath({"place", "village"});

  std::string const mwmName = "Towns";
  BuildFB("./data/osm_test_data/towns.osm", mwmName);

  size_t count = 0;
  ForEachFB(mwmName, [&](feature::FeatureBuilder const & fb)
  {
    ++count;
    //LOG(LINFO, (fb));

    bool const isTown = (fb.GetName() == "El Dorado");
    TEST_EQUAL(isTown, fb.HasType(townType), ());
    TEST_NOT_EQUAL(isTown, fb.HasType(villageType), ());

    TEST(fb.GetRank() > 0, ());
  });

  TEST_EQUAL(count, 4, ());

  // Prepare features data source.
  FrozenDataSource dataSource;
  std::vector<MwmSet::MwmId> mwmIDs;
  for (auto const & name : { mwmName, std::string(WORLD_FILE_NAME) })
  {
    BuildFeatures(name);
    BuildSearch(name);

    platform::LocalCountryFile localFile(platform::LocalCountryFile::MakeTemporary(GetMwmPath(name)));
    auto res = dataSource.RegisterMap(localFile);
    TEST_EQUAL(res.second, MwmSet::RegResult::Success, ());
    mwmIDs.push_back(std::move(res.first));
  }

  /// @todo We should have only 1 boundary here for the "El Dorado" town, because all other palces are villages.
  /// Now we have 2 boundaries + "Taylor" village, because it was transformed from place=city boundary above.
  /// This is not a blocker, but good to fix World generator for this case in future.

  // Load boundaries.
  search::CitiesBoundariesTable table(dataSource);
  TEST(table.Load(), ());
  TEST_EQUAL(table.GetSize(), 2, ());

  // Iterate for features in World.
  count = 0;
  FeaturesLoaderGuard guard(dataSource, mwmIDs[1]);
  for (size_t id = 0; id < guard.GetNumFeatures(); ++id)
  {
    auto ft = guard.GetFeatureByIndex(id);

    std::string_view const name = ft->GetName(StringUtf8Multilang::kDefaultCode);
    if (!name.empty())
    {
      TEST_EQUAL(ft->GetGeomType(), feature::GeomType::Point, ());

      search::CitiesBoundariesTable::Boundaries boundary;
      TEST(table.Get(id, boundary), ());
      TEST(boundary.HasPoint(ft->GetCenter()), ());

      if (name == "El Dorado")
        TEST_EQUAL(GetFeatureType(*ft), townType, ());

      ++count;
    }
  }

  TEST_EQUAL(count, 2, ());
}

} // namespace raw_generator_tests
