#include "testing/testing.hpp"

#include "indexer/feature_meta.hpp"

#include "coding/reader.hpp"
#include "coding/writer.hpp"

#include "std/map.hpp"

using feature::Metadata;

UNIT_TEST(Feature_Serialization)
{
  Metadata original;
  for (auto const & value : kPairs)
    original.Set(value.first, value.second);
  TEST_EQUAL(original.Size(), kPairs.size(), ());

  {
    Metadata serialized;
    vector<char> buffer;
    MemWriter<vector<char> > writer(buffer);
    original.Serialize(writer);

    MemReader reader(buffer.data(), buffer.size());
    ReaderSource<MemReader> src(reader);
    serialized.Deserialize(src);

    for (auto const & value : kPairs)
      TEST_EQUAL(serialized.Get(value.first), value.second, ());
    TEST_EQUAL(serialized.Get(Metadata::FMD_OPERATOR), "", ());
    TEST_EQUAL(serialized.Size(), kPairs.size(), ());
  }

  {
    Metadata serialized;
    vector<char> buffer;
    MemWriter<vector<char> > writer(buffer);
    // Here is the difference.
    original.SerializeToMWM(writer);

    MemReader reader(buffer.data(), buffer.size());
    ReaderSource<MemReader> src(reader);
    // Here is another difference.
    serialized.DeserializeFromMWM(src);

    for (auto const & value : kPairs)
      TEST_EQUAL(serialized.Get(value.first), value.second, ());
    TEST_EQUAL(serialized.Get(Metadata::FMD_OPERATOR), "", ());
    TEST_EQUAL(serialized.Size(), kPairs.size(), ());
  }
}
