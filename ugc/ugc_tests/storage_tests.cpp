#include "testing/testing.hpp"

#include "generator/generator_tests_support/test_feature.hpp"
#include "generator/generator_tests_support/test_mwm_builder.hpp"

#include "ugc/api.hpp"
#include "ugc/serdes_json.hpp"
#include "ugc/ugc_tests/utils.hpp"

#include "storage/country_info_getter.hpp"

#include "indexer/classificator.hpp"
#include "indexer/classificator_loader.hpp"
#include "indexer/data_source.hpp"
#include "indexer/feature.hpp"
#include "indexer/feature_decl.hpp"
#include "indexer/ftypes_matcher.hpp"
#include "indexer/mwm_set.hpp"

#include "coding/file_name_utils.hpp"
#include "coding/internal/file_data.hpp"
#include "coding/writer.hpp"
#include "coding/zlib.hpp"

#include "platform/local_country_file_utils.hpp"
#include "platform/platform.hpp"
#include "platform/platform_tests_support/scoped_file.hpp"

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
int64_t constexpr kMinVersionForMigration = 171020;

string const kTestMwmName = "ugc storage test";

bool DeleteIndexFile(ugc::IndexVersion v = ugc::IndexVersion::Latest)
{
  if (v == ugc::IndexVersion::Latest)
    return base::DeleteFileX(base::JoinPath(GetPlatform().WritableDir(), "index.json"));

  string version;
  switch (v)
  {
  case ugc::IndexVersion::V0:
    version = "v0";
    break;
  case ugc::IndexVersion::V1:
    version = "v1";
    break;
  }

  return base::DeleteFileX(base::JoinPath(GetPlatform().WritableDir(), "index.json." + version));
}

bool DeleteUGCFile(ugc::IndexVersion v = ugc::IndexVersion::Latest)
{
  if (v == ugc::IndexVersion::Latest)
    return base::DeleteFileX(base::JoinPath(GetPlatform().WritableDir(), "ugc.update.bin"));


  string version;
  switch (v)
  {
    case ugc::IndexVersion::V0:
      version = "v0";
      break;
    case ugc::IndexVersion::V1:
      version = "v1";
      break;
  }

  return base::DeleteFileX(base::JoinPath(GetPlatform().WritableDir(), "ugc.update.bin." + version));
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
    return FeatureIdForPoint(mercator, ftypes::IsEatChecker::Instance());
  }

  FeatureID FeatureIdForRailwayAtPoint(m2::PointD const & mercator)
  {
    return FeatureIdForPoint(mercator, ftypes::IsRailwayStationChecker::Instance());
  }

  DataSource & GetDataSource() { return m_dataSource; }

  ~MwmBuilder()
  {
    platform::CountryIndexes::DeleteFromDisk(m_testMwm);
    m_testMwm.DeleteFromDisk(MapOptions::Map);
  }

