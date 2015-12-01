#include "testing/testing.hpp"

#include "indexer/feature_meta.hpp"

#include "coding/reader.hpp"
#include "coding/writer.hpp"

#include "std/map.hpp"
#include "std/target_os.hpp"

using feature::Metadata;

namespace
{
map<Metadata::EType, string> const kKeyValues =
{
  {Metadata::FMD_ELE, "12345"},
  {Metadata::FMD_CUISINE, "greek;mediterranean"},
  {Metadata::FMD_EMAIL, "cool@email.at"}
};
} // namespace

UNIT_TEST(Feature_Metadata_GetSet)
{
  Metadata m;
  Metadata::EType const type = Metadata::FMD_ELE;
  // Absent types should return empty values.
  TEST_EQUAL(m.Get(type), "", ());
  m.Set(type, "12345");
  TEST_EQUAL(m.Get(type), "12345", ());
  TEST_EQUAL(m.Size(), 1, ());
  // Same types should replace old metadata values.
  m.Set(type, "5678");
  TEST_EQUAL(m.Get(type), "5678", ());
  // Empty values should drop fields.
  m.Set(type, "");
  TEST_EQUAL(m.Get(type), "", ());
  TEST_EQUAL(m.Size(), 0, ());
  TEST(m.Empty(), ());
}

UNIT_TEST(Feature_Metadata_PresentTypes)
{
  Metadata m;
  for (auto const & value : kKeyValues)
    m.Set(value.first, value.second);
  TEST_EQUAL(m.Size(), kKeyValues.size(), ());

  auto const types = m.GetPresentTypes();
  TEST_EQUAL(types.size(), m.Size(), ());
  for (auto const & type : types)
    TEST_EQUAL(m.Get(type), kKeyValues.find(type)->second, ());
}

UNIT_TEST(Feature_Serialization)
{
  Metadata original;
  for (auto const & value : kKeyValues)
    original.Set(value.first, value.second);
  TEST_EQUAL(original.Size(), kKeyValues.size(), ());

  {
    Metadata serialized;
    vector<char> buffer;
    MemWriter<decltype(buffer)> writer(buffer);
    original.Serialize(writer);

    MemReader reader(buffer.data(), buffer.size());
    ReaderSource<MemReader> src(reader);
    serialized.Deserialize(src);

    for (auto const & value : kKeyValues)
      TEST_EQUAL(serialized.Get(value.first), value.second, ());
    TEST_EQUAL(serialized.Get(Metadata::FMD_OPERATOR), "", ());
    TEST_EQUAL(serialized.Size(), kKeyValues.size(), ());
  }

  {
    Metadata serialized;
    vector<char> buffer;
    MemWriter<decltype(buffer)> writer(buffer);
    // Here is the difference.
    original.SerializeToMWM(writer);

    MemReader reader(buffer.data(), buffer.size());
    ReaderSource<MemReader> src(reader);
    // Here is another difference.
    serialized.DeserializeFromMWM(src);

    for (auto const & value : kKeyValues)
      TEST_EQUAL(serialized.Get(value.first), value.second, ());

    TEST_EQUAL(serialized.Get(Metadata::FMD_OPERATOR), "", ());
    TEST_EQUAL(serialized.Size(), kKeyValues.size(), ());
  }
}

UNIT_TEST(Feature_Metadata_GetWikipedia)
{
  Metadata m;
  Metadata::EType const wikiType = Metadata::FMD_WIKIPEDIA;
  m.Set(wikiType, "en:Article");
  TEST_EQUAL(m.Get(wikiType), "en:Article", ());
#ifdef OMIM_OS_MOBILE
  TEST_EQUAL(m.GetWikiURL(), "https://en.m.wikipedia.org/wiki/Article", ());
#else
  TEST_EQUAL(m.GetWikiURL(), "https://en.wikipedia.org/wiki/Article", ());
#endif
}
