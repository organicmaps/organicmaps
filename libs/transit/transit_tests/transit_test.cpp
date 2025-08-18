#include "testing/testing.hpp"

#include "transit/transit_serdes.hpp"
#include "transit/transit_types.hpp"
#include "transit/transit_version.hpp"

#include "coding/reader.hpp"
#include "coding/writer.hpp"

#include <algorithm>
#include <cstdint>
#include <vector>

using namespace routing;
using namespace routing::transit;
using namespace std;

namespace routing
{
namespace transit
{
auto constexpr kTransitHeaderVersion = static_cast<uint16_t>(::transit::TransitVersion::OnlySubway);

template <class S, class D, class Obj>
void TestCommonSerialization(Obj const & obj)
{
  vector<uint8_t> buffer;
  MemWriter<vector<uint8_t>> writer(buffer);

  S serializer(writer);
  serializer(obj);

  MemReader reader(buffer.data(), buffer.size());
  ReaderSource<MemReader> src(reader);
  Obj deserializedObj;
  D deserializer(src);
  deserializer(deserializedObj);

  TEST(obj.IsEqualForTesting(deserializedObj), (obj, deserializedObj));
}

void TestSerialization(TransitHeader const & header)
{
  TestCommonSerialization<FixedSizeSerializer<MemWriter<vector<uint8_t>>>,
                          FixedSizeDeserializer<ReaderSource<MemReader>>>(header);
}

template <class Obj>
void TestSerialization(Obj const & obj)
{
  TestCommonSerialization<Serializer<MemWriter<vector<uint8_t>>>, Deserializer<ReaderSource<MemReader>>>(obj);
}

UNIT_TEST(Transit_HeaderRewriting)
{
  TransitHeader const bigHeader(kTransitHeaderVersion /* version */, 500 /* stopsOffset */, 1000 /* gatesOffset */,
                                200000 /* edgesOffset */, 300000 /* transfersOffset */, 400000 /* linesOffset */,
                                5000000 /* shapesOffset */, 6000000 /* networksOffset */, 700000000 /* endOffset */);

  TransitHeader header;
  vector<uint8_t> buffer;
  MemWriter<vector<uint8_t>> writer(buffer);

  // Writing.
  FixedSizeSerializer<MemWriter<vector<uint8_t>>> serializer(writer);
  serializer(header);
  auto const endOffset = writer.Pos();

  // Rewriting.
  header = bigHeader;

  writer.Seek(0 /* start offset */);
  serializer(header);
  TEST_EQUAL(writer.Pos(), endOffset, ());

  // Reading.
  MemReader reader(buffer.data(), buffer.size());
  ReaderSource<MemReader> src(reader);
  TransitHeader deserializedHeader;
  FixedSizeDeserializer<ReaderSource<MemReader>> deserializer(src);
  deserializer(deserializedHeader);

  TEST(deserializedHeader.IsEqualForTesting(bigHeader), (deserializedHeader, bigHeader));
}
}  // namespace transit
}  // namespace routing

