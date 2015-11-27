#pragma once

#include "coding/compressed_bit_vector.hpp"
#include "coding/read_write_utils.hpp"
#include "coding/write_to_sink.hpp"

#include "indexer/coding_params.hpp"
#include "indexer/geometry_serialization.hpp"

#include "platform/mwm_version.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"

#include "std/algorithm.hpp"
#include "std/unique_ptr.hpp"
#include "std/utility.hpp"

/// Following classes are supposed to be used with StringsFile. They
/// allow to write/read them, compare or serialize to an in-memory
/// buffer. The reason to use these classes instead of
/// buffer_vector<uint8_t, N> is that in some specific cases, like
/// compressed search index construction, they allow to avoid
/// redundant serialization-deserialization or sorting.

// A wrapper around feature index.
struct FeatureIndexValue
{
  FeatureIndexValue() : m_featureId(0) {}

  FeatureIndexValue(uint64_t featureId) : m_featureId(featureId) {}

  bool operator<(FeatureIndexValue const & o) const { return m_featureId < o.m_featureId; }

  bool operator==(FeatureIndexValue const & o) const { return m_featureId == o.m_featureId; }

  void Swap(FeatureIndexValue & o) { swap(m_featureId, o.m_featureId); }

  uint64_t m_featureId;
};

struct FeatureWithRankAndCenter
{
  FeatureWithRankAndCenter() : m_pt(m2::PointD()), m_featureId(0), m_rank(0) {}

  FeatureWithRankAndCenter(m2::PointD pt, uint32_t featureId, uint8_t rank)
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

  m2::PointD m_pt;       // Center point of the feature.
  uint32_t m_featureId;  // Feature identifier.
  uint8_t m_rank;        // Rank of the feature.
};

template <typename TValue>
class SingleValueSerializer;

template <>
class SingleValueSerializer<FeatureWithRankAndCenter>
{
public:
  using TValue = FeatureWithRankAndCenter;

  SingleValueSerializer(serial::CodingParams const & codingParams) : m_codingParams(codingParams) {}

  template <typename TWriter>
  void Serialize(TWriter & writer, TValue const & v) const
  {
    serial::SavePoint(writer, v.m_pt, m_codingParams);
    WriteToSink(writer, v.m_featureId);
    WriteToSink(writer, v.m_rank);
  }

  template <typename TReader>
  void Deserialize(TReader & reader, TValue & v) const
  {
    ReaderSource<TReader> src(reader);
    DeserializeFromSource(src, v);
  }

  template <typename TSource>
  void DeserializeFromSource(TSource & src, TValue & v) const
  {
    v.m_pt = serial::LoadPoint(src, m_codingParams);
    v.m_featureId = ReadPrimitiveFromSource<uint32_t>(src);
    v.m_rank = ReadPrimitiveFromSource<uint8_t>(src);
  }

private:
  serial::CodingParams m_codingParams;
};

template <>
class SingleValueSerializer<FeatureIndexValue>
{
public:
  using TValue = FeatureIndexValue;

  SingleValueSerializer() = default;

  // todo(@mpimenov). Remove.
  SingleValueSerializer(serial::CodingParams const & /* codingParams */) {}

  // The serialization and deserialization is needed for StringsFile.
  // Use ValueList for group serialization in CBVs.
  template <typename TWriter>
  void Serialize(TWriter & writer, TValue const & v) const
  {
    WriteToSink(writer, v.m_featureId);
  }

  template <typename TReader>
  void Deserialize(TReader & reader, TValue & v) const
  {
    ReaderSource<TReader> src(reader);
    DeserializeFromSource(src, v);
  }

