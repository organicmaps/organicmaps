#pragma once

#include "coding/file_writer.hpp"
#include "coding/succinct_mapper.hpp"

#include "base/assert.hpp"

#include "3party/succinct/elias_fano.hpp"
#include "3party/succinct/rs_bit_vector.hpp"

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace idx::v2
{
// A succinct structure may follow other data: its alignment is relative to its own start.
template <class TWriter>
class PayloadWriter
{
public:
  explicit PayloadWriter(TWriter & writer) : m_writer(writer) {}
  uint64_t Pos() const { return m_written; }
  void Write(void const * data, size_t size)
  {
    m_writer.Write(data, size);
    m_written += size;
  }

private:
  TWriter & m_writer;
  uint64_t m_written = 0;
};

template <class T, class TWriter>
void Freeze(T & value, TWriter & writer)
{
  PayloadWriter<TWriter> payload(writer);
  coding::Freeze(value, payload, "idx");
}

// The vectors below are the succinct structures over succinct::mapper::dd_vector: they are not
// loaded into memory, they read the words they need from the reader, which has to outlive them.
// So they work with any reader. With a reader of memory they are as fast as in a buffer, with
// another one, of a file, every dd_vector keeps a small window of the last read words.

/// Bits vector with rank and []. For a fast select, build it with the select hints.
template <class TReader>
class RsBitsVector
{
public:
  using size_type = uint64_t;
  using value_type = bool;

  explicit RsBitsVector(TReader const & reader) { coding::ReaderMap(m_bits, reader); }

  bool operator[](size_type idx) const { return m_bits[idx]; }
  bool get(size_type idx) const { return m_bits[idx]; }
  auto size() const { return m_bits.size(); }
  auto num_ones() const { return m_bits.num_ones(); }
  auto rank(size_type idx) const { return m_bits.rank(idx); }

  auto select(size_type idx) const { return m_bits.select(idx); }
  auto select_range(size_type idx) const { return std::make_pair(m_bits.select(idx), m_bits.select(idx + 1)); }

private:
  succinct::rs_bit_vector_t<succinct::mapper::dd_vector<uint64_t, TReader>> m_bits;
};

class RsBitsVectorBuilder
{
public:
  using size_type = uint64_t;
  template <class TReader>
  using ContainerT = RsBitsVector<TReader>;

  explicit RsBitsVectorBuilder(bool selectHints = false) : m_selectHints(selectHints) {}
  explicit RsBitsVectorBuilder(size_type count, bool selectHints = false)
    : m_bits(count, false)
    , m_selectHints(selectHints)
  {}

  /// For a builder made with the count.
  void Add(size_type idx, bool value) { m_bits[idx] = value; }
  bool Get(size_type idx) const { return m_bits[idx]; }

  /// For a builder made without the count.
  void PushBack(bool value) { m_bits.push_back(value); }

  uint64_t Size() const { return m_bits.size(); }

  template <class TWriter>
  void Flush(TWriter & writer)
  {
    succinct::rs_bit_vector cont(m_bits, m_selectHints);
    Freeze(cont, writer);
  }

  void Flush(std::string const & path)
  {
    FileWriter writer(path);
    Flush(writer);
  }

private:
  std::vector<bool> m_bits;
  bool m_selectHints;
};

/// Non-decreasing numbers as an Elias-Fano bits vector: select(i) is the i-th number.
template <class TReader>
class EfBitsVector
{
public:
  using size_type = uint64_t;
  using value_type = uint64_t;

  explicit EfBitsVector(TReader const & reader) { coding::ReaderMap(m_bits, reader); }

  auto size() const { return m_bits.size(); }
  auto num_ones() const { return m_bits.num_ones(); }

  auto select(size_type idx) const { return m_bits.select(idx); }
  auto select_range(size_type idx) const { return m_bits.select_range(idx); }

private:
  succinct::elias_fano_t<succinct::mapper::dd_vector<uint64_t, TReader>> m_bits;
};

class EfBitsVectorBuilder
{
public:
  using value_type = uint64_t;
  using size_type = uint64_t;
  template <class TReader>
  using ContainerT = EfBitsVector<TReader>;

  explicit EfBitsVectorBuilder(size_type count = 0) : m_values(count) {}

  /// For a builder made with the count.
  void Add(size_type idx, value_type value) { m_values[idx] = value; }

  /// For a builder made without the count.
  void PushBack(value_type value) { m_values.push_back(value); }

  size_t Size() const { return m_values.size(); }

  template <class TWriter>
  void Flush(TWriter & writer)
  {
    ASSERT(!m_values.empty(), ());

    succinct::elias_fano::elias_fano_builder builder(m_values.back(), m_values.size());
    for (auto const value : m_values)
      builder.push_back(value);

    // Not optimized for rank.
    succinct::elias_fano cont(&builder, false /* with_rank_index */);
    Freeze(cont, writer);
  }

  void Flush(std::string const & path)
  {
    FileWriter writer(path);
    Flush(writer);
  }

private:
  std::vector<uint64_t> m_values;
};
}  // namespace idx::v2
