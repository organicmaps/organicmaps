#include "testing/testing.hpp"

#include "search/search_integration_tests/helpers.hpp"

#include "generator/generator_tests_support/test_mwm_builder.hpp"

#include "generator/feature_builder.hpp"
#include "generator/osm_element.hpp"
#include "generator/osm2type.hpp"

#include "indexer/classificator.hpp"
#include "indexer/feature_data.hpp"
#include "indexer/feature.hpp"
#include "indexer/index.hpp"

#include "platform/local_country_file.hpp"

using namespace search;
using namespace generator::tests_support;

namespace
{
uint64_t g_lastId = 0;

void MakeFeature(TestMwmBuilder & builder, pair<string, string> const & tag, m2::PointD const & pt)
{
  OsmElement e;
  e.AddTag(tag.first, tag.second);

  FeatureParams params;
  ftype::GetNameAndType(&e, params);
  params.AddName("en", "xxx");

  FeatureBuilder1 fb;
  fb.SetParams(params);
  fb.SetCenter(pt);
  fb.GetMetadataForTesting().Set(feature::Metadata::FMD_TEST_ID, strings::to_string(g_lastId));
  ++g_lastId;

  TEST(builder.Add(fb), (fb));
}

UNIT_CLASS_TEST(TestWithClassificator, GenerateDeprecatedTypes)
{
  auto file = platform::LocalCountryFile::MakeForTesting("testCountry");

  {
    TestMwmBuilder builder(file, feature::DataHeader::country);

    // Deprecated types.
    MakeFeature(builder, {"office", "travel_agent"}, {0, 0});
    MakeFeature(builder, {"shop", "tailor"}, {1, 1});
    MakeFeature(builder, {"shop", "estate_agent"}, {2, 2});
  }

  Index index;
  TEST_EQUAL(index.Register(file).second, MwmSet::RegResult::Success, ());

  // New types.
  StringIL arr[] = {{"shop"}, {"office"}};

  Classificator const & cl = classif();
  set<uint32_t> types;
  for (auto const & s : arr)
    types.insert(cl.GetTypeByPath(s));

  int count = 0;
  auto const fn = [&](FeatureType & ft)
  {
    ++count;
    ft.ForEachType([&](uint32_t t)
    {
      TEST(types.count(t) > 0, (cl.GetReadableObjectName(t)));
    });
  };
  index.ForEachInScale(fn, scales::GetUpperScale());

  TEST_EQUAL(count, 3, ());

  file.DeleteFromDisk(MapOptions::Map);
}
}  // namespace
