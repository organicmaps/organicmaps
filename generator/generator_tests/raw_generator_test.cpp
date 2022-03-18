#include "testing/testing.hpp"

#include "generator/generator_tests_support/test_generator.hpp"

#include "indexer/classificator.hpp"


namespace raw_generator_tests
{
using TestRawGenerator = generator::tests_support::TestRawGenerator;

// https://github.com/organicmaps/organicmaps/issues/2035
UNIT_CLASS_TEST(TestRawGenerator, Towns)
{
  std::string const mwmName = "Towns";
  BuildFB("./data/osm_test_data/towns.osm", mwmName);

  size_t count = 0;
  ForEachFB(mwmName, [&count](feature::FeatureBuilder const & fb)
  {
    ++count;
    //LOG(LINFO, (fb));

    static uint32_t const townType = classif().GetTypeByPath({"place", "town"});
    static uint32_t const villageType = classif().GetTypeByPath({"place", "village"});

    bool const isTown = (fb.GetName() == "El Dorado");
    TEST_EQUAL(isTown, fb.HasType(townType), ());
    TEST_NOT_EQUAL(isTown, fb.HasType(villageType), ());

    TEST(fb.GetRank() > 0, ());
  });

  TEST_EQUAL(count, 4, ());
}

} // namespace raw_generator_tests
