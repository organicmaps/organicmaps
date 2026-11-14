#include "testing/testing.hpp"

#include "search/search_index_header.hpp"
#include "search/search_index_values.hpp"

#include "indexer/trie.hpp"
#include "indexer/trie_builder.hpp"
#include "indexer/trie_reader.hpp"

#include "platform/platform.hpp"

#include "coding/byte_stream.hpp"
#include "coding/compressed_bit_vector.hpp"
#include "coding/files_container.hpp"
#include "coding/reader.hpp"
#include "coding/writer.hpp"

#include "base/logging.hpp"
#include "base/string_utils.hpp"

#include "defines.hpp"

#include <algorithm>
#include <cstdint>
#include <random>
#include <utility>
#include <vector>

namespace search_index_values_tests
{
using Value = Uint64IndexValue;
using List = ValueList<Value>;
using Serializer = SingleValueSerializer<Value>;
using ListFormat = Serializer::ListFormat;
using Header = search::SearchIndexHeader;
using Key = strings::UniString;
using Tokens = std::vector<std::pair<Key, Value>>;
using Ids = std::vector<uint64_t>;
using Bytes = std::vector<uint8_t>;

// The list of the trie builder for the previous format, which the maps of the version 2 have:
// nothing but this test writes it now.
class BitVectorList
{
public:
  using Value = Uint64IndexValue;

  void Init(std::vector<Value> const & values) { m_values = values; }

  size_t Size() const { return m_values.empty() ? 0 : 1; }
  bool IsEmpty() const { return m_values.empty(); }