  template <typename TSource>
  void DeserializeFromSource(TSource & src, TValue & v) const
  {
    v.m_featureId = ReadPrimitiveFromSource<uint64_t>(src);
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
  using TValue = FeatureIndexValue;

  ValueList() = default;

  ValueList(ValueList<FeatureIndexValue> const & o)
  {
    if (o.m_cbv)
      m_cbv = o.m_cbv->Clone();
  }

  void Init(vector<FeatureIndexValue> const & values)
  {
    vector<uint64_t> ids(values.size());
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

  template <typename TSink>
  void Serialize(TSink & sink, SingleValueSerializer<TValue> const & /* serializer */) const
  {
    if (IsEmpty())
      return;
    vector<uint8_t> buf;
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
  template <typename TSource>
  void Deserialize(TSource & src, uint32_t valueCount,
                   SingleValueSerializer<TValue> const & /* serializer */)
  {
    if (valueCount > 0)
      m_cbv = coding::CompressedBitVectorBuilder::DeserializeFromSource(src);
    else
      m_cbv.reset();
  }

  template <typename TSource>
  void Deserialize(TSource & src, SingleValueSerializer<TValue> const & /* serializer */)
  {
    if (src.Size() > 0)
      m_cbv = coding::CompressedBitVectorBuilder::DeserializeFromSource(src);
    else
      m_cbv.reset();
  }

  template <typename TF>
  void ForEach(TF && f) const
  {
    if (IsEmpty())
      return;
    coding::CompressedBitVectorEnumerator::ForEach(*m_cbv, [&f](uint64_t const bitPosition)
                                                   {
                                                     f(TValue(bitPosition));
                                                   });
  }

private:
  unique_ptr<coding::CompressedBitVector> m_cbv;
};

/// ValueList<FeatureWithRankAndCenter> sequentially serializes
/// encoded features infos.
template <>
class ValueList<FeatureWithRankAndCenter>
{
public:
  using TValue = FeatureWithRankAndCenter;
  using TSerializer = SingleValueSerializer<TValue>;

  void Init(vector<TValue> const & values) { m_values = values; }

  size_t Size() const { return m_values.size(); }

  bool IsEmpty() const { return m_values.empty(); }

  template <typename TSink>
  void Serialize(TSink & sink, SingleValueSerializer<TValue> const & serializer) const
  {
    for (auto const & value : m_values)
      serializer.Serialize(sink, value);
  }

  template <typename TSource>
  void Deserialize(TSource & src, uint32_t valueCount,
                   SingleValueSerializer<TValue> const & serializer)
  {
    m_values.resize(valueCount);
    for (size_t i = 0; i < valueCount; ++i)
      serializer.DeserializeFromSource(src, m_values[i]);
  }

  // When valueCount is not known, Deserialize reads
  // until the source is exhausted.
  template <typename TSource>
  void Deserialize(TSource & src, SingleValueSerializer<TValue> const & serializer)
  {
    m_values.clear();
    while (src.Size() > 0)
    {
      m_values.push_back(TValue());
      serializer.DeserializeFromSource(src, m_values.back());
    }
  }

  template <typename TF>
  void ForEach(TF && f) const
  {
    for (auto const & value : m_values)
      f(value);
  }

private:
  vector<TValue> m_values;
};

// This is a wrapper around mwm's version.
// Search index does not have a separate version and
// derives its format from the version of its mwm.
// For now, the class provides a more readable way to
// find out what is stored in the search index than by
// staring at the mwm version number. Similar functionality
// may be added to it later.
class MwmTraits
{
public:
  enum class SearchIndexFormat
  {
    // A list of features with their ranks and centers
    // is stored behind every node of the search trie.
    // This format corresponds to ValueList<FeatureWithRankAndCenter>.
    FeaturesWithRankAndCenter,

    // A compressed bit vector of feature indices is
    // stored behind every node of the search trie.
    // This format corresponds to ValueList<FeatureIndexValue>.
    CompressedBitVector,

    // The format of the search index is unknown. Most
    // likely, an error has occured.
    Unknown
  };

  MwmTraits(version::Format versionFormat)
  {
    if (versionFormat < version::v7)
    {
      m_format = SearchIndexFormat::FeaturesWithRankAndCenter;
    }
    else if (versionFormat == version::v7)
    {
      m_format = SearchIndexFormat::CompressedBitVector;
    }
    else
    {
      LOG(LWARNING, ("Unknown search index format."));
      m_format = SearchIndexFormat::Unknown;
    }
  }

  SearchIndexFormat GetSearchIndexFormat() const { return m_format; }

private:
  SearchIndexFormat m_format;
};
