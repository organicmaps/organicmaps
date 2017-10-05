#pragma once

#include "ugc/binary/header_v0.hpp"
#include "ugc/binary/index_ugc.hpp"
#include "ugc/binary/visitors.hpp"
#include "ugc/types.hpp"

#include "coding/bwt_coder.hpp"
#include "coding/dd_vector.hpp"
#include "coding/read_write_utils.hpp"
#include "coding/reader.hpp"
#include "coding/text_storage.hpp"
#include "coding/write_to_sink.hpp"

#include "base/assert.hpp"

#include <algorithm>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace ugc
{
namespace binary
{
using FeatureIndex = uint32_t;
using UGCOffset = uint64_t;

enum class Version : uint64_t
{
  V0 = 0,
  Latest = V0
};

class UGCSeriaizer
{
public:
  template <typename IndexUGCList>
  UGCSeriaizer(IndexUGCList && ugcs) : m_ugcs(std::forward<IndexUGCList>(ugcs))
  {
    std::sort(m_ugcs.begin(), m_ugcs.end(), [&](IndexUGC const & lhs, IndexUGC const & rhs) {
      return lhs.m_index < rhs.m_index;
    });
    ASSERT(std::is_sorted(m_ugcs.begin(), m_ugcs.end(),
                          [&](IndexUGC const & lhs, IndexUGC const & rhs) {
                            return lhs.m_index < rhs.m_index;
                          }),
           ());

    CollectTranslationKeys();
    CollectTexts();
  }

  template <typename Sink>
  void Serialize(Sink & sink)
  {
    WriteToSink(sink, Version::Latest);

    auto const startPos = sink.Pos();

    HeaderV0 header;
    WriteZeroesToSink(sink, header.Size());

    header.m_keysOffset = sink.Pos() - startPos;
    SerializeTranslationKeys(sink);

    std::vector<UGCOffset> ugcOffsets;

    header.m_ugcsOffset = sink.Pos() - startPos;
    SerializeUGC(sink, ugcOffsets);

    header.m_indexOffset = sink.Pos() - startPos;
    SerializeIndex(sink, ugcOffsets);

    header.m_textsOffset = sink.Pos() - startPos;
    SerializeTexts(sink);

    header.m_eosOffset = sink.Pos() - startPos;
    sink.Seek(startPos);
    header.Serialize(sink);
    sink.Seek(startPos + header.m_eosOffset);
  }

  // Concatenates all translation keys prefixed with their length as
  // varuint, then compresses them via BWT.
  template <typename Sink>
  void SerializeTranslationKeys(Sink & sink)
  {
    std::string allKeys;
    {
      MemWriter<std::string> writer(allKeys);
      for (auto const & key : m_keys)
        rw::Write(writer, key.m_key);
    }
    coding::BWTCoder::EncodeAndWriteBlock(sink, allKeys);
  }

  // Performs a binary serialization of all UGCS, writes all relative
  // offsets of serialized blobs to |offsets|.
  template <typename Sink>
  void SerializeUGC(Sink & sink, std::vector<UGCOffset> & offsets)
  {
    auto const startPos = sink.Pos();
    ASSERT_EQUAL(m_ugcs.size(), m_texts.size(), ());

    uint64_t textFrom = 0;
    for (size_t i = 0; i < m_ugcs.size(); ++i)
    {
      auto const currPos = sink.Pos();
      offsets.push_back(currPos - startPos);

      SerializerVisitor<Sink> ser(sink, m_keys, m_texts[i], textFrom);
      ser(m_ugcs[i].m_ugc);

      textFrom += m_texts[i].size();
    }
  }

  // Serializes feature ids and offsets of UGC blobs as two fixed-bits
  // vectors. Length of vectors is the number of UGCs. The first
  // vector is 32-bit sorted feature ids of UGC objects. The second
  // vector is 64-bit offsets of corresponding UGC blobs in the ugc
  // section.
  template <typename Sink>
  void SerializeIndex(Sink & sink, std::vector<UGCOffset> const & offsets)
  {
    ASSERT_EQUAL(m_ugcs.size(), offsets.size(), ());
    for (auto const & p : m_ugcs)
      WriteToSink(sink, p.m_index);
    for (auto const & offset : offsets)
      WriteToSink(sink, offset);
  }

  // Serializes texts in a compressed storage with block access.
  template <typename Sink>
  void SerializeTexts(Sink & sink)
  {
    coding::BlockedTextStorageWriter<Sink> writer(sink, 200000 /* blockSize */);
    for (auto const & collection : m_texts)
    {
      for (auto const & text : collection)
        writer.Append(text.m_text);
    }
  }

  std::vector<TranslationKey> const & GetTranslationKeys() const { return m_keys; }
  std::vector<std::vector<Text>> const & GetTexts() const { return m_texts; }

private:
  void CollectTranslationKeys();
  void CollectTexts();

  std::vector<IndexUGC> m_ugcs;
  std::vector<TranslationKey> m_keys;
  std::vector<vector<Text>> m_texts;
};

// Deserializer for UGC. May be used for random-access, but it is more
// efficient to keep it alive between accesses. The instances of
// |reader| for Deserialize() may differ between calls, but all
// instances must be set to the beginning of the UGC section
class UGCDeserializer
{
public:
  template <typename R>
  bool Deserialize(R & reader, FeatureIndex index, UGC & ugc)
  {
    NonOwningReaderSource source(reader);
    Version v = ReadPrimitiveFromSource<Version>(source);

    auto subReader = reader.CreateSubReader(source.Pos(), source.Size());

    switch (v)
    {
    case Version::V0: return DeserializeV0(*subReader, index, ugc);
    default: ASSERT(false, ("Cannot deserialize ugc for version", v));
    }

    return false;
  }

  template <typename R>
  bool DeserializeV0(R & reader, FeatureIndex index, UGC & ugc)
  {
    InitializeIfNeeded(reader);

    UGCOffset offset = 0;
    {
      ReaderPtr<Reader> idsSubReader(CreateFeatureIndexesSubReader(reader));
      DDVector<FeatureIndex, ReaderPtr<Reader>> ids(idsSubReader);
      auto const it = std::lower_bound(ids.begin(), ids.end(), index);
      if (it == ids.end() || *it != index)
        return false;

      auto const d = static_cast<uint32_t>(distance(ids.begin(), it));

      ReaderPtr<Reader> ofsSubReader(CreateUGCOffsetsSubReader(reader));
      DDVector<UGCOffset, ReaderPtr<Reader>> ofs(ofsSubReader);
      offset = ofs[d];
    }

    {
      auto ugcSubReader = CreateUGCSubReader(reader);
      NonOwningReaderSource source(*ugcSubReader);
      source.Skip(offset);

      auto textsSubReader = CreateTextsSubReader(reader);
      DeserializerVisitorV0<NonOwningReaderSource> des(source, m_keys, *textsSubReader, m_texts);
      des(ugc);
    }

    return true;
  }

  std::vector<TranslationKey> const & GetTranslationKeys() const { return m_keys; }

private:
  template <typename Reader>
  void InitializeIfNeeded(Reader & reader)
  {
    if (m_initialized)
      return;

    {
      NonOwningReaderSource source(reader);
      m_header.Deserialize(source);
    }

    {
      ASSERT_GREATER_OR_EQUAL(m_header.m_ugcsOffset, m_header.m_keysOffset, ());

      auto const pos = m_header.m_keysOffset;
      auto const size = m_header.m_ugcsOffset - pos;

      auto subReader = reader.CreateSubReader(pos, size);
      NonOwningReaderSource source(*subReader);
      DeserializeTranslationKeys(source);
    }

    m_initialized = true;
  }

  template <typename Source>
  void DeserializeTranslationKeys(Source & source)
  {
    ASSERT(m_keys.empty(), ());

    std::vector<uint8_t> block;
    coding::BWTCoder::ReadAndDecodeBlock(source, std::back_inserter(block));

    MemReader blockReader(block.data(), block.size());
    NonOwningReaderSource blockSource(blockReader);
    while (blockSource.Size() != 0)
    {
      std::string key;
      rw::Read(blockSource, key);
      m_keys.emplace_back(move(key));
    }
  }

  uint64_t GetNumUGCs()
  {
    ASSERT(m_initialized, ());
    ASSERT_GREATER_OR_EQUAL(m_header.m_textsOffset, m_header.m_indexOffset, ());
    auto const totalSize = m_header.m_textsOffset - m_header.m_indexOffset;

    size_t constexpr kIndexOffset = sizeof(FeatureIndex) + sizeof(UGCOffset);
    ASSERT(totalSize % kIndexOffset == 0, (totalSize));

    return totalSize / kIndexOffset;
  }

  template <typename R>
  std::unique_ptr<Reader> CreateUGCSubReader(R & reader)
  {
    ASSERT(m_initialized, ());

    auto const pos = m_header.m_ugcsOffset;
    ASSERT_GREATER_OR_EQUAL(m_header.m_indexOffset, pos, ());
    auto const size = m_header.m_indexOffset - pos;
    return reader.CreateSubReader(pos, size);
  }

  template <typename R>
  std::unique_ptr<Reader> CreateFeatureIndexesSubReader(R & reader)
  {
    ASSERT(m_initialized, ());

    auto const pos = m_header.m_indexOffset;
    auto const n = GetNumUGCs();
    return reader.CreateSubReader(pos, n * sizeof(FeatureIndex));
  }

  template <typename R>
  std::unique_ptr<Reader> CreateUGCOffsetsSubReader(R & reader)
  {
    ASSERT(m_initialized, ());

    auto const pos = m_header.m_indexOffset;
    auto const n = GetNumUGCs();
    return reader.CreateSubReader(pos + n * sizeof(FeatureIndex), n * sizeof(UGCOffset));
  }

  template <typename R>
  std::unique_ptr<Reader> CreateTextsSubReader(R & reader)
  {
    ASSERT(m_initialized, ());

    auto const pos = m_header.m_textsOffset;
    ASSERT_GREATER_OR_EQUAL(m_header.m_eosOffset, pos, ());
    auto const size = m_header.m_eosOffset - pos;
    return reader.CreateSubReader(pos, size);
  }

  HeaderV0 m_header;
  std::vector<TranslationKey> m_keys;
  coding::BlockedTextStorageReader m_texts;

  bool m_initialized = false;
};

std::string DebugPrint(Version v);
}  // namespace binary
}  // namespace ugc
