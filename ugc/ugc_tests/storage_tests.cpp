#include "testing/testing.hpp"

#include "generator/generator_tests_support/test_feature.hpp"
#include "generator/generator_tests_support/test_mwm_builder.hpp"

#include "ugc/api.hpp"
#include "ugc/serdes_json.hpp"
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
#include "coding/writer.hpp"

#include "platform/local_country_file_utils.hpp"
#include "platform/platform.hpp"

#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "3party/jansson/myjansson.hpp"

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

  ~MwmBuilder()
  {
    platform::CountryIndexes::DeleteFromDisk(m_testMwm);
    m_testMwm.DeleteFromDisk(MapOptions::Map);
  }

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

class StorageTest
{
public:
  ~StorageTest()
  {
    TEST(DeleteUGCFile(), ());
  }
};
}  // namespace ugc_tests

using namespace ugc_tests;

UNIT_CLASS_TEST(StorageTest, Smoke)
{
  auto & builder = MwmBuilder::Builder();
  m2::PointD const point(1.0, 1.0);
  builder.Build({TestCafe(point)});
  auto const id = builder.FeatureIdForCafeAtPoint(point);
  auto const original = MakeTestUGCUpdate(Time(chrono::hours(24 * 300)));
  Storage storage(builder.GetIndex());
  storage.Load();
  TEST_EQUAL(storage.SetUGCUpdate(id, original), Storage::SettingResult::Success, ());
  auto const actual = storage.GetUGCUpdate(id);
  TEST_EQUAL(original, actual, ());
  TEST(!storage.GetUGCToSend().empty(), ());
  storage.MarkAllAsSynchronized();
  TEST(storage.GetUGCToSend().empty(), ());
  TEST(DeleteIndexFile(), ());
}

UNIT_CLASS_TEST(StorageTest, DuplicatesAndDefragmentationSmoke)
{
  auto & builder = MwmBuilder::Builder();
  m2::PointD const point(1.0, 1.0);
  builder.Build({TestCafe(point), TestRailway(point)});
  auto const cafeId = builder.FeatureIdForCafeAtPoint(point);
  auto const railwayId = builder.FeatureIdForRailwayAtPoint(point);
  auto const first = MakeTestUGCUpdate(Time(chrono::hours(24 * 300)));
  auto const second = MakeTestUGCUpdate(Time(chrono::hours(24 * 100)));
  auto const third = MakeTestUGCUpdate(Time(chrono::hours(24 * 300)));
  auto const last = MakeTestUGCUpdate(Time(chrono::hours(24 * 100)));
  Storage storage(builder.GetIndex());
  storage.Load();
  TEST_EQUAL(storage.SetUGCUpdate(cafeId, first), Storage::SettingResult::Success, ());
  TEST_EQUAL(storage.SetUGCUpdate(cafeId, second), Storage::SettingResult::Success, ());
  TEST_EQUAL(storage.SetUGCUpdate(cafeId, third), Storage::SettingResult::Success, ());
  TEST_EQUAL(storage.SetUGCUpdate(cafeId, last), Storage::SettingResult::Success, ());
  TEST_EQUAL(storage.SetUGCUpdate(railwayId, first), Storage::SettingResult::Success, ());
  TEST_EQUAL(last, storage.GetUGCUpdate(cafeId), ());
  TEST_EQUAL(storage.GetIndexesForTesting().size(), 5, ());
  TEST_EQUAL(storage.GetNumberOfDeletedForTesting(), 3, ());
  storage.Defragmentation();
  TEST_EQUAL(storage.GetIndexesForTesting().size(), 2, ());
  TEST_EQUAL(storage.GetNumberOfDeletedForTesting(), 0, ());
  TEST_EQUAL(last, storage.GetUGCUpdate(cafeId), ());
  TEST_EQUAL(first, storage.GetUGCUpdate(railwayId), ());
}

