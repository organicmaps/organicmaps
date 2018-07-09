#pragma once

#include "coding/compressed_bit_vector.hpp"
#include "coding/geometry_coding.hpp"
#include "coding/read_write_utils.hpp"
#include "coding/write_to_sink.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

/// Following classes are supposed to be used with StringsFile. They
/// allow to write/read them, compare or serialize to an in-memory
/// buffer. The reason to use these classes instead of
/// buffer_vector<uint8_t, N> is that in some specific cases, like
/// compressed search index construction, they allow to avoid
/// redundant serialization-deserialization or sorting.

// A wrapper around feature index.
struct FeatureIndexValue
{
  FeatureIndexValue() = default;

  explicit FeatureIndexValue(uint64_t featureId) : m_featureId(featureId) {}

  bool operator<(FeatureIndexValue const & o) const { return m_featureId < o.m_featureId; }

  bool operator==(FeatureIndexValue const & o) const { return m_featureId == o.m_featureId; }

  void Swap(FeatureIndexValue & o) { swap(m_featureId, o.m_featureId); }

  uint64_t m_featureId = 0;
};

namespace std
{
template <>
struct hash<FeatureIndexValue>
{
public:
  size_t operator()(FeatureIndexValue const & value) const
  {
    return std::hash<uint64_t>{}(value.m_featureId);
  }
};
}  // namespace std

struct FeatureWithRankAndCenter
{
  FeatureWithRankAndCenter() = default;

  FeatureWithRankAndCenter(m2::PointD const & pt, uint32_t featureId, uint8_t rank)
    : m_pt(pt), m_featureId(featureId), m_rank(rank)
  {
  }

  bool operator<(FeatureWithRankAndCenter const & o) const { return m_featureId < o.m_featureId; }

  bool operator==(FeatureWithRankAndCenter const & o) const { return m_featureId == o.m_featureId; }

  void Swap(FeatureWithRankAndCenter & o)
  {
    swap(m_pt, o.m_pt);
    swap(m_featureId, o.m_featureId);
    swap(m_rank, o.m_rank);
  }

  m2::PointD m_pt;           // Center point of the feature.
  uint32_t m_featureId = 0;  // Feature identifier.
  uint8_t m_rank = 0;        // Rank of the feature.
};

namespace std
{
template <>
struct hash<FeatureWithRankAndCenter>
{
public:
  size_t operator()(FeatureWithRankAndCenter const & value) const
  {
    return std::hash<uint64_t>{}(value.m_featureId);
  }
};
}  // namespace std

template <typename Value>
class SingleValueSerializer;

template <>
class SingleValueSerializer<FeatureWithRankAndCenter>
{
public:
  using Value = FeatureWithRankAndCenter;

  SingleValueSerializer(serial::GeometryCodingParams const & codingParams)
    : m_codingParams(codingParams)
  {
  }

  template <typename Sink>
  void Serialize(Sink & sink, Value const & v) const
  {
    serial::SavePoint(sink, v.m_pt, m_codingParams);
    WriteToSink(sink, v.m_featureId);
    WriteToSink(sink, v.m_rank);
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
    v.m_pt = serial::LoadPoint(source, m_codingParams);
    v.m_featureId = ReadPrimitiveFromSource<uint32_t>(source);
    v.m_rank = ReadPrimitiveFromSource<uint8_t>(source);
  }

private:
  serial::GeometryCodingParams m_codingParams;
};

template <>
class SingleValueSerializer<FeatureIndexValue>
{
public:
  using Value = FeatureIndexValue;

  SingleValueSerializer() = default;

  // todo(@mpimenov). Remove.
  SingleValueSerializer(serial::GeometryCodingParams const & /* codingParams */) {}

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
template <typename TValue>
class ValueList;

// ValueList<FeatureIndexValue> serializes a group of feature
// indices as a compressed bit vector.
template <>
class ValueList<FeatureIndexValue>
{
public:
  using Value = FeatureIndexValue;

  ValueList() = default;

  ValueList(ValueList<FeatureIndexValue> const & o)
  {
    if (o.m_cbv)
      m_cbv = o.m_cbv->Clone();
  }

  void Init(std::vector<FeatureIndexValue> const & values)
  {
    std::vector<uint64_t> ids(values.size());
    for (size_t i = 0; i < ids.size(); ++i)
      ids[i] = values[i].m_featureId;
    m_cbv = coding::CompressedBitVectorBuilder::FromBitPositions(move(ids));
  }

  // This method returns number of values in the current instance of
  // ValueList<FeatureIndexValue>, but as these values are actually
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
  void Deserialize(Source & src, uint32_t valueCount,
                   SingleValueSerializer<Value> const & /* serializer */)
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
    coding::CompressedBitVectorEnumerator::ForEach(
        *m_cbv, [&toDo](uint64_t const bitPosition) { toDo(Value(bitPosition)); });
  }

private:
  std::unique_ptr<coding::CompressedBitVector> m_cbv;
};

/// ValueList<FeatureWithRankAndCenter> sequentially serializes
/// encoded features infos.
template <>
class ValueList<FeatureWithRankAndCenter>
{
public:
  using Value = FeatureWithRankAndCenter;
  using Serializer = SingleValueSerializer<Value>;

  void Init(std::vector<Value> const & values) { m_values = values; }

  size_t Size() const { return m_values.size(); }

  bool IsEmpty() const { return m_values.empty(); }

  template <typename Sink>
  void Serialize(Sink & sink, SingleValueSerializer<Value> const & serializer) const
  {
    for (auto const & value : m_values)
      serializer.Serialize(sink, value);
  }

  template <typename Source>
  void Deserialize(Source & src, uint32_t valueCount,
                   SingleValueSerializer<Value> const & serializer)
  {
    m_values.resize(valueCount);
    for (size_t i = 0; i < valueCount; ++i)
      serializer.DeserializeFromSource(src, m_values[i]);
  }

  // When valueCount is not known, Deserialize reads
  // until the source is exhausted.
  template <typename Source>
  void Deserialize(Source & src, SingleValueSerializer<Value> const & serializer)
  {
    m_values.clear();
    while (src.Size() > 0)
    {
      m_values.emplace_back();
      serializer.DeserializeFromSource(src, m_values.back());
    }
  }

  template <typename ToDo>
  void ForEach(ToDo && toDo) const
  {
    for (auto const & value : m_values)
      toDo(value);
  }

private:
  std::vector<Value> m_values;
};
