#pragma once

#include "coding/compressed_bit_vector.hpp"
#include "coding/write_to_sink.hpp"

#include "base/assert.hpp"

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

  SingleValueSerializer() = default;

  // The serialization and deserialization is needed for StringsFile.
  // Use ValueList for group serialization in CBVs.
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
};

// This template is used to accumulate, serialize and deserialize
// a group of values of the same type.
template <typename Value>
class ValueList;

// ValueList<Uint64IndexValue> serializes a group of feature
// indices as a compressed bit vector.
template <>
class ValueList<Uint64IndexValue>
{
public:
  using Value = Uint64IndexValue;

  ValueList() = default;

  ValueList(ValueList<Uint64IndexValue> const & o)
  {
    if (o.m_cbv)
      m_cbv = o.m_cbv->Clone();
  }

  void Init(std::vector<Uint64IndexValue> const & values)
  {
    std::vector<uint64_t> ids(values.size());
    for (size_t i = 0; i < ids.size(); ++i)
      ids[i] = values[i].m_featureId;
    m_cbv = coding::CompressedBitVectorBuilder::FromBitPositions(std::move(ids));
  }

  // This method returns number of values in the current instance of
  // ValueList<Uint64IndexValue>, but as these values are actually
  // features indices and can be dumped as a single serialized
  // compressed bit vector, this method returns 1 when there're at
  // least one feature's index in the list - so, compressed bit
  // vector will be built and serialized - and 0 otherwise.
  size_t Size() const { return (m_cbv && m_cbv->PopCount() != 0) ? 1 : 0; }

  bool IsEmpty() const { return Size() == 0; }

  template <typename Sink>
  void Serialize(Sink & sink, SingleValueSerializer<Value> const & /* serializer */) const
  {
    if (IsEmpty())
      return;
    std::vector<uint8_t> buf;
    MemWriter<decltype(buf)> writer(buf);
    m_cbv->Serialize(writer);
    sink.Write(buf.data(), buf.size());
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
  void Deserialize(Source & src, uint32_t valueCount, SingleValueSerializer<Value> const & /* serializer */)
  {
    if (valueCount > 0)
      m_cbv = coding::CompressedBitVectorBuilder::DeserializeFromSource(src);
    else
      m_cbv.reset();
  }

  template <typename Source>
  void Deserialize(Source & src, SingleValueSerializer<Value> const & /* serializer */)
  {
    if (src.Size() > 0)
      m_cbv = coding::CompressedBitVectorBuilder::DeserializeFromSource(src);
    else
      m_cbv.reset();
  }

  template <typename ToDo>
  void ForEach(ToDo && toDo) const
  {
    if (IsEmpty())
      return;
    coding::CompressedBitVectorEnumerator::ForEach(*m_cbv,
                                                   [&toDo](uint64_t const bitPosition) { toDo(Value(bitPosition)); });
  }

private:
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
