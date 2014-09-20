#include "../../testing/testing.hpp"
#include "../osrm_router.hpp"
#include "../../indexer/mercator.hpp"

#include "../../defines.hpp"


using namespace routing;

namespace
{

typedef vector<OsrmFtSegMappingBuilder::FtSegVectorT> InputDataT;
typedef vector< vector<OsrmNodeIdT> > NodeIdDataT;
typedef vector< pair<size_t, size_t> > RangeDataT;


void TestNodeId(OsrmFtSegMapping const & mapping, NodeIdDataT const & test)
{
  for (OsrmNodeIdT nodeId = 0; nodeId < test.size(); ++nodeId)
  {
    for (auto idx : test[nodeId])
      TEST_EQUAL(nodeId, mapping.GetNodeId(idx), ());
  }
}

void TestSegmentRange(OsrmFtSegMapping const & mapping, RangeDataT const & test)
{
  for (OsrmNodeIdT nodeId = 0; nodeId < test.size(); ++nodeId)
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
  OsrmFtSegMappingBuilder builder;
  for (OsrmNodeIdT nodeId = 0; nodeId < data.size(); ++nodeId)
    builder.Append(nodeId, data[nodeId]);

  TestNodeId(builder, nodeIds);
  TestSegmentRange(builder, ranges);

  string const fName = "test1.tmp";
  {
    FilesContainerW w(fName);
    builder.Save(w);
  }

  {
    FilesMappingContainer cont(fName);
    OsrmFtSegMapping mapping;
    mapping.Load(cont);

    TestNodeId(mapping, nodeIds);
    TestSegmentRange(mapping, ranges);

    for (size_t i = 0; i < mapping.GetSegmentsCount(); ++i)
    {
      OsrmNodeIdT const node = mapping.GetNodeId(i);
      size_t count = 0;
      mapping.ForEachFtSeg(node, [&] (OsrmFtSegMapping::FtSeg const & s)
      {
        TEST_EQUAL(s, data[node][count++], ());
      });
      TEST_EQUAL(count, data[node].size(), ());
    }
  }

  FileWriter::DeleteFileX(fName);
}

typedef OsrmFtSegMapping::FtSeg SegT;

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
