#pragma once

#include "indexer/v2/ids_packet.hpp"

#include "coding/byte_stream.hpp"
#include "coding/compressed_bit_vector.hpp"
#include "coding/write_to_sink.hpp"

#include "base/assert.hpp"
#include "base/buffer_vector.hpp"
#include "base/checked_cast.hpp"

#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

/// Following classes are supposed to be used with StringsFile. They
/// allow to write/read them, compare or serialize to an in-memory
/// buffer. The reason to use these classes instead of
/// buffer_vector<uint8_t, N> is that in some specific cases, like
/// compressed search index construction, they allow to avoid
/// redundant serialization-deserialization or sorting.

// A wrapper around feature index.
struct Uint64IndexValue
{
  Uint64IndexValue() = default;

  explicit Uint64IndexValue(uint64_t featureId) : m_featureId(featureId) {}

  auto operator<=>(Uint64IndexValue const &) const = default;

  void Swap(Uint64IndexValue & o) { std::swap(m_featureId, o.m_featureId); }

  uint64_t m_featureId = 0;
};

namespace std
{
template <>
struct hash<Uint64IndexValue>
{
public:
  size_t operator()(Uint64IndexValue const & value) const { return std::hash<uint64_t>{}(value.m_featureId); }
};
}  // namespace std

template <typename Value>
class SingleValueSerializer;

template <>
class SingleValueSerializer<Uint64IndexValue>
{
public:
  using Value = Uint64IndexValue;

  // How ValueList stores the features of a trie node.
  enum class ListFormat : uint8_t
  {
    // A serialized compressed bit vector: eight bytes for every feature, or a bit for every
    // feature of the map up to the last one of the list.
    CompressedBitVector,
    // idx::v2::IdsPacket.
    IdsPacket
  };

  explicit SingleValueSerializer(ListFormat listFormat = ListFormat::IdsPacket) : m_listFormat(listFormat) {}

  ListFormat GetListFormat() const { return m_listFormat; }

  // The serialization and deserialization is needed for StringsFile.
  // Use ValueList for group serialization.
  template <typename Sink>
  void Serialize(Sink & sink, Value const & v) const
  {
    WriteToSink(sink, v.m_featureId);
  }

  template <typename Reader>
  void Deserialize(Reader & reader, Value & v) const
  {
    ReaderSource<Reader> source(reader);
    DeserializeFromSource(source, v);
  }

  template <typename Source>
  void DeserializeFromSource(Source & source, Value & v) const
  {
    v.m_featureId = ReadPrimitiveFromSource<uint64_t>(source);
  }

private:
  ListFormat m_listFormat;
};

// This template is used to accumulate, serialize and deserialize
// a group of values of the same type.
template <typename Value>
class ValueList;

// ValueList<Uint64IndexValue> keeps the feature indices of a trie node the way they are stored,
// not as a vector of ids, to take less memory: the bytes of an ids packet, or the bit vector of
// an old map. ForEach() decodes them.
template <>
class ValueList<Uint64IndexValue>
{
public:
  using Value = Uint64IndexValue;
  using ListFormat = SingleValueSerializer<Value>::ListFormat;

  ValueList() = default;

  ValueList(ValueList<Uint64IndexValue> const & o) : m_packet(o.m_packet)
  {
    if (o.m_cbv)
      m_cbv = o.m_cbv->Clone();
  }

  void Init(std::vector<Uint64IndexValue> const & values)
  {
    m_cbv.reset();
    m_packet.clear();
    if (values.empty())
      return;

    std::vector<uint32_t> ids(values.size());
    for (size_t i = 0; i < ids.size(); ++i)
      ids[i] = base::asserted_cast<uint32_t>(values[i].m_featureId);
    PushBackByteSink<Packet> sink(m_packet);
    idx::v2::IdsPacket::Write(sink, ids, ids.front());
  }

  // This method returns number of values in the current instance of
  // ValueList<Uint64IndexValue>, but as these values are actually
  // features indices and can be dumped as a single serialized
  // list, this method returns 1 when there're at least one
  // feature's index in the list and 0 otherwise.
  size_t Size() const { return IsEmpty() ? 0 : 1; }

  bool IsEmpty() const { return m_packet.empty() && !m_cbv; }

  template <typename Sink>
  void Serialize(Sink & sink, SingleValueSerializer<Value> const & serializer) const
  {
    // Only a list of Init() is written, and only as a packet: the bit vectors are of the old maps.
    ASSERT(!m_cbv, ());
    ASSERT(serializer.GetListFormat() == ListFormat::IdsPacket, ());
    if (!IsEmpty())
      sink.Write(m_packet.data(), m_packet.size());
  }

