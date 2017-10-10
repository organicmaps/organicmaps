#include "testing/testing.hpp"

#include "generator/generator_tests_support/test_feature.hpp"
#include "generator/generator_tests_support/test_mwm_builder.hpp"

#include "ugc/api.hpp"
#include "ugc/ugc_tests/utils.hpp"

#include "storage/country_info_getter.hpp"

#include "indexer/classificator_loader.hpp"
#include "indexer/feature.hpp"
#include "indexer/feature_decl.hpp"
#include "indexer/ftypes_matcher.hpp"
#include "indexer/index.hpp"
#include "indexer/mwm_set.hpp"

#include "coding/file_name_utils.hpp"
#include "coding/internal/file_data.hpp"

#include "platform/local_country_file_utils.hpp"
#include "platform/platform.hpp"

#include <chrono>
#include <functional>
#include <string>
#include <vector>

using namespace std;
using namespace generator::tests_support;
using namespace ugc;

namespace
{
string const kTestMwmName = "ugc storage test";

bool DeleteIndexFile()
{
  return my::DeleteFileX(my::JoinPath(GetPlatform().WritableDir(), "index.json"));
}

bool DeleteUGCFile()
{
  return my::DeleteFileX(my::JoinPath(GetPlatform().WritableDir(), "ugc.update.bin"));
}
}  // namespace

namespace ugc_tests
{
class TestCafe : public TestPOI
{
public:
  TestCafe(m2::PointD const & center) : TestPOI(center, "Cafe", "en")
  {
    SetTypes({{"amenity", "cafe"}});
  }
};

class TestRailway : public TestPOI
{
public:
  TestRailway(m2::PointD const & center) : TestPOI(center, "Railway", "en")
  {
    SetTypes({{"railway", "station"}});
  }
};

class MwmBuilder
{
public:
  using BuilderFn = function<void(TestMwmBuilder & builder)>;

  static MwmBuilder & Builder()
  {
    static MwmBuilder builder;
    return builder;
  }

  void Build(vector<TestPOI> const & poi)
  {
    BuildMwm([&poi](TestMwmBuilder & builder)
             {
               for (auto const & p : poi)
                 builder.Add(p);
             });
  }

  FeatureID FeatureIdForCafeAtPoint(m2::PointD const & mercator)
  {
    return FeatureIdForPoint(mercator, ftypes::IsFoodChecker::Instance());
  }

  FeatureID FeatureIdForRailwayAtPoint(m2::PointD const & mercator)
  {
    return FeatureIdForPoint(mercator, ftypes::IsRailwayStationChecker::Instance());
  }

  Index & GetIndex() { return m_index; }

private:
  MwmBuilder()
  {
    classificator::Load();
  }

  MwmSet::MwmId BuildMwm(BuilderFn const & fn)
  {
    if (m_testMwm.OnDisk(MapOptions::Map))
      Cleanup(m_testMwm);

    m_testMwm = platform::LocalCountryFile(GetPlatform().WritableDir(),
                                           platform::CountryFile(kTestMwmName),
                                           0 /* version */);
    {
      generator::tests_support::TestMwmBuilder builder(m_testMwm, feature::DataHeader::country);
      fn(builder);
    }

    auto result = m_index.RegisterMap(m_testMwm);
    CHECK_EQUAL(result.second, MwmSet::RegResult::Success, ());

    auto const & id = result.first;

    auto const & info = id.GetInfo();
    if (info)
      m_infoGetter.AddCountry(storage::CountryDef(kTestMwmName, info->m_limitRect));

    CHECK(id.IsAlive(), ());
    return id;
  }

  template<typename Checker>
  FeatureID FeatureIdForPoint(m2::PointD const & mercator, Checker const & checker)
  {
    m2::RectD const rect = MercatorBounds::RectByCenterXYAndSizeInMeters(mercator, 0.2 /* rect width */);
    FeatureID id;
    auto const fn = [&id, &checker](FeatureType const & featureType)
    {
      if (checker(featureType))
        id = featureType.GetID();
    };
    m_index.ForEachInRect(fn, rect, scales::GetUpperScale());
    CHECK(id.IsValid(), ());
    return id;
  }

  void Cleanup(platform::LocalCountryFile const & map)
  {
    m_index.DeregisterMap(map.GetCountryFile());
    platform::CountryIndexes::DeleteFromDisk(map);
    map.DeleteFromDisk(MapOptions::Map);
  }

  Index m_index;
  storage::CountryInfoGetterForTesting m_infoGetter;
  platform::LocalCountryFile m_testMwm;
};
}  // namespace ugc_tests

using namespace ugc_tests;

