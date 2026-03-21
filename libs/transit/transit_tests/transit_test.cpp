#include "testing/testing.hpp"

#include "transit/transit_serdes.hpp"
#include "transit/transit_types.hpp"
#include "transit/transit_version.hpp"

#include "coding/reader.hpp"
#include "coding/writer.hpp"

#include <algorithm>
#include <cstdint>
#include <vector>

namespace routing
{
namespace transit
{
auto constexpr kTransitHeaderVersion = static_cast<uint16_t>(::transit::TransitVersion::OnlySubway);

template <class S, class D, class Obj>
void TestCommonSerialization(Obj const & obj)
{
  std::vector<uint8_t> buffer;
  MemWriter<std::vector<uint8_t>> writer(buffer);

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
  TestCommonSerialization<FixedSizeSerializer<MemWriter<std::vector<uint8_t>>>,
                          FixedSizeDeserializer<ReaderSource<MemReader>>>(header);
}

template <class Obj>
void TestSerialization(Obj const & obj)
{
  TestCommonSerialization<Serializer<MemWriter<std::vector<uint8_t>>>, Deserializer<ReaderSource<MemReader>>>(obj);
}

UNIT_TEST(Transit_HeaderRewriting)
{
  TransitHeader const bigHeader(kTransitHeaderVersion /* version */, 500 /* stopsOffset */, 1000 /* gatesOffset */,
                                200000 /* edgesOffset */, 300000 /* transfersOffset */, 400000 /* linesOffset */,
                                5000000 /* shapesOffset */, 6000000 /* networksOffset */, 700000000 /* endOffset */);

  TransitHeader header;
  std::vector<uint8_t> buffer;
  MemWriter<std::vector<uint8_t>> writer(buffer);

  // Writing.
  FixedSizeSerializer<MemWriter<std::vector<uint8_t>>> serializer(writer);
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
  std::vector<routing::transit::Network> const networks = {routing::transit::Network(1 /* id */, "Title 1"),
                                                           routing::transit::Network(2 /* id */, "Title 2")};
  routing::transit::CheckValidSortedUnique(networks, "networks");

  std::vector<routing::transit::ShapeId> const shapeIds = {routing::transit::ShapeId(1, 2),
                                                           routing::transit::ShapeId(1, 3)};
  routing::transit::CheckValidSortedUnique(shapeIds, "shapeIds");
}

UNIT_TEST(Transit_HeaderSerialization)
{
  {
    routing::transit::TransitHeader header;
    routing::transit::TestSerialization(header);
    TEST(header.IsValid(), (header));
  }
  {
    routing::transit::TransitHeader header(routing::transit::kTransitHeaderVersion /* version */, 500 /* stopsOffset */,
                                           1000 /* gatesOffset */, 2000 /* edgesOffset */, 3000 /* transfersOffset */,
                                           4000 /* linesOffset */, 5000 /* shapesOffset */, 6000 /* networksOffset */,
                                           7000 /* endOffset */);
    routing::transit::TestSerialization(header);
    TEST(header.IsValid(), (header));
  }
}

UNIT_TEST(Transit_TransitHeaderValidity)
{
  {
    routing::transit::TransitHeader header;
    TEST(header.IsValid(), (header));
  }
  {
    routing::transit::TransitHeader const header(routing::transit::kTransitHeaderVersion /* version */,
                                                 40 /* stopsOffset */, 44 /* gatesOffset */, 48 /* edgesOffset */,
                                                 52 /* transfersOffset */, 56 /* linesOffset */, 60 /* shapesOffset */,
                                                 64 /* networksOffset */, 68 /* endOffset */);
    TEST(header.IsValid(), (header));
  }
  {
    routing::transit::TransitHeader const header(routing::transit::kTransitHeaderVersion /* version */,
                                                 44 /* stopsOffset */, 40 /* gatesOffset */, 48 /* edgesOffset */,
                                                 52 /* transfersOffset */, 56 /* linesOffset */, 60 /* shapesOffset */,
                                                 64 /* networksOffset */, 68 /* endOffset */);
    TEST(!header.IsValid(), (header));
  }
}

UNIT_TEST(Transit_TitleAnchorSerialization)
{
  {
    routing::transit::TitleAnchor anchor(17 /* min zoom */, 0 /* anchor */);
    routing::transit::TestSerialization(anchor);
    TEST(anchor.IsValid(), (anchor));
  }
  {
    routing::transit::TitleAnchor anchor(10 /* min zoom */, 2 /* anchor */);
    routing::transit::TestSerialization(anchor);
    TEST(anchor.IsValid(), (anchor));
  }
  {
    routing::transit::TitleAnchor anchor(18 /* min zoom */, 7 /* anchor */);
    routing::transit::TestSerialization(anchor);
    TEST(anchor.IsValid(), (anchor));
  }
}

UNIT_TEST(Transit_StopSerialization)
{
  {
    routing::transit::Stop stop;
    routing::transit::TestSerialization(stop);
    TEST(!stop.IsValid(), (stop));
  }
  {
    routing::transit::Stop stop(1234 /* id */, 1234567 /* osm id */, 5678 /* feature id */, 7 /* transfer id */,
                                {7, 8, 9, 10} /* line id */, {55.0, 37.0} /* point */, {} /* anchors */);
    routing::transit::TestSerialization(stop);
    TEST(stop.IsValid(), (stop));
  }
}

UNIT_TEST(Transit_SingleMwmSegmentSerialization)
{
  {
    routing::transit::SingleMwmSegment s(12344 /* feature id */, 0 /* segmentIdx */, true /* forward */);
    routing::transit::TestSerialization(s);
    TEST(s.IsValid(), (s));
  }
  {
    routing::transit::SingleMwmSegment s(12544 /* feature id */, 5 /* segmentIdx */, false /* forward */);
    routing::transit::TestSerialization(s);
    TEST(s.IsValid(), (s));
  }
}

UNIT_TEST(Transit_GateSerialization)
{
  routing::transit::Gate gate(12345678 /* osm id */, 12345 /* feature id */, true /* entrance */, false /* exit */,
                              117 /* weight */, {1, 2, 3} /* stop ids */, {30.0, 50.0} /* point */);
  routing::transit::TestSerialization(gate);
  TEST(gate.IsValid(), (gate));
}

UNIT_TEST(Transit_GatesRelational)
{
  std::vector<routing::transit::Gate> const gates = {{1234567 /* osm id */,
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
  TEST(std::is_sorted(gates.cbegin(), gates.cend()), ());
}

UNIT_TEST(Transit_EdgeSerialization)
{
  {
    routing::transit::Edge edge(1 /* start stop id */, 2 /* finish stop id */, 123 /* weight */, 11 /* line id */,
                                false /* transfer */,
                                {routing::transit::ShapeId(1, 2), routing::transit::ShapeId(3, 4),
                                 routing::transit::ShapeId(5, 6)} /* shape ids */);
    routing::transit::TestSerialization(edge);
    TEST(edge.IsValid(), (edge));
  }
  {
    routing::transit::Edge edge(1 /* start stop id */, 2 /* finish stop id */, 123 /* weight */, 11 /* line id */,
                                false /* transfer */, {routing::transit::ShapeId(1, 2)} /* shape ids */);
    routing::transit::TestSerialization(edge);
    TEST(edge.IsValid(), (edge));
  }
  {
    routing::transit::Edge edge(1 /* start stop id */, 2 /* finish stop id */, 123 /* weight */, 11 /* line id */,
                                false /* transfer */, {routing::transit::ShapeId(2, 1)} /* shape ids */);
    routing::transit::TestSerialization(edge);
    TEST(edge.IsValid(), (edge));
  }
  {
    routing::transit::Edge edge(1 /* start stop id */, 2 /* finish stop id */, 123 /* weight */,
                                routing::transit::kInvalidLineId, true /* transfer */, {} /* shape ids */);
    routing::transit::TestSerialization(edge);
    TEST(edge.IsValid(), (edge));
  }
  {
    routing::transit::Edge edge(1 /* start stop id */, 2 /* finish stop id */, 123 /* weight */, 11 /* line id */,
                                true /* transfer */, {} /* shape ids */);
    routing::transit::TestSerialization(edge);
    // Note. A transfer edge (transfer == true) with a valid line id is not allowable.
    TEST(!edge.IsValid(), (edge));
  }
}

UNIT_TEST(Transit_TransferSerialization)
{
  routing::transit::Transfer transfer(1 /* id */, {40.0, 35.0} /* point */, {1, 2, 3} /* stop ids */,
                                      {routing::transit::TitleAnchor(16, 0 /* anchor */)});
  routing::transit::TestSerialization(transfer);
  TEST(transfer.IsValid(), (transfer));
}

UNIT_TEST(Transit_LineSerialization)
{
  {
    routing::transit::Line line(1 /* line id */, "2" /* number */, "Линия" /* title */, "subway" /* type */,
                                "red" /* color */, 3 /* network id */, {{1}} /* stop ids */, 10 /* interval */);
    routing::transit::TestSerialization(line);
    TEST(line.IsValid(), (line));
  }
  {
    routing::transit::Line line(10 /* line id */, "11" /* number */, "Линия" /* title */, "subway" /* type */,
                                "green" /* color */, 12 /* network id */, {{13, 14, 15}} /* stop ids */,
                                15 /* interval */);
    routing::transit::TestSerialization(line);
    TEST(line.IsValid(), (line));
  }
  {
    routing::transit::Line line(100 /* line id */, "101" /* number */, "Линия" /* title */, "subway" /* type */,
                                "blue" /* color */, 103 /* network id */, {{1, 2, 3}, {7, 8, 9}} /* stop ids */,
                                15 /* interval */);
    routing::transit::TestSerialization(line);
    TEST(line.IsValid(), (line));
  }
}

UNIT_TEST(Transit_ShapeSerialization)
{
  {
    routing::transit::Shape shape(routing::transit::ShapeId(10, 20),
                                  {m2::PointD(0.0, 20.0), m2::PointD(0.0, 0.0)} /* polyline */);
    routing::transit::TestSerialization(shape);
    TEST(shape.IsValid(), (shape));
  }
  {
    routing::transit::Shape shape(routing::transit::ShapeId(11, 21), {m2::PointD(20.0, 20.0), m2::PointD(21.0, 21.0),
                                                                      m2::PointD(22.0, 22.0)} /* polyline */);
    routing::transit::TestSerialization(shape);
    TEST(shape.IsValid(), (shape));
  }
}

UNIT_TEST(Transit_ShapeIdRelational)
{
  std::vector<routing::transit::ShapeId> const ids = {{0, 10}, {0, 11}, {1, 10}, {1, 11}};
  TEST(std::is_sorted(ids.cbegin(), ids.cend()), ());
}

UNIT_TEST(Transit_NetworkSerialization)
{
  routing::transit::Network network(0 /* network id */, "Title" /* title */);
  routing::transit::TestSerialization(network);
  TEST(network.IsValid(), (network));
}
}  // namespace