namespace
{
UNIT_TEST(Transit_CheckValidSortedUnique)
{
  vector<Network> const networks = {Network(1 /* id */, "Title 1"), Network(2 /* id */, "Title 2")};
  CheckValidSortedUnique(networks, "networks");

  vector<ShapeId> const shapeIds = {ShapeId(1, 2), ShapeId(1, 3)};
  CheckValidSortedUnique(shapeIds, "shapeIds");
}

UNIT_TEST(Transit_HeaderSerialization)
{
  {
    TransitHeader header;
    TestSerialization(header);
    TEST(header.IsValid(), (header));
  }
  {
    TransitHeader header(kTransitHeaderVersion /* version */, 500 /* stopsOffset */, 1000 /* gatesOffset */,
                         2000 /* edgesOffset */, 3000 /* transfersOffset */, 4000 /* linesOffset */,
                         5000 /* shapesOffset */, 6000 /* networksOffset */, 7000 /* endOffset */);
    TestSerialization(header);
    TEST(header.IsValid(), (header));
  }
}

UNIT_TEST(Transit_TransitHeaderValidity)
{
  {
    TransitHeader header;
    TEST(header.IsValid(), (header));
  }
  {
    TransitHeader const header(kTransitHeaderVersion /* version */, 40 /* stopsOffset */, 44 /* gatesOffset */,
                               48 /* edgesOffset */, 52 /* transfersOffset */, 56 /* linesOffset */,
                               60 /* shapesOffset */, 64 /* networksOffset */, 68 /* endOffset */);
    TEST(header.IsValid(), (header));
  }
  {
    TransitHeader const header(kTransitHeaderVersion /* version */, 44 /* stopsOffset */, 40 /* gatesOffset */,
                               48 /* edgesOffset */, 52 /* transfersOffset */, 56 /* linesOffset */,
                               60 /* shapesOffset */, 64 /* networksOffset */, 68 /* endOffset */);
    TEST(!header.IsValid(), (header));
  }
}

UNIT_TEST(Transit_TitleAnchorSerialization)
{
  {
    TitleAnchor anchor(17 /* min zoom */, 0 /* anchor */);
    TestSerialization(anchor);
    TEST(anchor.IsValid(), (anchor));
  }
  {
    TitleAnchor anchor(10 /* min zoom */, 2 /* anchor */);
    TestSerialization(anchor);
    TEST(anchor.IsValid(), (anchor));
  }
  {
    TitleAnchor anchor(18 /* min zoom */, 7 /* anchor */);
    TestSerialization(anchor);
    TEST(anchor.IsValid(), (anchor));
  }
}

UNIT_TEST(Transit_StopSerialization)
{
  {
    Stop stop;
    TestSerialization(stop);
    TEST(!stop.IsValid(), (stop));
  }
  {
    Stop stop(1234 /* id */, 1234567 /* osm id */, 5678 /* feature id */, 7 /* transfer id */,
              {7, 8, 9, 10} /* line id */, {55.0, 37.0} /* point */, {} /* anchors */);
    TestSerialization(stop);
    TEST(stop.IsValid(), (stop));
  }
}

UNIT_TEST(Transit_SingleMwmSegmentSerialization)
{
  {
    SingleMwmSegment s(12344 /* feature id */, 0 /* segmentIdx */, true /* forward */);
    TestSerialization(s);
    TEST(s.IsValid(), (s));
  }
  {
    SingleMwmSegment s(12544 /* feature id */, 5 /* segmentIdx */, false /* forward */);
    TestSerialization(s);
    TEST(s.IsValid(), (s));
  }
}

UNIT_TEST(Transit_GateSerialization)
{
  Gate gate(12345678 /* osm id */, 12345 /* feature id */, true /* entrance */, false /* exit */, 117 /* weight */,
            {1, 2, 3} /* stop ids */, {30.0, 50.0} /* point */);
  TestSerialization(gate);
  TEST(gate.IsValid(), (gate));
}

UNIT_TEST(Transit_GatesRelational)
{
  vector<Gate> const gates = {{1234567 /* osm id */,
                               123 /* feature id */,
                               true /* entrance */,
                               false /* exit */,
                               1 /* weight */,
                               {1, 2, 3} /* stops ids */,
                               m2::PointD(0.0, 0.0)},
                              {12345678 /* osm id */,
                               1234 /* feature id */,
                               true /* entrance */,
                               false /* exit */,
                               1 /* weight */,
                               {1, 2, 3} /* stops ids */,
                               m2::PointD(0.0, 0.0)},
                              {12345678 /* osm id */,
                               1234 /* feature id */,
                               true /* entrance */,
                               false /* exit */,
                               1 /* weight */,
                               {1, 2, 3, 4} /* stops ids */,
                               m2::PointD(0.0, 0.0)}};
  TEST(is_sorted(gates.cbegin(), gates.cend()), ());
}

UNIT_TEST(Transit_EdgeSerialization)
{
  {
    Edge edge(1 /* start stop id */, 2 /* finish stop id */, 123 /* weight */, 11 /* line id */, false /* transfer */,
              {ShapeId(1, 2), ShapeId(3, 4), ShapeId(5, 6)} /* shape ids */);
    TestSerialization(edge);
    TEST(edge.IsValid(), (edge));
  }
  {
    Edge edge(1 /* start stop id */, 2 /* finish stop id */, 123 /* weight */, 11 /* line id */, false /* transfer */,
              {ShapeId(1, 2)} /* shape ids */);
    TestSerialization(edge);
    TEST(edge.IsValid(), (edge));
  }
  {
    Edge edge(1 /* start stop id */, 2 /* finish stop id */, 123 /* weight */, 11 /* line id */, false /* transfer */,
              {ShapeId(2, 1)} /* shape ids */);
    TestSerialization(edge);
    TEST(edge.IsValid(), (edge));
  }
  {
    Edge edge(1 /* start stop id */, 2 /* finish stop id */, 123 /* weight */, kInvalidLineId, true /* transfer */,
              {} /* shape ids */);
    TestSerialization(edge);
    TEST(edge.IsValid(), (edge));
  }
  {
    Edge edge(1 /* start stop id */, 2 /* finish stop id */, 123 /* weight */, 11 /* line id */, true /* transfer */,
              {} /* shape ids */);
    TestSerialization(edge);
    // Note. A transfer edge (transfer == true) with a valid line id is not allowable.
    TEST(!edge.IsValid(), (edge));
  }
}

UNIT_TEST(Transit_TransferSerialization)
{
  Transfer transfer(1 /* id */, {40.0, 35.0} /* point */, {1, 2, 3} /* stop ids */, {TitleAnchor(16, 0 /* anchor */)});
  TestSerialization(transfer);
  TEST(transfer.IsValid(), (transfer));
}

UNIT_TEST(Transit_LineSerialization)
{
  {
    Line line(1 /* line id */, "2" /* number */, "Линия" /* title */, "subway" /* type */, "red" /* color */,
              3 /* network id */, {{1}} /* stop ids */, 10 /* interval */);
    TestSerialization(line);
    TEST(line.IsValid(), (line));
  }
  {
    Line line(10 /* line id */, "11" /* number */, "Линия" /* title */, "subway" /* type */, "green" /* color */,
              12 /* network id */, {{13, 14, 15}} /* stop ids */, 15 /* interval */);
    TestSerialization(line);
    TEST(line.IsValid(), (line));
  }
  {
    Line line(100 /* line id */, "101" /* number */, "Линия" /* title */, "subway" /* type */, "blue" /* color */,
              103 /* network id */, {{1, 2, 3}, {7, 8, 9}} /* stop ids */, 15 /* interval */);
    TestSerialization(line);
    TEST(line.IsValid(), (line));
  }
}

UNIT_TEST(Transit_ShapeSerialization)
{
  {
    Shape shape(ShapeId(10, 20), {m2::PointD(0.0, 20.0), m2::PointD(0.0, 0.0)} /* polyline */);
    TestSerialization(shape);
    TEST(shape.IsValid(), (shape));
  }
  {
    Shape shape(ShapeId(11, 21),
                {m2::PointD(20.0, 20.0), m2::PointD(21.0, 21.0), m2::PointD(22.0, 22.0)} /* polyline */);
    TestSerialization(shape);
    TEST(shape.IsValid(), (shape));
  }
}

UNIT_TEST(Transit_ShapeIdRelational)
{
  vector<ShapeId> const ids = {{0, 10}, {0, 11}, {1, 10}, {1, 11}};
  TEST(is_sorted(ids.cbegin(), ids.cend()), ());
}

UNIT_TEST(Transit_NetworkSerialization)
{
  Network network(0 /* network id */, "Title" /* title */);
  TestSerialization(network);
  TEST(network.IsValid(), (network));
}
}  // namespace
