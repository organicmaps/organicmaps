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

using namespace feature;
using namespace generator::tests_support;
using namespace std;

namespace
{
class GenerateTest : public TestWithClassificator
{
public:
  void MakeFeature(TestMwmBuilder & builder, vector<pair<string, string>> const & tags, m2::PointD const & pt)
  {
    OsmElement e;
    for (auto const & tag : tags)
      e.AddTag(tag.first, tag.second);

    FeatureBuilderParams params;
    ftype::GetNameAndType(&e, params);
    params.AddName("en", "xxx");

    FeatureBuilder fb;
    fb.SetParams(params);
    fb.SetCenter(pt);
    fb.GetMetadata().Set(Metadata::FMD_TEST_ID, strings::to_string(m_lastId));
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
    TestMwmBuilder builder(file, DataHeader::MapType::Country);

    // Deprecated types.
    MakeFeature(builder, {{"leisure", "dog_park"}, {"sport", "tennis"}}, {0.0, 0.0});
    MakeFeature(builder, {{"leisure", "playground"}, {"sport", "tennis"}}, {1.0, 1.0});
  }

  FrozenDataSource dataSource;
  TEST_EQUAL(dataSource.Register(file).second, MwmSet::RegResult::Success, ());

  // New types.
  base::StringIL arr[] = {{"leisure", "dog_park"}, {"leisure", "playground"}, {"sport", "tennis"}};

  Classificator const & cl = classif();
  set<uint32_t> types;
  for (auto const & s : arr)
    types.insert(cl.GetTypeByPath(s));

  int count = 0;
  auto const fn = [&](FeatureType & ft)
  {
    ++count;
    ft.ForEachType([&](uint32_t t) { TEST(types.count(t) > 0, (cl.GetReadableObjectName(t))); });
  };
  dataSource.ForEachInScale(fn, scales::GetUpperScale());

  TEST_EQUAL(count, 2, ());

  file.DeleteFromDisk(MapFileType::Map);
}
}  // namespace
