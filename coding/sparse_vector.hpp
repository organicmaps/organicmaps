#pragma once

#include "3party/succinct/rs_bit_vector.hpp"

namespace coding
{

/// Simple sparse vector for better memory usage.
template <class Value>
class SparseVector
{
  succinct::rs_bit_vector m_bits;
  std::vector<Value> m_values;

public:
  SparseVector() = default;

  bool Empty() const { return GetSize() == 0; }
  uint64_t GetSize() const { return m_bits.size(); }
  bool Has(uint64_t i) const { return m_bits[i]; }
  Value const & Get(uint64_t i) const { return m_values[m_bits.rank(i)]; }

  size_t GetMemorySize() const { return m_values.capacity() * sizeof(Value) + m_bits.size() / 8; }

private:
  SparseVector(std::vector<Value> && values, succinct::bit_vector_builder & bitsBuilder)
    : m_bits(&bitsBuilder)
    , m_values(std::move(values))
  {}

  template <class T>
  friend class SparseVectorBuilder;
};

template <class Value>
class SparseVectorBuilder
{
  succinct::bit_vector_builder m_bitsBuilder;
  std::vector<Value> m_values;

public:
  explicit SparseVectorBuilder(size_t reserveSize = 0) { m_bitsBuilder.reserve(reserveSize); }

  void PushEmpty() { m_bitsBuilder.push_back(false); }
  void PushValue(Value v)
  {
    m_bitsBuilder.push_back(true);
    m_values.emplace_back(std::move(v));
  }

  SparseVector<Value> Build()
  {
    // Shrink memory as a primary goal of this component.
    if (m_values.capacity() / double(m_values.size()) > 1.4)
      std::vector<Value>(m_values).swap(m_values);

    return SparseVector<Value>(std::move(m_values), m_bitsBuilder);
  }
};

}  // namespace coding
