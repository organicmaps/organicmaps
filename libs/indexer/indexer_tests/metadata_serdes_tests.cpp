#include "testing/testing.hpp"

#include "indexer/feature_meta.hpp"
#include "indexer/metadata_serdes.hpp"

#include "coding/reader.hpp"
#include "coding/writer.hpp"

#include "base/string_utils.hpp"

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace
{
using Buffer = std::vector<uint8_t>;

UNIT_TEST(MetadataSerDesTest_Smoke)
{
  Buffer buffer;

  auto const genMeta = [](uint32_t i)
  {
    feature::Metadata meta;
    meta.Set(feature::Metadata::FMD_TEST_ID, strings::to_string(i));
    return meta;
  };

  uint32_t constexpr kMetaNumber = 1000;
  std::map<uint32_t, feature::Metadata> values;
  for (uint32_t i = 0; i < kMetaNumber; ++i)
    values.emplace(i, genMeta(i));

  {
    indexer::MetadataBuilder builder;

    for (auto const & kv : values)
      builder.Put(kv.first, kv.second);

    MemWriter<Buffer> writer(buffer);
    builder.Freeze(writer);
  }

  {
    MemReader reader(buffer.data(), buffer.size());
    auto deserializer = indexer::MetadataDeserializer::Load(reader);
    TEST(deserializer.get(), ());

    for (uint32_t i = 0; i < kMetaNumber; ++i)
    {
      feature::Metadata meta;
      TEST(deserializer->Get(i, meta), ());
      TEST(meta.Equals(values[i]), (meta, i));
    }
  }

  {
    MemReader reader(buffer.data(), buffer.size());
    auto deserializer = indexer::MetadataDeserializer::Load(reader);
    TEST(deserializer.get(), ());

    for (uint32_t i = 0; i < kMetaNumber; ++i)
    {
      indexer::MetadataDeserializer::MetaIds ids;
      TEST(deserializer->GetIds(i, ids), ());
      auto const & meta = values[i];
      TEST_EQUAL(meta.Size(), 1, (meta));
      TEST_EQUAL(ids.size(), 1, (ids));
      TEST(meta.Has(feature::Metadata::FMD_TEST_ID), (meta));
      TEST_EQUAL(ids[0].first, feature::Metadata::FMD_TEST_ID, (ids));
      TEST_EQUAL(meta.Get(feature::Metadata::FMD_TEST_ID), deserializer->GetMetaById(ids[0].second), (i, meta, ids));
    }
  }
}
}  // namespace
