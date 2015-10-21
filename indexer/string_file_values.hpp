#pragma once

#include "coding/compressed_bit_vector.hpp"
#include "coding/read_write_utils.hpp"
#include "coding/write_to_sink.hpp"

#include "indexer/coding_params.hpp"
#include "indexer/geometry_serialization.hpp"

#include "base/assert.hpp"

#include "std/algorithm.hpp"
#include "std/unique_ptr.hpp"
#include "std/utility.hpp"

/// Following classes are supposed to be used with StringsFile. They
/// allow to write/read them, compare or serialize to an in-memory
/// buffer. The reason to use these classes instead of
/// buffer_vector<uint8_t, N> is that in some specific cases, like
/// compressed search index construction, they allow to avoid
/// redundant serialization-deserialization or sorting.

/// A wrapper around feature index.
struct FeatureIndexValue
{
  FeatureIndexValue() : m_featureId(0) {}

  FeatureIndexValue(uint64_t featureId) : m_featureId(featureId) {}

  // The serialization and deserialization is needed for StringsFile.
  // Use ValueList for group serialization in CBVs.
  template <typename TWriter>
  void Serialize(TWriter & writer) const
  {
    WriteToSink(writer, m_featureId);
  }

  template <typename TReader>
  void Deserialize(TReader & reader)
  {
    ReaderSource<TReader> src(reader);
    DeserializeFromSource(src);
  }

  template <typename TSource>
  void DeserializeFromSource(TSource & src)
  {
    m_featureId = ReadPrimitiveFromSource<uint64_t>(src);
  }

  inline void const * data() const { return &m_featureId; }

  inline size_t size() const { return sizeof(m_featureId); }

  bool operator<(FeatureIndexValue const & o) const { return m_featureId < o.m_featureId; }

  bool operator==(FeatureIndexValue const & o) const { return m_featureId == o.m_featureId; }

  void Swap(FeatureIndexValue & o) { ::swap(m_featureId, o.m_featureId); }

  uint64_t m_featureId;
};

struct FeatureWithRankAndCenter
{
  FeatureWithRankAndCenter() = default;

  FeatureWithRankAndCenter(m2::PointD pt, uint32_t featureId, uint8_t rank,
                           serial::CodingParams codingParams)
    : m_pt(pt), m_featureId(featureId), m_rank(rank), m_codingParams(codingParams)
  {
  }

  template <typename TWriter>
  void Serialize(TWriter & writer) const
  {
    serial::SavePoint(writer, m_pt, m_codingParams);
    WriteToSink(writer, m_featureId);
    WriteToSink(writer, m_rank);
  }

  template <typename TReader>
  void Deserialize(TReader & reader)
  {
    ReaderSource<TReader> src(reader);
    DeserializeFromSource(src);
  }

  template <typename TSource>
  void DeserializeFromSource(TSource & src)
  {
    m_pt = serial::LoadPoint(src, m_codingParams);
    m_featureId = ReadPrimitiveFromSource<uint32_t>(src);
    m_rank = ReadPrimitiveFromSource<uint8_t>(src);
  }

  bool operator<(FeatureWithRankAndCenter const & o) const { return m_featureId < o.m_featureId; }

  bool operator==(FeatureWithRankAndCenter const & o) const { return m_featureId == o.m_featureId; }

  void Swap(FeatureWithRankAndCenter & o)
  {
    swap(m_pt, o.m_pt);
    swap(m_featureId, o.m_featureId);
    swap(m_rank, o.m_rank);
  }

  void SetCodingParams(serial::CodingParams const & codingParams) { m_codingParams = codingParams; }

  m2::PointD m_pt;       // Center point of the feature.
  uint32_t m_featureId;  // Offset of the feature.
  uint8_t m_rank;        // Rank of the feature.
  serial::CodingParams m_codingParams;
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
  using TValue = FeatureIndexValue;

  ValueList() : m_cbv(unique_ptr<coding::CompressedBitVector>()) {}

  ValueList(ValueList<FeatureIndexValue> const & o) : m_codingParams(o.m_codingParams)
  {
    if (o.m_cbv)
      m_cbv = coding::CompressedBitVectorBuilder::FromCBV(*o.m_cbv);
    else
      m_cbv = unique_ptr<coding::CompressedBitVector>();
  }

