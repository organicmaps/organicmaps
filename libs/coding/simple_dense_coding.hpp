#pragma once

#include <cstdint>
#include <vector>

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-private-field"
#endif

#include "3party/succinct/elias_fano_compressed_list.hpp"

#if defined(__clang__)
#pragma clang diagnostic pop
#endif

namespace coding
{
// This class represents a variant of a so-called simple dense coding
// scheme for byte strings.  It can be used when it's necessary to
// compress strings with skewed entropy and nevertheless efficient
// access to the string's elements is needed.
//
// The main idea is to assign codewords from the set { 0, 1, 00, 01,
// 10, 11, 000, ... } to string's symbols in accordance with their
// frequencies and to create a helper bit-vector for starting
// positions of the codewords in compressed string.
//
// Memory complexity: 2 * n * (H_0(T) + 1) bits for a text T, but note
// that this is an upper bound and too pessimistic.

// Time complexity: O(log(n * H_0(T))) to access i-th element of the
// string, because of logarithmic complexity of
// rs_bit_vector::select. This will be fixed when RRR will be
// implemented.
//
// For details, see Kimmo Fredriksson, Fedor Nikitin, "Simple Random
// Access Compression", Fundamenta Informaticae 2009,
// http://www.cs.uku.fi/~fredriks/pub/papers/fi09.pdf.
class SimpleDenseCoding
{
public:
  SimpleDenseCoding() = default;

  SimpleDenseCoding(std::vector<uint8_t> const & data);

  SimpleDenseCoding(SimpleDenseCoding && rhs);

  uint8_t Get(uint64_t i) const;

  inline uint64_t Size() const { return m_ranks.size(); }

  // map is used here (instead of Map) for compatibility with succinct
  // structures.
  template <typename TVisitor>
  void map(TVisitor & visitor)
  {
    visitor(m_ranks, "m_ranks");
    visitor(m_symbols, "m_symbols");
  }

private:
  succinct::elias_fano_compressed_list m_ranks;
  succinct::mapper::mappable_vector<uint8_t> m_symbols;
};
}  // namespace coding
