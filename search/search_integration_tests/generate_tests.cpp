#include "testing/testing.hpp"

#include "generator/generator_tests_support/test_mwm_builder.hpp"
#include "generator/generator_tests_support/test_with_classificator.hpp"

#include "generator/feature_builder.hpp"
#include "generator/osm2type.hpp"
#include "generator/osm_element.hpp"

#include "indexer/classificator.hpp"
#include "indexer/data_source.hpp"
#include "indexer/feature.hpp"
#include "indexer/feature_data.hpp"

#include "platform/local_country_file.hpp"

#include "base/stl_helpers.hpp"

#include <cstdint>
#include <set>
#include <string>
#include <utility>

using namespace generator::tests_support;
using namespace std;

namespace
{
class GenerateTest : public TestWithClassificator
{
public:
  void MakeFeature(TestMwmBuilder & builder, pair<string, string> const & tag,
                   m2::PointD const & pt)
  {
    OsmElement e;
    e.AddTag(tag.first, tag.second);

    FeatureParams params;
    ftype::GetNameAndType(&e, params);
    params.AddName("en", "xxx");

    FeatureBuilder1 fb;
    fb.SetParams(params);
    fb.SetCenter(pt);
    fb.GetMetadata().Set(feature::Metadata::FMD_TEST_ID, strings::to_string(m_lastId));
    ++m_lastId;

    TEST(builder.Add(fb), (fb));
  }

private:
  uint64_t m_lastId = 0;
};

UNIT_CLASS_TEST(GenerateTest, GenerateDeprecatedTypes)
{
  auto file = platform::LocalCountryFile::MakeForTesting("testCountry");

  {
    TestMwmBuilder builder(file, feature::DataHeader::country);

    // Deprecated types.
    MakeFeature(builder, {"office", "travel_agent"}, {0, 0});
    MakeFeature(builder, {"shop", "tailor"}, {1, 1});
    MakeFeature(builder, {"shop", "estate_agent"}, {2, 2});
  }

  FrozenDataSource dataSource;
  TEST_EQUAL(dataSource.Register(file).second, MwmSet::RegResult::Success, ());

  // New types.
  base::StringIL arr[] = {{"shop"}, {"office"}};

  Classificator const & cl = classif();
  set<uint32_t> types;
  for (auto const & s : arr)
    types.insert(cl.GetTypeByPath(s));

  int count = 0;
  auto const fn = [&](FeatureType & ft) {
    ++count;
    ft.ForEachType([&](uint32_t t) { TEST(types.count(t) > 0, (cl.GetReadableObjectName(t))); });
  };
  dataSource.ForEachInScale(fn, scales::GetUpperScale());

  TEST_EQUAL(count, 3, ());

  file.DeleteFromDisk(MapOptions::Map);
}
}  // namespace