private:
  MwmBuilder() { classificator::Load(); }

  MwmSet::MwmId BuildMwm(BuilderFn const & fn)
  {
    if (m_testMwm.OnDisk(MapOptions::Map))
      Cleanup(m_testMwm);

    m_testMwm = platform::LocalCountryFile(GetPlatform().WritableDir(),
                                           platform::CountryFile(kTestMwmName),
                                           kMinVersionForMigration);
    {
      generator::tests_support::TestMwmBuilder builder(m_testMwm, feature::DataHeader::country);
      fn(builder);
    }

    auto result = m_dataSource.RegisterMap(m_testMwm);
    CHECK_EQUAL(result.second, MwmSet::RegResult::Success, ());

    auto const & id = result.first;

    auto const & info = id.GetInfo();
    if (info)
      m_infoGetter.AddCountry(storage::CountryDef(kTestMwmName, info->m_bordersRect));

    CHECK(id.IsAlive(), ());
    return id;
  }

  template<typename Checker>
  FeatureID FeatureIdForPoint(m2::PointD const & mercator, Checker const & checker)
  {
    m2::RectD const rect = MercatorBounds::RectByCenterXYAndSizeInMeters(mercator, 0.2 /* rect width */);
    FeatureID id;
    auto const fn = [&id, &checker](FeatureType & featureType) {
      if (checker(featureType))
        id = featureType.GetID();
    };
    m_dataSource.ForEachInRect(fn, rect, scales::GetUpperScale());
    CHECK(id.IsValid(), ());
    return id;
  }

  void Cleanup(platform::LocalCountryFile const & map)
  {
    m_dataSource.DeregisterMap(map.GetCountryFile());
    platform::CountryIndexes::DeleteFromDisk(map);
    map.DeleteFromDisk(MapOptions::Map);
  }

  FrozenDataSource m_dataSource;
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
  Storage storage(builder.GetDataSource());
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
  Storage storage(builder.GetDataSource());
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
  Storage storage(builder.GetDataSource());
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
    Storage storage(builder.GetDataSource());
    storage.Load();
    TEST_EQUAL(storage.SetUGCUpdate(cafeId, cafeUGC), Storage::SettingResult::Success, ());
    TEST_EQUAL(storage.SetUGCUpdate(railwayId, railwayUGC), Storage::SettingResult::Success, ());
    storage.SaveIndex();
  }

  Storage storage(builder.GetDataSource());
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
  Storage storage(builder.GetDataSource());
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

  base::Json ugcNode(data);
  auto const & indexes = storage.GetIndexesForTesting();
  TEST_EQUAL(indexes.size(), 2, ());
  auto const & firstIndex = indexes.front();
  auto const & lastIndex = indexes.back();
  TEST(firstIndex.m_deleted, ());
  TEST(!lastIndex.m_deleted, ());
  TEST(!firstIndex.m_synchronized, ());
  TEST(!lastIndex.m_synchronized, ());

  auto embeddedNode = base::NewJSONObject();
  ToJSONObject(*embeddedNode.get(), "data_version", lastIndex.m_dataVersion);
  ToJSONObject(*embeddedNode.get(), "mwm_name", lastIndex.m_mwmName);
  ToJSONObject(*embeddedNode.get(), "feature_id", lastIndex.m_featureId);
  auto const & c = classif();
  ToJSONObject(*embeddedNode.get(), "feature_type", c.GetReadableObjectName(c.GetTypeForIndex(lastIndex.m_matchingType)));
  ToJSONObject(*ugcNode.get(), "feature", *embeddedNode.release());

  auto array = base::NewJSONArray();
  json_array_append_new(array.get(), ugcNode.get_deep_copy());
  auto reviewsNode = base::NewJSONObject();
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
  Storage storage(builder.GetDataSource());
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

  Storage storage(builder.GetDataSource());
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
    Storage storage(builder.GetDataSource());
    storage.Load();
    TEST_EQUAL(storage.SetUGCUpdate(cafeId, cafeUGC), Storage::SettingResult::Success, ());
    storage.SaveIndex();
  }

  Storage storage(builder.GetDataSource());
  storage.Load();
  TEST_EQUAL(storage.GetNumberOfUnsynchronized(), 1, ());

  TEST_EQUAL(storage.SetUGCUpdate(cafeId, cafeUGC), Storage::SettingResult::Success, ());
  TEST_EQUAL(storage.GetNumberOfUnsynchronized(), 1, ());
  TEST_EQUAL(storage.GetNumberOfDeletedForTesting(), 1, ());

  storage.MarkAllAsSynchronized();
  TEST_EQUAL(storage.GetNumberOfUnsynchronized(), 0, ());

  TEST(DeleteIndexFile(), ());
}

