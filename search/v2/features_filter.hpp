#pragma once

#include "coding/compressed_bit_vector.hpp"

#include "std/unique_ptr.hpp"

namespace search
{
namespace v2
{
// A lightweight filter of features.
//
// NOTE: this class *IS NOT* thread-safe.
class FeaturesFilter
{
public:
  FeaturesFilter();

  FeaturesFilter(unique_ptr<coding::CompressedBitVector> filter, uint32_t threshold);

  inline void SetFilter(unique_ptr<coding::CompressedBitVector> filter) { m_filter = move(filter); }
  inline void SetThreshold(uint32_t threshold) { m_threshold = threshold; }

  bool NeedToFilter(coding::CompressedBitVector & features) const;
  unique_ptr<coding::CompressedBitVector> Filter(coding::CompressedBitVector & cbv) const;

private:
  unique_ptr<coding::CompressedBitVector> m_filter;
  uint32_t m_threshold;
};
}  // namespace v2
}  // namespace search
