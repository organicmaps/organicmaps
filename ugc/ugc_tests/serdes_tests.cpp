#include "testing/testing.hpp"

#include "ugc/serdes.hpp"
#include "ugc/types.hpp"

#include "coding/reader.hpp"
#include "coding/writer.hpp"

#include <chrono>
#include <cstdint>
#include <vector>

using namespace std;
using namespace ugc;

namespace
{
using Buffer = vector<uint8_t>;
using Ser = Serializer<MemWriter<Buffer>>;
using Des = DeserializerV0<ReaderSource<MemReader>>;

chrono::hours FromDays(uint32_t days)
{
  return std::chrono::hours(days * 24);
}

Rating GetTestRating()
{
  vector<RatingRecord> records;
  records.emplace_back("music" /* key */, 5.0 /* value */);
  records.emplace_back("service" /* key */, 4.0 /* value */);

  return Rating(records, 4.5 /* aggValue */);
}

UGC GetTestUGC()
{
  Rating rating;
  rating.m_ratings.emplace_back("food" /* key */, 4.0 /* value */);
  rating.m_ratings.emplace_back("service" /* key */, 5.0 /* value */);
  rating.m_ratings.emplace_back("music" /* key */, 5.0 /* value */);
  rating.m_aggValue = 4.5;

  vector<Review> reviews;
  reviews.emplace_back(20 /* id */, Text("Damn good coffee", StringUtf8Multilang::kEnglishCode),
                       Author(UID(987654321 /* hi */, 123456789 /* lo */), "Cole"),
                       5.0 /* rating */, Sentiment::Positive, Time(FromDays(10)));
  reviews.emplace_back(67812 /* id */,
                       Text("Clean place, reasonably priced", StringUtf8Multilang::kDefaultCode),
                       Author(UID(0 /* hi */, 315 /* lo */), "Cooper"), 5.0 /* rating */,
                       Sentiment::Positive, Time(FromDays(1)));

  vector<Attribute> attributes;
  attributes.emplace_back("best-drink", "Coffee");

  return UGC(rating, reviews, attributes);
}

MemWriter<Buffer> MakeSink(Buffer & buffer)
{
  return MemWriter<Buffer>(buffer);
}

ReaderSource<MemReader> MakeSource(Buffer const & buffer)
{
  MemReader reader(buffer.data(), buffer.size());
  return ReaderSource<MemReader>(reader);
}

UNIT_TEST(SerDes_Rating)
{
  auto const expectedRating = GetTestRating();
  TEST_EQUAL(expectedRating, expectedRating, ());

  HeaderV0 header;

  Buffer buffer;

  {
    auto sink = MakeSink(buffer);
    Ser(sink, header)(expectedRating);
  }

  Rating actualRating({} /* ratings */, {} /* aggValue */);

  {
    auto source = MakeSource(buffer);
    Des(source, header)(actualRating);
  }

  TEST_EQUAL(expectedRating, actualRating, ());
}

UNIT_TEST(SerDes_UGC)
{
  auto const expectedUGC = GetTestUGC();
  TEST_EQUAL(expectedUGC, expectedUGC, ());

  HeaderV0 header;

  Buffer buffer;

  {
    auto sink = MakeSink(buffer);
    Ser(sink, header)(expectedUGC);
  }

  UGC actualUGC({} /* rating */, {} /* reviews */, {} /* attributes */);
  {
    auto source = MakeSource(buffer);
    Des(source, header)(actualUGC);
  }

  TEST_EQUAL(expectedUGC, actualUGC, ());
}
}  // namespace