UNIT_CLASS_TEST(StorageTest, GetNumberOfUnsentSeparately)
{
  TEST_EQUAL(lightweight::impl::GetNumberOfUnsentUGC(), 0, ());
  auto & builder = MwmBuilder::Builder();
  m2::PointD const cafePoint(1.0, 1.0);
  builder.Build({TestCafe(cafePoint)});
  auto const cafeId = builder.FeatureIdForCafeAtPoint(cafePoint);
  auto const cafeUGC = MakeTestUGCUpdate(Time(chrono::hours(24 * 10)));

  {
    Storage storage(builder.GetDataSource());
    storage.Load();
    TEST_EQUAL(storage.SetUGCUpdate(cafeId, cafeUGC), Storage::SettingResult::Success, ());
    storage.SaveIndex();
    TEST_EQUAL(storage.GetNumberOfUnsynchronized(), 1, ());
  }

  TEST_EQUAL(lightweight::impl::GetNumberOfUnsentUGC(), 1, ());

  {
    Storage storage(builder.GetDataSource());
    storage.Load();
    storage.MarkAllAsSynchronized();
    TEST_EQUAL(storage.GetNumberOfUnsynchronized(), 0, ());
    storage.SaveIndex();
  }

  TEST_EQUAL(lightweight::impl::GetNumberOfUnsentUGC(), 0, ());
  TEST(DeleteIndexFile(), ());
}

UNIT_TEST(UGC_IndexMigrationFromV0ToV1Smoke)
{
  platform::tests_support::ScopedFile dummyUgcUpdate("ugc.update.bin", "some test content");
  LOG(LINFO, ("Created dummy ugc update", dummyUgcUpdate));

  auto & p = GetPlatform();
  auto const version = "v0";
  auto const indexFileName = "index.json";
  auto const folder = base::JoinPath(p.WritableDir(), "ugc_migration_supported_files", "test_index", version);
  auto const indexFilePath = base::JoinPath(folder, indexFileName);
  {
    using Inflate = coding::ZLib::Inflate;
    auto const r = p.GetReader(base::JoinPath(folder, "index.gz"));
    string data;
    r->ReadAsString(data);
    Inflate inflate(Inflate::Format::GZip);
    string index;
    inflate(data.data(), data.size(), back_inserter(index));
    FileWriter w(indexFilePath);
    w.Write(index.data(), index.size());
  }

  auto & builder = MwmBuilder::Builder();
  builder.Build({});
  auto const v0IndexFilePath = indexFilePath + "." + version;

  {
    Storage s(builder.GetDataSource());
    s.LoadForTesting(indexFilePath);
    uint64_t migratedIndexFileSize = 0;
    uint64_t v0IndexFileSize = 0;
    TEST(base::GetFileSize(indexFilePath, migratedIndexFileSize), ());
    TEST(base::GetFileSize(v0IndexFilePath, v0IndexFileSize), ());
    TEST_GREATER(migratedIndexFileSize, 0, ());
    TEST_GREATER(v0IndexFileSize, 0, ());
    auto const & indexes = s.GetIndexesForTesting();
    for (auto const & i : indexes)
    {
      TEST_EQUAL(static_cast<uint8_t>(i.m_version), static_cast<uint8_t>(IndexVersion::Latest), ());
      TEST(!i.m_synchronized, ());
    }

    TEST(s.SaveIndex(indexFilePath), ());
  }

  {
    Storage s(builder.GetDataSource());
    s.LoadForTesting(indexFilePath);
    auto const & indexes = s.GetIndexesForTesting();
    TEST_NOT_EQUAL(indexes.size(), 0, ());
    for (auto const & i : indexes)
    {
      TEST_EQUAL(static_cast<uint8_t>(i.m_version), static_cast<uint8_t>(IndexVersion::Latest), ());
      TEST(!i.m_synchronized, ());
    }
  }

  base::DeleteFileX(indexFilePath);
  base::DeleteFileX(v0IndexFilePath);
}

UNIT_TEST(UGC_NoReviews)
{
  auto & builder = MwmBuilder::Builder();
  Storage s(builder.GetDataSource());
  s.Load();
  s.SaveIndex();
  // When we didn't write any reviews there should be no index file and no ugc file.
  TEST(!DeleteIndexFile(), ());
  TEST(!DeleteUGCFile(), ());
}

