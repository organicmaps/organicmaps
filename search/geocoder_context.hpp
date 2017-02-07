#pragma once

#include "search/cbv.hpp"
#include "search/features_layer.hpp"
#include "search/geocoder_locality.hpp"
#include "search/hotels_filter.hpp"

#include <cstddef>
#include <memory>
#include <vector>

namespace search
{
class FeaturesFilter;
class TokenRange;

struct BaseContext
{
  // Advances |curToken| to the nearest unused token, or to the end of
  // |m_usedTokens| if there are no unused tokens.
  size_t SkipUsedTokens(size_t curToken) const;

  // Returns true iff all tokens are used.
  bool AllTokensUsed() const;

  // Returns true if there exists at least one used token in |range|.
  bool HasUsedTokensInRange(TokenRange const & range) const;

  // Counts number of groups of consecutive unused tokens.
  size_t NumUnusedTokenGroups() const;

  // List of bit-vectors of features, where i-th element of the list
  // corresponds to the i-th token in the search query.
  std::vector<CBV> m_features;
  CBV m_villages;
  CBV m_streets;

  // Stack of layers filled during geocoding.
  std::vector<FeaturesLayer> m_layers;

  // Stack of regions filled during geocoding.
  std::vector<Region const *> m_regions;

  City const * m_city = nullptr;

  // This vector is used to indicate what tokens were already matched
  // and can't be re-used during the geocoding process.
  std::vector<bool> m_usedTokens;

  // Number of tokens in the query.
  size_t m_numTokens = 0;

  std::unique_ptr<hotels_filter::HotelsFilter::ScopedFilter> m_hotelsFilter;
};
}  // namespace search