  template <typename Sink>
  void Serialize(Sink & sink, Serializer const & /* serializer */) const
  {
    if (IsEmpty())
      return;

    Ids ids;
    for (auto const & value : m_values)
      ids.push_back(value.m_featureId);
    Bytes buf;
    MemWriter<Bytes> writer(buf);
    coding::CompressedBitVectorBuilder::FromBitPositions(std::move(ids))->Serialize(writer);
    sink.Write(buf.data(), buf.size());
  }

private:
  std::vector<Value> m_values;
};

template <class TList>
TList MakeList(Ids const & ids)
{
  std::vector<Value> values;
  for (auto const id : ids)
    values.emplace_back(id);
  TList list;
  list.Init(values);
  return list;
}

Ids GetIds(List const & list)
{
  Ids ids;
  list.ForEach([&ids](Value const & value) { ids.push_back(value.m_featureId); });
  return ids;
}

Bytes Serialize(Ids const & ids, ListFormat format)
{
  Bytes bytes;
  PushBackByteSink<Bytes> sink(bytes);
  if (format == ListFormat::IdsPacket)
    MakeList<List>(ids).Serialize(sink, Serializer(format));
  else
    MakeList<BitVectorList>(ids).Serialize(sink, Serializer(format));
  return bytes;
}

Ids Range(uint64_t first, uint64_t count, uint64_t step = 1)
{
  Ids ids;
  for (uint64_t i = 0; i < count; ++i)
    ids.push_back(first + i * step);
  return ids;
}

// The trie as a map has it.
Bytes BuildTrie(Tokens const & tokens, ListFormat format)
{
  Bytes bytes;
  PushBackByteSink<Bytes> sink(bytes);
  if (format == ListFormat::IdsPacket)
    trie::Build<PushBackByteSink<Bytes>, Key, List, Serializer>(sink, Serializer(format), tokens);
  else
    trie::Build<PushBackByteSink<Bytes>, Key, BitVectorList, Serializer>(sink, Serializer(format), tokens);
  std::reverse(bytes.begin(), bytes.end());
  return bytes;
}

// The sorted tokens of a trie with their features.
template <class TReader>
Tokens ReadTrie(TReader const & reader, ListFormat format)
{
  auto const root = trie::ReadTrie<TReader, List>(reader, Serializer(format));
  Tokens tokens;
  trie::ForEachRef(*root, [&tokens](Key const & key, Value const & value) { tokens.emplace_back(key, value); }, Key());
  std::sort(tokens.begin(), tokens.end());
  return tokens;
}

UNIT_TEST(SearchIndexValues_ListFormats)
{
  std::mt19937 rng(1);
  Ids random;
  for (uint64_t id = rng() % 100; random.size() < 5000; id += 1 + rng() % 400)
    random.push_back(id);

  std::vector<Ids> const lists = {
      {},
      {5},
      {1000000},
      {7, 1000000},
      Range(100, 50),
      // Every third feature of a map: a dense bit vector.
      Range(0, 30000, 3),
      Range(1000000, 100000, 7),
      random,
  };

  // The same list object reads all of them: a read replaces its features.
  List list;
  for (auto const & ids : lists)
  {
    for (auto const format : {ListFormat::CompressedBitVector, ListFormat::IdsPacket})
    {
      auto bytes = Serialize(ids, format);
      TEST_EQUAL(bytes.empty(), ids.empty(), ());

      {
        // A leaf of the trie: the list is the whole node.
        MemReader reader(bytes.data(), bytes.size());
        ReaderSource source(reader);
        list.Deserialize(source, Serializer(format));
        TEST_EQUAL(GetIds(list), ids, ());
        TEST_EQUAL(list.IsEmpty(), ids.empty(), ());
        TEST_EQUAL(source.Size(), 0, ());
        // The clone of a trie iterator copies its list.
        TEST_EQUAL(GetIds(List(list)), ids, ());
      }

      // An inner node: other data follow the list.
      size_t constexpr kTailSize = 100000;
      bytes.resize(bytes.size() + kTailSize, 0xAB);
      MemReader reader(bytes.data(), bytes.size());
      ReaderSource source(reader);
      list.Deserialize(source, ids.empty() ? 0 : 1 /* valueCount */, Serializer(format));
      TEST_EQUAL(GetIds(list), ids, ());
      TEST_EQUAL(source.Size(), kTailSize, ());
    }
  }

  // A sparse bit vector takes eight bytes for a feature.
  TEST_EQUAL(Serialize({1000000}, ListFormat::CompressedBitVector).size(), 1 + 1 + 8, ());
  TEST_EQUAL(Serialize({1000000}, ListFormat::IdsPacket).size(), 1 + 3, ());
  TEST_EQUAL(Serialize(Range(1000000, 100000, 7), ListFormat::CompressedBitVector).size(), 1 + 3 + 8 * 100000, ());
  // A bitmap of 10938 words from the first feature.
  TEST_EQUAL(Serialize(Range(1000000, 100000, 7), ListFormat::IdsPacket).size(), 1 + 3 + 2 + 8 * 10938, ());
  TEST_LESS(4 * Serialize(random, ListFormat::IdsPacket).size(),
            Serialize(random, ListFormat::CompressedBitVector).size(), ());
  // A dense bit vector has a bit for every feature from zero, like a bitmap packet here.
  TEST_EQUAL(Serialize(Range(0, 30000, 3), ListFormat::CompressedBitVector).size(), 1 + 2 + 8 * 1407, ());
  TEST_EQUAL(Serialize(Range(0, 30000, 3), ListFormat::IdsPacket).size(), 1 + 1 + 2 + 8 * 1407, ());
}

UNIT_TEST(SearchIndexValues_TrieFormats)
{
  std::mt19937 rng(2);
  Tokens tokens;
  // Tokens with common prefixes; a few of them have many features.
  for (uint32_t i = 0; i < 3000; ++i)
  {
    auto const token = strings::MakeUniString("token" + std::to_string(i % 700) + (i % 3 == 0 ? "x" : ""));
    size_t const count = (i % 100 == 0) ? 2000 : 1 + rng() % 4;
    for (size_t j = 0; j < count; ++j)
      tokens.emplace_back(token, Value(rng() % 500000));
  }
  std::sort(tokens.begin(), tokens.end());
  tokens.erase(std::unique(tokens.begin(), tokens.end()), tokens.end());

  auto const vectors = BuildTrie(tokens, ListFormat::CompressedBitVector);
  auto const packets = BuildTrie(tokens, ListFormat::IdsPacket);
  TEST_LESS(packets.size(), vectors.size() / 2, ());

  TEST(ReadTrie(MemReader(vectors.data(), vectors.size()), ListFormat::CompressedBitVector) == tokens, ());
  TEST(ReadTrie(MemReader(packets.data(), packets.size()), ListFormat::IdsPacket) == tokens, ());
}

// The trie of a map is read in the format of its header and written back byte for byte: for a map
// of the previous version that checks the reader and the writer of this test with real data.
// Its tokens stay the same in both formats.
UNIT_TEST(SearchIndexValues_MapTrie)
{
  FilesContainerR const container(GetPlatform().GetReader("minsk-pass" DATA_FILE_EXTENSION));
  auto reader = container.GetReader(SEARCH_INDEX_FILE_TAG);
  Header header;
  header.Read(*reader.GetPtr());

  auto const tokens = ReadTrie(reader.SubReader(header.m_indexOffset, header.m_indexSize), header.GetListFormat());
  TEST_GREATER(tokens.size(), 10000, ());

  Bytes stored(header.m_indexSize);
  reader.Read(header.m_indexOffset, stored.data(), stored.size());
  TEST(BuildTrie(tokens, header.GetListFormat()) == stored, ());

  auto const vectors = BuildTrie(tokens, ListFormat::CompressedBitVector);
  auto const packets = BuildTrie(tokens, ListFormat::IdsPacket);
  TEST(ReadTrie(MemReader(vectors.data(), vectors.size()), ListFormat::CompressedBitVector) == tokens, ());
  TEST(ReadTrie(MemReader(packets.data(), packets.size()), ListFormat::IdsPacket) == tokens, ());
  TEST_LESS(2 * packets.size(), vectors.size(), ());
  LOG(LINFO, ("The search trie of minsk-pass:", vectors.size(), "bytes with bit vectors,", packets.size(),
              "bytes with packets,", tokens.size(), "features of tokens"));
}

UNIT_TEST(SearchIndexHeader_ListFormat)
{
  // A new index has the lists of the default serializer.
  Header header;
  TEST(header.m_version == Header::Version::Latest, ());
  TEST(header.GetListFormat() == Serializer().GetListFormat(), ());
  TEST(header.GetListFormat() == ListFormat::IdsPacket, ());

  header.m_indexOffset = 16;
  header.m_indexSize = 12345;
  Bytes bytes;
  MemWriter<Bytes> writer(bytes);
  header.Serialize(writer);

  Header read;
  MemReader reader(bytes.data(), bytes.size());
  read.Read(reader);
  TEST(read.m_version == Header::Version::V3, ());
  TEST(read.GetListFormat() == ListFormat::IdsPacket, ());
  TEST_EQUAL(read.m_indexOffset, 16, ());
  TEST_EQUAL(read.m_indexSize, 12345, ());

  // The version of the maps with bit vectors.
  bytes[0] = 2;
  read.Read(reader);
  TEST(read.m_version == Header::Version::V2, ());
  TEST(read.GetListFormat() == ListFormat::CompressedBitVector, ());
}
}  // namespace search_index_values_tests