UNIT_CLASS_TEST(StorageTest, DifferentTypes)
{
  auto & builder = MwmBuilder::Builder();
  m2::PointD const point(1.0, 1.0);
  builder.Build({TestCafe(point), TestRailway(point)});
  auto const cafeId = builder.FeatureIdForCafeAtPoint(point);
  auto const railwayId = builder.FeatureIdForRailwayAtPoint(point);
  auto const cafeUGC = MakeTestUGCUpdate(Time(chrono::hours(24 * 10)));
  auto const railwayUGC = MakeTestUGCUpdate(Time(chrono::hours(24 * 300)));
  Storage storage(builder.GetIndex());
  storage.Load();
  TEST_EQUAL(storage.SetUGCUpdate(cafeId, cafeUGC), Storage::SettingResult::Success, ());
  TEST_EQUAL(storage.SetUGCUpdate(railwayId, railwayUGC), Storage::SettingResult::Success, ());
  TEST_EQUAL(railwayUGC, storage.GetUGCUpdate(railwayId), ());
  TEST_EQUAL(cafeUGC, storage.GetUGCUpdate(cafeId), ());
}

UNIT_CLASS_TEST(StorageTest, LoadIndex)
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
    storage.Load();
    TEST_EQUAL(storage.SetUGCUpdate(cafeId, cafeUGC), Storage::SettingResult::Success, ());
    TEST_EQUAL(storage.SetUGCUpdate(railwayId, railwayUGC), Storage::SettingResult::Success, ());
    storage.SaveIndex();
  }

  Storage storage(builder.GetIndex());
  storage.Load();
  auto const & indexArray = storage.GetIndexesForTesting();
  TEST_EQUAL(indexArray.size(), 2, ());
  for (auto const & i : indexArray)
  {
    TEST(!i.m_synchronized, ());
    TEST(!i.m_deleted, ());
  }

  TEST_EQUAL(storage.SetUGCUpdate(cafeId, cafeUGC), Storage::SettingResult::Success, ());
  TEST_EQUAL(indexArray.size(), 3, ());
  TEST(DeleteIndexFile(), ());
}

UNIT_CLASS_TEST(StorageTest, ContentTest)
{
  auto & builder = MwmBuilder::Builder();
  m2::PointD const cafePoint(1.0, 1.0);
  builder.Build({TestCafe(cafePoint)});
  auto const cafeId = builder.FeatureIdForCafeAtPoint(cafePoint);
  auto const oldUGC = MakeTestUGCUpdate(Time(chrono::hours(24 * 10)));
  auto const newUGC = MakeTestUGCUpdate(Time(chrono::hours(24 * 300)));
  Storage storage(builder.GetIndex());
  storage.Load();
  TEST_EQUAL(storage.SetUGCUpdate(cafeId, oldUGC), Storage::SettingResult::Success, ());
  TEST_EQUAL(storage.SetUGCUpdate(cafeId, newUGC), Storage::SettingResult::Success, ());
  TEST_EQUAL(storage.GetIndexesForTesting().size(), 2, ());
  auto const toSendActual = storage.GetUGCToSend();

  string data;
  {
    using Sink = MemWriter<string>;
    Sink sink(data);
    SerializerJson<Sink> ser(sink);
    ser(newUGC);
  }

  my::Json ugcNode(data);
  auto const & indexes = storage.GetIndexesForTesting();
  TEST_EQUAL(indexes.size(), 2, ());
  auto const & firstIndex = indexes.front();
  auto const & lastIndex = indexes.back();
  TEST(firstIndex.m_deleted, ());
  TEST(!lastIndex.m_deleted, ());
  TEST(!firstIndex.m_synchronized, ());
  TEST(!lastIndex.m_synchronized, ());

  auto embeddedNode = my::NewJSONObject();
  ToJSONObject(*embeddedNode.get(), "data_version", lastIndex.m_dataVersion);
  ToJSONObject(*embeddedNode.get(), "mwm_name", lastIndex.m_mwmName);
  ToJSONObject(*embeddedNode.get(), "feature_id", lastIndex.m_featureId);
  ToJSONObject(*embeddedNode.get(), "feature_type", classif().GetReadableObjectName(lastIndex.m_matchingType));
  ToJSONObject(*ugcNode.get(), "feature", *embeddedNode.release());

  auto array = my::NewJSONArray();
  json_array_append_new(array.get(), ugcNode.get_deep_copy());
  auto reviewsNode = my::NewJSONObject();
  ToJSONObject(*reviewsNode.get(), "reviews", *array.release());
  unique_ptr<char, JSONFreeDeleter> buffer(json_dumps(reviewsNode.get(), JSON_COMPACT | JSON_ENSURE_ASCII));
  string const toSendExpected(buffer.get());
  TEST_EQUAL(toSendActual, toSendExpected, ());
  storage.MarkAllAsSynchronized();
  TEST(firstIndex.m_synchronized, ());
  TEST(lastIndex.m_synchronized, ());
  TEST(DeleteIndexFile(), ());
}

