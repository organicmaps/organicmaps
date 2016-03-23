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
// NOTE: this class and its subclasses *ARE* thread-safe.
class FeaturesFilter
{
public:
  FeaturesFilter(coding::CompressedBitVector const & filter, uint32_t threshold);

  virtual ~FeaturesFilter() = default;

  bool NeedToFilter(coding::CompressedBitVector const & features) const;

  virtual unique_ptr<coding::CompressedBitVector> Filter(
      coding::CompressedBitVector const & cbv) const = 0;

protected:
  coding::CompressedBitVector const & m_filter;
  uint32_t const m_threshold;
};

// Exact filter - leaves only features belonging to the set it was
// constructed from.
class LocalityFilter : public FeaturesFilter
{
public:
  LocalityFilter(coding::CompressedBitVector const & filter);

  // FeaturesFilter overrides:
  unique_ptr<coding::CompressedBitVector> Filter(
      coding::CompressedBitVector const & cbv) const override;
};

// Fuzzy filter - tries to leave only features belonging to the set it
// was constructed from, but if the result is empty, leaves at most
// first |threshold| features instead. This property is quite useful
// when there are no matching features in viewport but it's ok to
// process a limited number of features outside the viewport.
class ViewportFilter : public FeaturesFilter
{
public:
  ViewportFilter(coding::CompressedBitVector const & filter, uint32_t threshold);

  // FeaturesFilter overrides:
  unique_ptr<coding::CompressedBitVector> Filter(
      coding::CompressedBitVector const & cbv) const override;
};
}  // namespace v2
}  // namespace search
