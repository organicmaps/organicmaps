#include "testing/testing.hpp"

#include "routing/car_router.hpp"

#include "indexer/features_offsets_table.hpp"
#include "geometry/mercator.hpp"

#include "platform/country_file.hpp"
#include "platform/local_country_file.hpp"
#include "platform/local_country_file_utils.hpp"
#include "platform/platform.hpp"

#include "platform/platform_tests_support/scoped_mwm.hpp"

#include "coding/file_writer.hpp"

#include "defines.hpp"

#include "base/scope_guard.hpp"

#include "std/bind.hpp"
#include "std/unique_ptr.hpp"
#include "std/vector.hpp"

using namespace routing;
using platform::CountryIndexes;

namespace
{

typedef vector<OsrmFtSegMappingBuilder::FtSegVectorT> InputDataT;
typedef vector< vector<TOsrmNodeId> > NodeIdDataT;
typedef vector< pair<size_t, size_t> > RangeDataT;
typedef OsrmMappingTypes::FtSeg SegT;

void TestNodeId(OsrmFtSegMapping const & mapping, NodeIdDataT const & test)
{
  for (TOsrmNodeId nodeId = 0; nodeId < test.size(); ++nodeId)
  {
    for (auto idx : test[nodeId])
      TEST_EQUAL(nodeId, mapping.GetNodeId(idx), ());
  }
}

void TestSegmentRange(OsrmFtSegMapping const & mapping, RangeDataT const & test)
{
  for (TOsrmNodeId nodeId = 0; nodeId < test.size(); ++nodeId)
  {
    // Input test range is { start, count } but we should pass [start, end).
    auto const & r = test[nodeId];
    TEST_EQUAL(mapping.GetSegmentsRange(nodeId), make_pair(r.first, r.first + r.second), ());
  }
}

void TestMapping(InputDataT const & data,
                 NodeIdDataT const & nodeIds,
                 RangeDataT const & ranges)
{
  platform::CountryFile country("TestCountry");
  platform::LocalCountryFile localFile(GetPlatform().WritableDir(), country, 0 /* version */);
  localFile.SyncWithDisk();
  platform::tests_support::ScopedMwm mapMwm(
      platform::GetFileName(localFile.GetCountryFile().GetName(), MapOptions::Map,
                            version::FOR_TESTING_TWO_COMPONENT_MWM1));
  static string const ftSegsPath = GetPlatform().WritablePathForFile("test1.tmp");

  platform::CountryIndexes::PreparePlaceOnDisk(localFile);
  string const & featuresOffsetsTablePath =
    CountryIndexes::GetPath(localFile, CountryIndexes::Index::Offsets);
  MY_SCOPE_GUARD(ftSegsFileDeleter, bind(FileWriter::DeleteFileX, ftSegsPath));
  MY_SCOPE_GUARD(featuresOffsetsTableFileDeleter,
                 bind(FileWriter::DeleteFileX, featuresOffsetsTablePath));
  MY_SCOPE_GUARD(indexesDeleter, bind(&CountryIndexes::DeleteFromDisk, localFile));

  {
    // Prepare fake features offsets table for input data, because
    // OsrmFtSegMapping::Load() loads routing index and creates
    // additional helper indexes and some of them require
    // FeatureOffsetsTable existence.
    //
    // As instantiation of FeatureOffsetsTable requires complete MWM
    // file with features or at least already searialized
    // FeatureOffsetsTable, the purpose of this code is to prepare a
    // file with serialized FeatureOffsetsTable and feed it to
    // OsrmFtSegMapping.
    feature::FeaturesOffsetsTable::Builder tableBuilder;
    for (auto const & segVector : data)
    {
      for (auto const & seg : segVector)
        tableBuilder.PushOffset(seg.m_fid);
    }
    unique_ptr<feature::FeaturesOffsetsTable> table =
        feature::FeaturesOffsetsTable::Build(tableBuilder);
    table->Save(featuresOffsetsTablePath);
  }

  OsrmFtSegMappingBuilder builder;
  for (TOsrmNodeId nodeId = 0; nodeId < data.size(); ++nodeId)
    builder.Append(nodeId, data[nodeId]);

  TestNodeId(builder, nodeIds);
  TestSegmentRange(builder, ranges);

  {
    FilesContainerW w(ftSegsPath);
    builder.Save(w);
  }

  {
    FilesMappingContainer cont(ftSegsPath);
    OsrmFtSegMapping mapping;
    mapping.Load(cont, localFile);
    mapping.Map(cont);

    TestNodeId(mapping, nodeIds);
    TestSegmentRange(mapping, ranges);

    for (size_t i = 0; i < mapping.GetSegmentsCount(); ++i)
    {
      TOsrmNodeId const node = mapping.GetNodeId(i);
      size_t count = 0;
      mapping.ForEachFtSeg(node, [&] (OsrmMappingTypes::FtSeg const & s)
      {
        TEST_EQUAL(s, data[node][count++], ());
      });
      TEST_EQUAL(count, data[node].size(), ());
    }
  }
}

bool TestFtSeg(SegT const & s)
{
  return (SegT(s.Store()) == s);
}

}

