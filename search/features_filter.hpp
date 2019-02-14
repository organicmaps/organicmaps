#pragma once

#include <cstdint>

namespace search
{
class CBV;

// A lightweight filter of features.
//
// NOTE: this class and its subclasses *ARE* thread-safe.
class FeaturesFilter
{
public:
  FeaturesFilter(CBV const & filter, uint64_t threshold);

  virtual ~FeaturesFilter() = default;

  bool NeedToFilter(CBV const & features) const;

  virtual CBV Filter(CBV const & cbv) const = 0;

protected:
  CBV const & m_filter;
  uint64_t const m_threshold;
};

// Exact filter - leaves only features belonging to the set it was
// constructed from.
class LocalityFilter : public FeaturesFilter
{
public:
  LocalityFilter(CBV const & filter);

  // FeaturesFilter overrides:
  CBV Filter(CBV const & cbv) const override;
};

// Fuzzy filter - tries to leave only features belonging to the set it
// was constructed from, but if the result is empty, leaves at most
// first |threshold| features instead. This property is quite useful
// when there are no matching features in viewport but it's ok to
// process a limited number of features outside the viewport.
class ViewportFilter : public FeaturesFilter
{
public:
  ViewportFilter(CBV const & filter, uint64_t threshold);

  // FeaturesFilter overrides:
  CBV Filter(CBV const & cbv) const override;
};
}  // namespace search
