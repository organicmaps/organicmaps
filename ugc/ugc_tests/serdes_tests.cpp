#include "testing/testing.hpp"

#include "ugc/ugc_tests/utils.hpp"

#include "ugc/api.hpp"
#include "ugc/serdes.hpp"
#include "ugc/serdes_json.hpp"
#include "ugc/types.hpp"

#include "coding/reader.hpp"
#include "coding/writer.hpp"

#include <cstdint>
#include <vector>

using namespace std;
using namespace ugc;

namespace
{
using Buffer = vector<uint8_t>;
using ToBin = Serializer<MemWriter<Buffer>>;
using FromBin = DeserializerV0<ReaderSource<MemReader>>;
using ToJson = SerializerJson<MemWriter<Buffer>>;
using FromJson = DeserializerJsonV0<ReaderSource<MemReader>>;

Ratings GetTestRating()
{
  return {{"music" /* key */, 5.0 /* value */}, {"service" /* key */, 4.0 /* value */}};
}

MemWriter<Buffer> MakeSink(Buffer & buffer) { return MemWriter<Buffer>(buffer); }

ReaderSource<MemReader> MakeSource(Buffer const & buffer)
{
  MemReader reader(buffer.data(), buffer.size());
  return ReaderSource<MemReader>(reader);
}

template<typename Object, typename Serializator, typename Deserializator>
void MakeTest(Object const & src)
{
  Buffer buffer;
  Object trg;

  {
    auto sink = MakeSink(buffer);
    Serializator ser(sink);
    ser(src);
  }

  {
    auto source = MakeSource(buffer);
    Deserializator des(source);
    des(trg);
  }
  TEST_EQUAL(src, trg, ());
}

UNIT_TEST(SerDes_Rating)
{
  auto const expectedRating = GetTestRating();
  TEST_EQUAL(expectedRating, expectedRating, ());

  MakeTest<Ratings, ToBin, FromBin>(expectedRating);
}

UNIT_TEST(SerDes_Json_Rating)
{
  auto const expectedRating = GetTestRating();
  TEST_EQUAL(expectedRating, expectedRating, ());

  MakeTest<Ratings, ToJson, FromJson>(expectedRating);
}

UNIT_TEST(SerDes_Json_Reviews)
{
  auto expectedUGC = MakeTestUGC1(Time(chrono::hours(24 * 100))).m_reviews;
  TEST_EQUAL(expectedUGC, expectedUGC, ());

  MakeTest<decltype(expectedUGC), ToJson, FromJson>(expectedUGC);
}

UNIT_TEST(SerDes_Json_RatingRecords)
{
  auto expectedUGC = MakeTestUGC1(Time(chrono::hours(24 * 100))).m_ratings;
  TEST_EQUAL(expectedUGC, expectedUGC, ());

  MakeTest<decltype(expectedUGC), ToJson, FromJson>(expectedUGC);
}

UNIT_TEST(SerDes_Json_UGC)
{
  auto expectedUGC = MakeTestUGC1(Time(chrono::hours(24 * 100)));
  TEST_EQUAL(expectedUGC, expectedUGC, ());

  MakeTest<decltype(expectedUGC), ToJson, FromJson>(expectedUGC);
}

UNIT_TEST(SerDes_UGC)
{
  // Time must be in whole days to prevent lose of precision during
  // serialization/deserialization.
  auto const expectedUGC = MakeTestUGC1(Time(chrono::hours(24 * 100)));
  TEST_EQUAL(expectedUGC, expectedUGC, ());

  Buffer buffer;

  {
    auto sink = MakeSink(buffer);
    Serialize(sink, expectedUGC);
  }

  UGC actualUGC({} /* rating */, {} /* reviews */, {} /* totalRating */, {} /* votes */);
  {
    auto source = MakeSource(buffer);
    Deserialize(source, actualUGC);
  }

  TEST_EQUAL(expectedUGC, actualUGC, ());
}

UNIT_TEST(SerDes_Json_UGCUpdate)
{
  auto expectedUGC = MakeTestUGCUpdate(Time(chrono::hours(24 * 200)));
  TEST_EQUAL(expectedUGC, expectedUGC, ());

  MakeTest<decltype(expectedUGC), ToJson, FromJson>(expectedUGC);
}

UNIT_TEST(SerDes_UGCUpdate)
{
  // Time must be in whole days to prevent lose of precision during
  // serialization/deserialization.
  auto const expectedUGC = MakeTestUGCUpdate(Time(chrono::hours(24 * 300)));
  TEST_EQUAL(expectedUGC, expectedUGC, ());

  Buffer buffer;
  {
    auto sink = MakeSink(buffer);
    Serialize(sink, expectedUGC);
  }

  UGCUpdate actualUGC{};
  {
    auto source = MakeSource(buffer);
    Deserialize(source, actualUGC);
  }

  TEST_EQUAL(expectedUGC, actualUGC, ());
}
}  // namespace
