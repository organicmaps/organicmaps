#pragma once

#include "std/unique_ptr.hpp"

namespace coding
{
class CompressedBitVector;
}

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

  FeaturesFilter(coding::CompressedBitVector const & filter, uint32_t threshold);

  inline void SetFilter(coding::CompressedBitVector const * filter) { m_filter = filter; }

  inline void SetThreshold(uint32_t threshold) { m_threshold = threshold; }

  bool NeedToFilter(coding::CompressedBitVector const & features) const;
  unique_ptr<coding::CompressedBitVector> Filter(coding::CompressedBitVector const & cbv) const;

private:
  // Non-owning ptr.
  coding::CompressedBitVector const * m_filter;
  uint32_t m_threshold;
};

}  // namespace v2
}  // namespace search