UNIT_TEST(StorageTests_Smoke)
{
  auto & builder = MwmBuilder::Builder();
  m2::PointD const point(1.0, 1.0);
  builder.Build({TestCafe(point)});
  auto const id = builder.FeatureIdForCafeAtPoint(point);
  auto const original = MakeTestUGCUpdate(Time(chrono::hours(24 * 300)));
  Storage storage(builder.GetIndex());
  storage.SetUGCUpdate(id, original);
  auto const actual = storage.GetUGCUpdate(id);
  TEST_EQUAL(original, actual, ());
  TEST(!storage.GetUGCToSend().empty(), ());
  storage.MarkAllAsSynchronized();
  TEST(storage.GetUGCToSend().empty(), ());
  TEST(DeleteIndexFile(), ());
  TEST(DeleteUGCFile(), ());
}

UNIT_TEST(StorageTests_DuplicatesAndDefragmentationSmoke)
{
  auto & builder = MwmBuilder::Builder();
  m2::PointD const point(1.0, 1.0);
  builder.Build({TestCafe(point)});
  auto const id = builder.FeatureIdForCafeAtPoint(point);
  auto const first = MakeTestUGCUpdate(Time(chrono::hours(24 * 300)));
  auto const second = MakeTestUGCUpdate(Time(chrono::hours(24 * 100)));
  auto const third = MakeTestUGCUpdate(Time(chrono::hours(24 * 300)));
  auto const last = MakeTestUGCUpdate(Time(chrono::hours(24 * 100)));
  Storage storage(builder.GetIndex());
  storage.SetUGCUpdate(id, first);
  storage.SetUGCUpdate(id, second);
  storage.SetUGCUpdate(id, third);
  storage.SetUGCUpdate(id, last);
  TEST_EQUAL(last, storage.GetUGCUpdate(id), ());
  TEST_EQUAL(storage.GetIndexesForTesting().size(), 4, ());
  TEST_EQUAL(storage.GetNumberOfDeletedForTesting(), 3, ());
  storage.Defragmentation();
  TEST_EQUAL(storage.GetIndexesForTesting().size(), 1, ());
  TEST_EQUAL(storage.GetNumberOfDeletedForTesting(), 0, ());
  TEST_EQUAL(last, storage.GetUGCUpdate(id), ());
  TEST(DeleteUGCFile(), ());
}

UNIT_TEST(StorageTests_DifferentTypes)
{
  auto & builder = MwmBuilder::Builder();
  m2::PointD const point(1.0, 1.0);
  builder.Build({TestCafe(point), TestRailway(point)});
  auto const cafeId = builder.FeatureIdForCafeAtPoint(point);
  auto const railwayId = builder.FeatureIdForRailwayAtPoint(point);
  auto const cafeUGC = MakeTestUGCUpdate(Time(chrono::hours(24 * 10)));
  auto const railwayUGC = MakeTestUGCUpdate(Time(chrono::hours(24 * 300)));
  Storage storage(builder.GetIndex());
  storage.SetUGCUpdate(cafeId, cafeUGC);
  storage.SetUGCUpdate(railwayId, railwayUGC);
  TEST_EQUAL(railwayUGC, storage.GetUGCUpdate(railwayId), ());
  TEST_EQUAL(cafeUGC, storage.GetUGCUpdate(cafeId), ());
  TEST(DeleteUGCFile(), ());
}

UNIT_TEST(StorageTest_LoadIndex)
{
  auto & builder = MwmBuilder::Builder();
  m2::PointD const cafePoint(1.0, 1.0);
  m2::PointD const railwayPoint(2.0, 2.0);
  builder.Build({TestCafe(cafePoint), TestRailway(railwayPoint)});
  auto const cafeId = builder.FeatureIdForCafeAtPoint(cafePoint);
  auto const railwayId = builder.FeatureIdForRailwayAtPoint(railwayPoint);
  auto const cafeUGC = MakeTestUGCUpdate(Time(chrono::hours(24 * 10)));
  auto const railwayUGC = MakeTestUGCUpdate(Time(chrono::hours(24 * 300)));

  {
    Storage storage(builder.GetIndex());
    storage.SetUGCUpdate(cafeId, cafeUGC);
    storage.SetUGCUpdate(railwayId, railwayUGC);
    storage.SaveIndex();
  }

  Storage storage(builder.GetIndex());
  auto const & indexArray = storage.GetIndexesForTesting();
  TEST_EQUAL(indexArray.size(), 2, ());
  for (auto const & i : indexArray)
  {
    TEST(!i.m_synchronized, ());
    TEST(!i.m_deleted, ());
  }

  storage.SetUGCUpdate(cafeId, cafeUGC);
  TEST_EQUAL(indexArray.size(), 3, ());
  TEST(DeleteIndexFile(), ());
  TEST(DeleteUGCFile(), ());
}