UNIT_CLASS_TEST(StorageTest, InvalidUGC)
{
  auto & builder = MwmBuilder::Builder();
  m2::PointD const cafePoint(1.0, 1.0);
  builder.Build({TestCafe(cafePoint)});
  auto const cafeId = builder.FeatureIdForCafeAtPoint(cafePoint);
  Storage storage(builder.GetIndex());
  storage.Load();

  UGCUpdate first;
  TEST_EQUAL(storage.SetUGCUpdate(cafeId, first), Storage::SettingResult::InvalidUGC, ());
  first.m_time = chrono::system_clock::now();
  TEST_EQUAL(storage.SetUGCUpdate(cafeId, first), Storage::SettingResult::InvalidUGC, ());
  TEST(storage.GetIndexesForTesting().empty(), ());
  TEST(storage.GetUGCToSend().empty(), ());
  first.m_text = KeyboardText("a", 1, {2});
  TEST_EQUAL(storage.SetUGCUpdate(cafeId, first), Storage::SettingResult::Success, ());
  UGCUpdate second;
  second.m_time = chrono::system_clock::now();
  second.m_ratings.emplace_back("a", 0);
  TEST_EQUAL(storage.SetUGCUpdate(cafeId, second), Storage::SettingResult::InvalidUGC, ());
  second.m_ratings.emplace_back("b", 1);
  TEST_EQUAL(storage.SetUGCUpdate(cafeId, second), Storage::SettingResult::Success, ());
}

UNIT_CLASS_TEST(StorageTest, DifferentUGCVersions)
{
  auto & builder = MwmBuilder::Builder();
  m2::PointD const firstPoint(1.0, 1.0);
  m2::PointD const secondPoint(2.0, 2.0);
  builder.Build({TestCafe(firstPoint), TestCafe(secondPoint)});

  Storage storage(builder.GetIndex());
  storage.Load();

  auto const firstId = builder.FeatureIdForCafeAtPoint(firstPoint);
  auto const oldUGC = MakeTestUGCUpdateV0(Time(chrono::hours(24 * 10)));
  TEST_EQUAL(storage.SetUGCUpdateForTesting(firstId, oldUGC), Storage::SettingResult::Success, ());
  auto const secondId = builder.FeatureIdForCafeAtPoint(secondPoint);
  auto const newUGC = MakeTestUGCUpdate(Time(chrono::hours(24 * 5)));
  TEST_EQUAL(storage.SetUGCUpdate(secondId, newUGC), Storage::SettingResult::Success, ());

  TEST_EQUAL(newUGC, storage.GetUGCUpdate(secondId), ());
  UGCUpdate fromOld;
  fromOld.BuildFrom(oldUGC);
  TEST_EQUAL(fromOld, storage.GetUGCUpdate(firstId), ());
}

UNIT_CLASS_TEST(StorageTest, NumberOfUnsynchronized)
{
  auto & builder = MwmBuilder::Builder();
  m2::PointD const cafePoint(1.0, 1.0);
  builder.Build({TestCafe(cafePoint)});
  auto const cafeId = builder.FeatureIdForCafeAtPoint(cafePoint);
  auto const cafeUGC = MakeTestUGCUpdate(Time(chrono::hours(24 * 10)));

  {
    Storage storage(builder.GetIndex());
    storage.Load();
    TEST_EQUAL(storage.SetUGCUpdate(cafeId, cafeUGC), Storage::SettingResult::Success, ());
    storage.SaveIndex();
  }

  Storage storage(builder.GetIndex());
  storage.Load();
  TEST_EQUAL(storage.GetNumberOfUnsynchronized(), 1, ());

  TEST_EQUAL(storage.SetUGCUpdate(cafeId, cafeUGC), Storage::SettingResult::Success, ());
  TEST_EQUAL(storage.GetNumberOfUnsynchronized(), 1, ());
  TEST_EQUAL(storage.GetNumberOfDeletedForTesting(), 1, ());

  storage.MarkAllAsSynchronized();
  TEST_EQUAL(storage.GetNumberOfUnsynchronized(), 0, ());

  TEST(DeleteIndexFile(), ());
}
