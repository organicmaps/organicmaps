#include "testing/testing.hpp"

#include "coding/reader.hpp"
#include "coding/writer.hpp"

#include "routing_common/transit_serdes.hpp"
#include "routing_common/transit_types.hpp"

#include <cstdint>
#include <vector>

using namespace routing;
using namespace routing::transit;
using namespace std;

namespace routing
{
namespace transit
{
template<class S, class D, class Obj>
void TestCommonSerialization(Obj const & obj)
{
  vector<uint8_t> buffer;
  MemWriter<vector<uint8_t>> writer(buffer);

  S serializer(writer);
  obj.Visit(serializer);

  MemReader reader(buffer.data(), buffer.size());
  ReaderSource<MemReader> src(reader);
  Obj deserializedObj;
  D deserializer(src);
  deserializedObj.Visit(deserializer);

  TEST(obj.IsEqualForTesting(deserializedObj), (obj, deserializedObj));
}

void TestSerialization(TransitHeader const & header)
{
  TestCommonSerialization<FixSizeNumberSerializer<MemWriter<vector<uint8_t>>>,
                          FixSizeNumberDeserializer<ReaderSource<MemReader>>>(header);
}

template<class Obj>
void TestSerialization(Obj const & obj)
{
  TestCommonSerialization<Serializer<MemWriter<vector<uint8_t>>>,
                          Deserializer<ReaderSource<MemReader>>>(obj);
}

UNIT_TEST(Transit_HeaderRewriting)
{
  TransitHeader const bigHeader(1 /* version */, 1000 /* gatesOffset */, 200000 /* edgesOffset */,
                                300000 /* transfersOffset */, 400000 /* linesOffset */,
                                5000000 /* shapesOffset */, 6000000 /* networksOffset */,
                                700000000 /* endOffset */);

  TransitHeader header;
  vector<uint8_t> buffer;
  MemWriter<vector<uint8_t>> writer(buffer);

  // Writing.
  auto const startOffset = writer.Pos();
  FixSizeNumberSerializer<MemWriter<vector<uint8_t>>> serializer(writer);
  header.Visit(serializer);
  auto const endOffset = writer.Pos();

  // Rewriting.
  header = bigHeader;

  writer.Seek(startOffset);
  header.Visit(serializer);
  writer.Seek(endOffset);

  // Reading.
  MemReader reader(buffer.data(), buffer.size());
  ReaderSource<MemReader> src(reader);
  TransitHeader deserializedHeader;
  FixSizeNumberDeserializer<ReaderSource<MemReader>> deserializer(src);
  deserializedHeader.Visit(deserializer);

  TEST(deserializedHeader.IsEqualForTesting(bigHeader), (deserializedHeader, bigHeader));
}
}  // namespace transit
}  // namespace routing

namespace
{
UNIT_TEST(Transit_HeaderSerialization)
{
  {
    TransitHeader header;
    TestSerialization(header);
  }
  {
    TransitHeader header(1 /* version */, 1000 /* gatesOffset */, 2000 /* edgesOffset */,
                         3000 /* transfersOffset */, 4000 /* linesOffset */, 5000 /* shapesOffset */,
                         6000 /* networksOffset */, 7000 /* endOffset */);
    TestSerialization(header);
  }
}

UNIT_TEST(Transit_TitleAnchorSerialization)
{
  {
    TitleAnchor anchor(17 /* min zoom */, 0 /* anchor */);
    TestSerialization(anchor);
  }
  {
    TitleAnchor anchor(10 /* min zoom */, 2 /* anchor */);
    TestSerialization(anchor);
  }
  {
    TitleAnchor anchor(18 /* min zoom */, 7 /* anchor */);
    TestSerialization(anchor);
  }
}

UNIT_TEST(Transit_StopSerialization)
{
  {
    Stop stop;
    TestSerialization(stop);
  }
  {
    Stop stop(1234 /* id */, kInvalidOsmId, 5678 /* feature id */, 7 /* transfer id */,
              {7, 8, 9, 10} /* line id */, {55.0, 37.0} /* point */, {} /* anchors */);
    TestSerialization(stop);
  }
}

UNIT_TEST(Transit_SingleMwmSegmentSerialization)
{
  {
    SingleMwmSegment s(12344 /* feature id */, 0 /* segmentIdx */, true /* forward */);
    TestSerialization(s);
  }
  {
    SingleMwmSegment s(12544 /* feature id */, 5 /* segmentIdx */, false /* forward */);
    TestSerialization(s);
  }
}

UNIT_TEST(Transit_GateSerialization)
{
  Gate gate(kInvalidOsmId, 12345 /* feature id */, true /* entrance */, false /* exit */,
            117.8 /* weight */, {1, 2, 3} /* stop ids */, {30.0, 50.0} /* point */);
  TestSerialization(gate);
}

UNIT_TEST(Transit_EdgeSerialization)
{
  Edge edge(1 /* start stop id */, 2 /* finish stop id */, 123.4 /* weight */, 11 /* line id */,
            false /* transfer */, {ShapeId(1, 2), ShapeId(3, 4), ShapeId(5, 6)} /* shape ids */);
  TestSerialization(edge);
}

UNIT_TEST(Transit_TransferSerialization)
{
  Transfer transfer(1 /* id */, {40.0, 35.0} /* point */, {1, 2, 3} /* stop ids */,
                    {TitleAnchor(16, 0 /* anchor */)});
  TestSerialization(transfer);
}

UNIT_TEST(Transit_LineSerialization)
{
  {
    Line line(1 /* line id */, "2" /* number */, "Линия" /* title */,
              "subway" /* type */, "red" /* color */, 3 /* network id */, {} /* stop ids */);
    TestSerialization(line);
  }
  {
    Line line(10 /* line id */, "11" /* number */, "Линия" /* title */,
              "subway" /* type */, "green" /* color */, 12 /* network id */,
              {{13, 14, 15}} /* stop ids */);
    TestSerialization(line);
  }
  {
    Line line(100 /* line id */, "101" /* number */, "Линия" /* title */,
              "subway" /* type */, "blue" /* color */, 103 /* network id */,
              {{1, 2, 3}, {7, 8, 9}} /* stop ids */);
    TestSerialization(line);
  }
}

UNIT_TEST(Transit_ShapeSerialization)
{
  {
    Shape shape(ShapeId(10, 20), {} /* polyline */);
    TestSerialization(shape);
  }
  {
    Shape shape(ShapeId(11, 21),
                {m2::PointD(20.0, 20.0), m2::PointD(21.0, 21.0), m2::PointD(22.0, 22.0)} /* polyline */);
    TestSerialization(shape);
  }
}

UNIT_TEST(Transit_NetworkSerialization)
{
  Network network(0 /* network id */, "Title" /* title */);
  TestSerialization(network);
}
}  // namespace
