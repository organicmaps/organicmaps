#include "testing/testing.hpp"

#include "indexer/centers_table.hpp"
#include "indexer/classificator_loader.hpp"
#include "indexer/data_header.hpp"
#include "indexer/feature_algo.hpp"
#include "indexer/features_vector.hpp"

#include "platform/platform.hpp"

#include "coding/reader.hpp"
#include "coding/writer.hpp"

#include "geometry/mercator.hpp"
#include "geometry/point2d.hpp"

#include "base/file_name_utils.hpp"

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

using namespace search;
using namespace std;

namespace
{
using TBuffer = vector<uint8_t>;

struct CentersTableTest
{
  CentersTableTest() { classificator::Load(); }
};

UNIT_CLASS_TEST(CentersTableTest, Smoke)
{
  string const kMap = base::JoinPath(GetPlatform().WritableDir(), "minsk-pass.mwm");

  FeaturesVectorTest fv(kMap);

  TBuffer buffer;

  {
    CentersTableBuilder builder;
    feature::DataHeader header(kMap);

    builder.SetGeometryParams(header.GetBounds());
    fv.GetVector().ForEach([&](FeatureType & ft, uint32_t id) { builder.Put(id, feature::GetCenter(ft)); });

    MemWriter<TBuffer> writer(buffer);
    builder.Freeze(writer);
  }

  {
    MemReader reader(buffer.data(), buffer.size());
    auto table = CentersTable::LoadV1(reader);
    TEST(table.get(), ());

    fv.GetVector().ForEach([&](FeatureType & ft, uint32_t id)
    {
      m2::PointD actual;
      TEST(table->Get(id, actual), ());

      m2::PointD expected = feature::GetCenter(ft);

      TEST_LESS_OR_EQUAL(mercator::DistanceOnEarth(actual, expected), 1.0, (id));
    });
  }
}

UNIT_CLASS_TEST(CentersTableTest, SmokeV0)
{
  string const kMap = base::JoinPath(GetPlatform().WritableDir(), "minsk-pass.mwm");

  FeaturesVectorTest fv(kMap);

  feature::DataHeader header(kMap);
  auto const codingParams = header.GetDefGeometryCodingParams();

  TBuffer buffer;

  {
    CentersTableBuilder builder;

    builder.SetGeometryCodingParamsV0ForTests(codingParams);
    fv.GetVector().ForEach([&](FeatureType & ft, uint32_t id) { builder.PutV0ForTests(id, feature::GetCenter(ft)); });

    MemWriter<TBuffer> writer(buffer);
    builder.FreezeV0ForTests(writer);
  }

  {
    MemReader reader(buffer.data(), buffer.size());
    auto table = CentersTable::LoadV0(reader, codingParams);
    TEST(table.get(), ());

    fv.GetVector().ForEach([&](FeatureType & ft, uint32_t id)
    {
      m2::PointD actual;
      TEST(table->Get(id, actual), ());

      m2::PointD expected = feature::GetCenter(ft);

      TEST_LESS_OR_EQUAL(mercator::DistanceOnEarth(actual, expected), 1.0, (id));
    });
  }
}

UNIT_CLASS_TEST(CentersTableTest, Subset)
{
  vector<pair<uint32_t, m2::PointD>> const features = {
      {1, m2::PointD(0.0, 0.0)}, {5, m2::PointD(1.0, 1.0)}, {10, m2::PointD(2.0, 2.0)}};

  TBuffer buffer;
  {
    CentersTableBuilder builder;

    builder.SetGeometryParams({{0.0, 0.0}, {2.0, 2.0}});
    for (auto const & feature : features)
      builder.Put(feature.first, feature.second);

    MemWriter<TBuffer> writer(buffer);
    builder.Freeze(writer);
  }

  {
    MemReader reader(buffer.data(), buffer.size());
    auto table = CentersTable::LoadV1(reader);
    TEST(table.get(), ());

    uint32_t i = 0;
    size_t j = 0;

    while (i < 100)
    {
      ASSERT(j == features.size() || features[j].first >= i, ("Invariant violation"));

      m2::PointD actual;
      if (j != features.size() && i == features[j].first)
      {
        TEST(table->Get(i, actual), ());
        TEST_LESS_OR_EQUAL(mercator::DistanceOnEarth(actual, features[j].second), 1.0, ());
      }
      else
      {
        TEST(!table->Get(i, actual), ());
      }

      ++i;
      while (j != features.size() && features[j].first < i)
        ++j;
    }
  }
}
}  // namespace
