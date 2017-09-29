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

  TEST(obj.IsEqualForTesting(deserializedObj), ());
}

UNIT_TEST(ZeroTransitHeaderSerialization)
{
  TransitHeader header;
  TestSerialization(header);
}

UNIT_TEST(TransitHeaderSerialization)
{
  TransitHeader header(1 /* version */, 1000 /* gatesOffset */, 2000 /* edgesOffset */,
                       3000 /* transfersOffset */, 4000 /* linesOffset */, 5000 /* shapesOffset */,
                       6000 /* networksOffset */, 7000 /* endOffset */);
  TestSerialization(header);
}

UNIT_TEST(ZeroTransitStopSerialization)
{
  Stop stop;
  TestSerialization(stop);
}

UNIT_TEST(TransitStopSerialization)
{
  Stop stop(1234 /* id */, 5678 /* feature id */, 7 /* transfer id */, {7, 8, 9, 10} /* line id */, {55.0, 37.0});
  TestSerialization(stop);
}
}  // namespace