  void Init(vector<FeatureIndexValue> const & values)
  {
    vector<uint64_t> ids(values.size());
    for (size_t i = 0; i < ids.size(); ++i)
      ids[i] = values[i].m_featureId;
    m_cbv = coding::CompressedBitVectorBuilder::FromBitPositions(ids);
  }

  // This method returns number of values in the current instance of
  // ValueList<FeatureIndexValue>, but as these values are actually
  // features indices and can be dumped as a single serialized
  // compressed bit vector, this method returns 1 when there're at
  // least one feature's index in the list - so, compressed bit
  // vector will be built and serialized - and 0 otherwise.
  size_t Size() const { return m_cbv->PopCount() == 0 ? 0 : 1; }

  bool IsEmpty() const { return m_cbv->PopCount(); }

  template <typename TSink>
  void Serialize(TSink & sink) const
  {
    vector<uint8_t> buf;
    MemWriter<vector<uint8_t>> writer(buf);
    m_cbv->Serialize(writer);
    sink.Write(buf.data(), buf.size());
  }

  // Note the default parameter. It is here for compatibility with
  // an old data format that was serializing FeatureWithRankAndCenter`s.
  // They were put in a vector, this vector's size was encoded somehow
  // and then the vector was written with a method similar to Serialize above.
  // The deserialization code read the valueCount separately and then
  // read each FeatureWithRankAndCenter one by one.
  // A newer approach is to make Serialize/Deserialize responsible for
  // every part of serialization and as such it does not need valueCount.
  template <typename TSource>
  void Deserialize(TSource & src, uint32_t valueCount = 0)
  {
    m_cbv = coding::CompressedBitVectorBuilder::DeserializeFromSource(src);
  }

  template <typename TF>
  void ForEach(TF && f) const
  {
    coding::CompressedBitVectorEnumerator::ForEach(*m_cbv, [&](uint64_t const bitPosition)
                                                   {
                                                     f(TValue(bitPosition));
                                                   });
  }

  void SetCodingParams(serial::CodingParams const & codingParams) { m_codingParams = codingParams; }

private:
  unique_ptr<coding::CompressedBitVector> m_cbv;
  serial::CodingParams m_codingParams;
};

/// ValueList<FeatureWithRankAndCenter> sequentially serializes
/// encoded features infos.
template <>
class ValueList<FeatureWithRankAndCenter>
{
public:
  using TValue = FeatureWithRankAndCenter;

  ValueList() = default;
  ValueList(serial::CodingParams const & codingParams) : m_codingParams(codingParams) {}

  void Init(vector<TValue> const & values) { m_values = values; }

  size_t Size() const { return m_values.size(); }

  bool IsEmpty() const { return m_values.empty(); }

  template <typename TSink>
  void Serialize(TSink & sink) const
  {
    for (auto const & value : m_values)
      value.Serialize(sink);
  }

  template <typename TSource>
  void Deserialize(TSource & src, uint32_t valueCount)
  {
    m_values.resize(valueCount);
    for (size_t i = 0; i < valueCount; ++i)
      m_values[i].DeserializeFromSource(src);
  }

  // When valueCount is not known, Deserialize reads
  // until the source is exhausted.
  template <typename TSource>
  void Deserialize(TSource & src)
  {
    uint32_t const size = static_cast<uint32_t>(src.Size());
    while (src.Pos() < size)
    {
#ifdef DEBUG
      uint64_t const pos = src.Pos();
#endif
      m_values.push_back(TValue());
      m_values.back().DeserializeFromSource(src);
      ASSERT_NOT_EQUAL(pos, src.Pos(), ());
    }
    ASSERT_EQUAL(size, src.Pos(), ());
  }

  template <typename TF>
  void ForEach(TF && f) const
  {
    for (auto const & value : m_values)
      f(value);
  }

  void SetCodingParams(serial::CodingParams const & codingParams) { m_codingParams = codingParams; }

private:
  vector<TValue> m_values;
  serial::CodingParams m_codingParams;
};
