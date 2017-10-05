#include "testing/testing.hpp"

#include "coding/reader.hpp"
#include "coding/writer.hpp"

#include "routing_common/transit_serdes.hpp"
#include "routing_common/transit_types.hpp"

#include <cstdint>
#include <vector>

namespace
{
using namespace routing;
using namespace routing::transit;
using namespace std;

template <class Obj>
void TestSerialization(Obj const & obj)
{
  vector<uint8_t> buffer;
  MemWriter<vector<uint8_t>> writer(buffer);

  Serializer<MemWriter<vector<uint8_t>>> serializer(writer);
  obj.Visit(serializer);

  MemReader reader(buffer.data(), buffer.size());
  ReaderSource<MemReader> src(reader);
  Obj deserializedObj;
  Deserializer<ReaderSource<MemReader>> deserializer(src);
  deserializedObj.Visit(deserializer);

  TEST(obj.IsEqualForTesting(deserializedObj), (obj, deserializedObj));
}

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

UNIT_TEST(Transit_StopSerialization)
{
  {
    Stop stop;
    TestSerialization(stop);
  }
  {
    Stop stop(1234 /* id */, 5678 /* feature id */, 7 /* transfer id */, {7, 8, 9, 10} /* line id */, {55.0, 37.0});
    TestSerialization(stop);
  }
}

UNIT_TEST(Transit_GateSerialization)
{
  Gate gate(12345 /* feature id */, true /* entrance */, false /* exit */, 117.8 /* weight */,
            {1, 2, 3} /* stop ids */, {30.0, 50.0} /* point */);
  TestSerialization(gate);
}

UNIT_TEST(Transit_EdgeSerialization)
{
  Edge edge(1 /* start stop id */, 2 /* finish stop id */, 123.4 /* weight */, 11 /* line id */,
            false /* transfer */, {1, 2, 3} /* shape ids */);
  TestSerialization(edge);
}

UNIT_TEST(Transit_TransferSerialization)
{
  Transfer transfer(1 /* id */, {40.0, 35.0} /* point */, {1, 2, 3} /* stop ids */);
  TestSerialization(transfer);
}

UNIT_TEST(Transit_LineSerialization)
{
  {
    Line line(1 /* line id */, "2" /* number */, "Линия" /* title */,
              "subway" /* type */, 3 /* network id */, {} /* stop ids */);
    TestSerialization(line);
  }
  {
    Line line(10 /* line id */, "11" /* number */, "Линия" /* title */,
              "subway" /* type */, 12 /* network id */, {13, 14, 15} /* stop ids */);
    TestSerialization(line);
  }
}

UNIT_TEST(Transit_ShapeSerialization)
{
  {
    Shape shape(1 /* shape id */, 10 /* stop 1 id */, 11 /* stop 2 id */, {} /* polyline */);
    TestSerialization(shape);
  }
  {
    Shape shape(1 /* shape id */, 10 /* stop 1 id */, 11 /* stop 2 id */,
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