UNIT_TEST(FtSeg_Smoke)
{
  SegT arr[] = {
    { 5, 1, 2 },
    { 666, 0, 17 },
  };

  for (size_t i = 0; i < ARRAY_SIZE(arr); ++i)
    TEST(TestFtSeg(arr[i]), (arr[i].Store()));
}

UNIT_TEST(OsrmFtSegMappingBuilder_Smoke)
{
  {
    InputDataT data =
    {
      { {0, 0, 1} },
      { {1, 0, 1} },
      { {2, 0, 1}, {3, 0, 1} },
      { {4, 0, 1} },
      { {5, 0, 1}, {6, 0, 1}, {7, 0, 1} },
      { {8, 0, 1}, {9, 0, 1}, {10, 0, 1}, {11, 0, 1} },
      { {12, 0, 1} }
    };

    NodeIdDataT nodeIds =
    {
      { 0 },
      { 1 },
      { 2, 3 },
      { 4 },
      { 5, 6, 7 },
      { 8, 9, 10, 11 },
      { 12 }
    };

    RangeDataT ranges =
    {
      { 0, 1 },
      { 1, 1 },
      { 2, 2 },
      { 4, 1 },
      { 5, 3 },
      { 8, 4 },
      { 12, 1 },
    };

    TestMapping(data, nodeIds, ranges);
  }


  {
    InputDataT data =
    {
      { {0, 0, 1} },
      { {1, 0, 1} },
      { {2, 0, 1} },
      { {3, 0, 1} },
      { {4, 0, 1} },
      { {5, 0, 1}, {6, 0, 1} },
      { {7, 0, 1} },
      { {8, 0, 1}, {9, 0, 1}, {10, 0, 1} },
      { {11, 0, 1}, {12, 0, 1}, {13, 0, 1} }
    };

    NodeIdDataT nodeIds =
    {
      { 0 },
      { 1 },
      { 2 },
      { 3 },
      { 4 },
      { 5, 6 },
      { 7 },
      { 8, 9, 10 },
      { 11, 12, 13 }
    };

    RangeDataT ranges =
    {
      { 0, 1 },
      { 1, 1 },
      { 2, 1 },
      { 3, 1 },
      { 4, 1 },
      { 5, 2 },
      { 7, 1 },
      { 8, 3 },
      { 11, 3 },
    };

    TestMapping(data, nodeIds, ranges);
  }


  {
    InputDataT data =
    {
      { {0, 0, 1}, {1, 2, 3} },
      { },
      { {3, 6, 7} },
      { {4, 8, 9}, {5, 10, 11} },
    };

    NodeIdDataT nodeIds =
    {
      { 0, 1 },
      { },
      { 3 },
      { 4, 5 },
    };

    RangeDataT ranges =
    {
      { 0, 2 },
      { 2, 1 },
      { 3, 1 },
      { 4, 2 },
    };

    TestMapping(data, nodeIds, ranges);
  }
}
