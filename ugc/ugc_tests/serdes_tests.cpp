#include "testing/testing.hpp"

#include "ugc/api.hpp"
#include "ugc/serdes.hpp"
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
using Ser = Serializer<MemWriter<Buffer>>;
using Des = DeserializerV0<ReaderSource<MemReader>>;

Rating GetTestRating()
{
  vector<RatingRecord> records;
  records.emplace_back("music" /* key */, 5.0 /* value */);
  records.emplace_back("service" /* key */, 4.0 /* value */);

  return Rating(records, 4.5 /* aggValue */);
}

MemWriter<Buffer> MakeSink(Buffer & buffer) { return MemWriter<Buffer>(buffer); }

ReaderSource<MemReader> MakeSource(Buffer const & buffer)
{
  MemReader reader(buffer.data(), buffer.size());
  return ReaderSource<MemReader>(reader);
}

UNIT_TEST(SerDes_Rating)
{
  auto expectedRating = GetTestRating();
  TEST_EQUAL(expectedRating, expectedRating, ());

  HeaderV0 header;

  Buffer buffer;

  {
    auto sink = MakeSink(buffer);
    Ser ser(sink, header);
    ser(expectedRating);
  }

  Rating actualRating({} /* ratings */, {} /* aggValue */);

  {
    auto source = MakeSource(buffer);
    Des des(source, header);
    des(actualRating);
  }

  TEST_EQUAL(expectedRating, actualRating, ());
}

UNIT_TEST(SerDes_UGC)
{
  auto expectedUGC = Api::MakeTestUGC1();
  TEST_EQUAL(expectedUGC, expectedUGC, ());

  HeaderV0 header;

  Buffer buffer;

  {
    auto sink = MakeSink(buffer);
    Ser ser(sink, header);
    ser(expectedUGC);
  }

  UGC actualUGC({} /* rating */, {} /* reviews */, {} /* attributes */);
  {
    auto source = MakeSource(buffer);
    Des des(source, header);
    des(actualUGC);
  }

  TEST_EQUAL(expectedUGC, actualUGC, ());
}
}  // namespace