  // Note the valueCount parameter. It is here for compatibility with
  // an old data format that was serializing FeatureWithRankAndCenter`s.
  // They were put in a vector, this vector's size was encoded somehow
  // and then the vector was written with a method similar to Serialize above.
  // The deserialization code read the valueCount separately and then
  // read each FeatureWithRankAndCenter one by one.
  // A better approach is to make Serialize/Deserialize responsible for
  // every part of serialization and as such it should not need valueCount.
  template <typename Source>
  void Deserialize(Source & src, uint32_t valueCount, SingleValueSerializer<Value> const & serializer)
  {
    m_cbv.reset();
    m_packet.clear();
    if (valueCount == 0)
      return;

    if (serializer.GetListFormat() == ListFormat::CompressedBitVector)
    {
      m_cbv = coding::CompressedBitVectorBuilder::DeserializeFromSource(src);
      return;
    }

    // Only the parser of a packet finds its end, so a copy of the source goes ahead for the size.
    Source end(src);
    idx::v2::IdsPacket::Skip(end, idx::v2::IdsPacket::Singleton::Split);
    m_packet.resize(static_cast<size_t>(end.Pos() - src.Pos()));
    src.Read(m_packet.data(), m_packet.size());
  }

  // Reads the list of a leaf, which takes all the bytes of the source.
  template <typename Source>
  void Deserialize(Source & src, SingleValueSerializer<Value> const & serializer)
  {
    size_t const size = static_cast<size_t>(src.Size());
    if (size == 0 || serializer.GetListFormat() == ListFormat::CompressedBitVector)
    {
      Deserialize(src, size == 0 ? 0 : 1 /* valueCount */, serializer);
      return;
    }

    // The bytes of a leaf are its packet: no need to parse it for the size.
    m_cbv.reset();
    m_packet.resize(size);
    src.Read(m_packet.data(), size);
  }

  template <typename ToDo>
  void ForEach(ToDo && toDo) const
  {
    if (m_cbv)
    {
      coding::CompressedBitVectorEnumerator::ForEach(*m_cbv, [&toDo](uint64_t id) { toDo(Value(id)); });
      return;
    }
    if (m_packet.empty())
      return;

    auto const toFirstId = [](uint64_t first) { return static_cast<uint32_t>(first); };
    auto const toValue = [&toDo](uint32_t id) { toDo(Value(id)); };
    ArrayByteSource src(m_packet.data());
    uint32_t link = 0;
    VERIFY(idx::v2::IdsPacket::Read(src, idx::v2::IdsPacket::Singleton::Split, link, toFirstId, toValue), ());
    ASSERT(src.PtrUint8() == m_packet.data() + m_packet.size(), ());
  }

private:
  // Enough for 80-90% of the lists of a map, which need no allocation then.
  using Packet = buffer_vector<uint8_t, 16>;

  // At most one of them is set.
  Packet m_packet;
  std::unique_ptr<coding::CompressedBitVector> m_cbv;
};

class SingleUint64Value
{
public:
  using Value = Uint64IndexValue;

  SingleUint64Value() = default;

  SingleUint64Value(SingleUint64Value const & o)
  {
    m_empty = o.m_empty;
    m_val = o.m_val;
  }

  void Init(std::vector<Uint64IndexValue> const & values)
  {
    CHECK_LESS_OR_EQUAL(values.size(), 1, ());
    m_empty = values.empty();
    if (!m_empty)
      m_val = values[0].m_featureId;
  }

  size_t Size() const { return m_empty ? 0 : 1; }

  bool IsEmpty() const { return m_empty; }

  template <typename Sink>
  void Serialize(Sink & sink, SingleValueSerializer<Value> const & /* serializer */) const
  {
    if (m_empty)
      return;
    WriteVarUint(sink, m_val);
  }

  template <typename Source>
  void Deserialize(Source & src, uint64_t valueCount, SingleValueSerializer<Value> const & /* serializer */)
  {
    CHECK_LESS_OR_EQUAL(valueCount, 1, ());
    m_empty = valueCount == 0;
    if (!m_empty)
      m_val = ReadVarUint<uint64_t>(src);
  }

  template <typename Source>
  void Deserialize(Source & src, SingleValueSerializer<Value> const & /* serializer */)
  {
    m_empty = src.Size() == 0;
    if (!m_empty)
      m_val = ReadVarUint<uint64_t>(src);
  }

  template <typename ToDo>
  void ForEach(ToDo && toDo) const
  {
    if (IsEmpty())
      return;
    toDo(Value(m_val));
  }

private:
  uint64_t m_val;
  bool m_empty = false;
};
