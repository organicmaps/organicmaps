#pragma once

#include "coding/compressed_bit_vector.hpp"
#include "coding/read_write_utils.hpp"
#include "coding/write_to_sink.hpp"

#include "base/assert.hpp"

#include "std/algorithm.hpp"

/// Following classes are supposed to be used with StringsFile. They
/// allow to write/read them, compare or serialize to an in-memory
/// buffer. The reason to use these classes instead of
/// buffer_vector<uint8_t, N> is that in some specific cases, like
/// compressed search index construction, they allow to avoid
/// redundant serialization-deserialization or sorting.

/// A wrapper around feature index.
struct FeatureIndexValue
{
  FeatureIndexValue() : m_value(0) {}

  template <typename TWriter>
  void Write(TWriter & writer) const
  {
    WriteToSink(writer, m_value);
  }

  template <typename TReader>
  void Read(TReader & reader)
  {
    m_value = ReadPrimitiveFromSource<uint32_t>(reader);
  }

  inline void const * data() const { return &m_value; }

  inline size_t size() const { return sizeof(m_value); }

  bool operator<(FeatureIndexValue const & value) const { return m_value < value.m_value; }

  bool operator==(FeatureIndexValue const & value) const { return m_value == value.m_value; }

  void swap(FeatureIndexValue & value) { ::swap(m_value, value.m_value); }

  uint32_t m_value;
};

/// A wrapper around serialized SaverT::ValueType.
struct SerializedFeatureInfoValue
{
  using ValueT = buffer_vector<uint8_t, 32>;

  template <typename TWriter>
  void Write(TWriter & writer) const
  {
    rw::WriteVectorOfPOD(writer, m_value);
  }

  template <typename TReader>
  void Read(TReader & reader)
  {
    rw::ReadVectorOfPOD(reader, m_value);
  }

  inline void const * data() const { return m_value.data(); }

  inline size_t size() const { return m_value.size() * sizeof(ValueT::value_type); }

  bool operator<(SerializedFeatureInfoValue const & value) const { return m_value < value.m_value; }

  bool operator==(SerializedFeatureInfoValue const & value) const
  {
    return m_value == value.m_value;
  }

  void swap(SerializedFeatureInfoValue & value) { m_value.swap(value.m_value); }

  ValueT m_value;
};

/// This template is used to accumulate and serialize a group of
/// values of the same type.
template <typename Value>
class ValueList;

/// ValueList<FeatureIndexValue> serializes a group of features
/// indices as a compressed bit vector, thus, allowing us to save a
/// disk space.
template <>
class ValueList<FeatureIndexValue>
{
public:
  void Append(FeatureIndexValue const & value)
  {
    // External-memory trie adds <string, value> pairs in a sorted
    // order, thus, values are supposed to be accumulated in a
    // sorted order, and we can avoid sorting them before construction
    // of a CompressedBitVector.
    ASSERT(m_offsets.empty() || m_offsets.back() <= value.m_value, ());
    if (!m_offsets.empty() && m_offsets.back() == value.m_value)
      return;
    m_offsets.push_back(value.m_value);
  }

  /// This method returns number of values in the current instance of
  /// ValueList<FeatureIndexValue>, but as these values are actually
  /// features indices and can be dumped as a single serialized
  /// compressed bit vector, this method returns 1 when there're at
  /// least one feature's index in the list - so, compressed bit
  /// vector will be built and serialized - and 0 otherwise.
  size_t size() const { return m_offsets.empty() ? 0 : 1; }

  bool empty() const { return m_offsets.empty(); }

  template <typename SinkT>
  void Dump(SinkT & sink) const
  {
    vector<uint8_t> buffer;
    MemWriter<vector<uint8_t>> writer(buffer);
    BuildCompressedBitVector(writer, m_offsets);
    sink.Write(buffer.data(), buffer.size());
  }

private:
  vector<uint32_t> m_offsets;
};

/// ValueList<SerializedFeatureInfoValue> sequentially serializes
/// encoded features infos.
template <>
class ValueList<SerializedFeatureInfoValue>
{
public:
  ValueList() : m_size(0) {}

  void Append(SerializedFeatureInfoValue const & value)
  {
    m_value.insert(m_value.end(), value.m_value.begin(), value.m_value.end());
    ++m_size;
  }

  size_t size() const { return m_size; }

  bool empty() const { return !m_size; }

  template <typename SinkT>
  void Dump(SinkT & sink) const
  {
    sink.Write(m_value.data(), m_value.size());
  }

private:
  buffer_vector<uint8_t, 32> m_value;
  uint32_t m_size;
};
