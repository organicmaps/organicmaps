#include "testing/testing.hpp"

#include "indexer/postcodes.hpp"

#include "coding/reader.hpp"
#include "coding/writer.hpp"

#include <cstdint>
#include <map>
#include <string>

using namespace indexer;
using namespace std;

using Buffer = vector<uint8_t>;

UNIT_TEST(Postcodes_Smoke)
{
  map<uint32_t, string> const features = {
      {1, "127001"}, {5, "aa1 1aa"}, {10, "123"}, {20, "aaa 1111"}};

  Buffer buffer;
  {
    PostcodesBuilder builder;

    for (auto const & feature : features)
      builder.Put(feature.first, feature.second);

    MemWriter<Buffer> writer(buffer);
    builder.Freeze(writer);
  }

  {
    MemReader reader(buffer.data(), buffer.size());
    auto postcodes = Postcodes::Load(reader);
    TEST(postcodes.get(), ());

    string actual;
    for (uint32_t i = 0; i < 100; ++i)
    {
      auto const it = features.find(i);
      if (it != features.end())
      {
        TEST(postcodes->Get(i, actual), ());
        TEST_EQUAL(actual, it->second, ());
      }
      else
      {
        TEST(!postcodes->Get(i, actual), ());
      }
    }
  }
}