UNIT_TEST(UGC_TooOldDataVersionsForMigration)
{
  auto & builder = MwmBuilder::Builder();
  m2::PointD const cafePoint(1.0, 1.0);
  m2::PointD const railwayPoint(2.0, 2.0);
  builder.Build({TestCafe(cafePoint), TestRailway(railwayPoint)});
  auto const cafeId = builder.FeatureIdForCafeAtPoint(cafePoint);
  auto const cafeUGC = MakeTestUGCUpdate(Time(chrono::hours(24 * 10)));

  auto const railwayId = builder.FeatureIdForRailwayAtPoint(railwayPoint);
  auto const railwayUGC = MakeTestUGCUpdate(Time(chrono::hours(48 * 10)));

  {
    Storage s(builder.GetDataSource());
    s.Load();
    s.SetUGCUpdate(cafeId, cafeUGC);
    s.SetUGCUpdate(railwayId, railwayUGC);
    auto & indexes = s.GetIndexesForTesting();
    TEST_EQUAL(indexes.size(), 2, ());

    auto & cafeIndex = indexes.front();
    cafeIndex.m_dataVersion = kMinVersionForMigration - 1;
    cafeIndex.m_version = IndexVersion::V0;

    auto & railwayIndex = indexes.back();
    railwayIndex.m_dataVersion = kMinVersionForMigration;
    railwayIndex.m_version = IndexVersion::V0;
    railwayIndex.m_synchronized = true;

    s.SaveIndex();
  }

  {
    Storage s(builder.GetDataSource());
    s.Load();
    // Migration should pass successfully even there are indexes with a data version lower than minimal.
    // Such indexes should be marked as deleted and should be removed after the defragmentation process.
    auto const & indexes = s.GetIndexesForTesting();
    TEST_EQUAL(indexes.size(), 1, ());

    auto const & railwayIndex = indexes.back();
    TEST_EQUAL(static_cast<uint8_t>(railwayIndex.m_version), static_cast<uint8_t>(IndexVersion::Latest), ());
    TEST(!railwayIndex.m_synchronized, ());
    TEST_EQUAL(railwayIndex.m_dataVersion, kMinVersionForMigration, ());
    s.SaveIndex();
  }

  {
    Storage s(builder.GetDataSource());
    s.Load();
    auto const & indexes = s.GetIndexesForTesting();
    TEST_EQUAL(indexes.size(), 1, ());
    auto const & railwayIndex = indexes.back();
    TEST_EQUAL(static_cast<uint8_t>(railwayIndex.m_version), static_cast<uint8_t>(IndexVersion::Latest), ());
    TEST(!railwayIndex.m_synchronized, ());
    TEST_EQUAL(railwayIndex.m_dataVersion, kMinVersionForMigration, ());
  }

  TEST(DeleteIndexFile(), ());
  TEST(DeleteIndexFile(IndexVersion::V0), ());
  TEST(DeleteUGCFile(), ());
  TEST(DeleteUGCFile(IndexVersion::V0), ());
}

UNIT_CLASS_TEST(StorageTest, UGC_HasUGCForPlace)
{
  auto & builder = MwmBuilder::Builder();
  m2::PointD const point(1.0, 1.0);
  builder.Build({TestCafe(point)});
  auto const id = builder.FeatureIdForCafeAtPoint(point);
  auto const original = MakeTestUGCUpdate(Time(chrono::hours(24 * 300)));
  Storage storage(builder.GetDataSource());
  storage.Load();
  TEST_EQUAL(storage.SetUGCUpdate(id, original), Storage::SettingResult::Success, ());
  auto const actual = storage.GetUGCUpdate(id);
  TEST_EQUAL(original, actual, ());

  auto const & c = classif();
  auto const cafeType = c.GetTypeByReadableObjectName("amenity-cafe");
  TEST(storage.HasUGCForPlace(cafeType, point), ());
}
