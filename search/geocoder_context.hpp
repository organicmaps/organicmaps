#pragma once

#include "search/cbv.hpp"
#include "search/cuisine_filter.hpp"
#include "search/features_layer.hpp"
#include "search/geocoder_locality.hpp"
#include "search/model.hpp"
#include "search/retrieval.hpp"

#include <memory>
#include <string>
#include <vector>

namespace search
{
class FeaturesFilter;
class TokenRange;

struct BaseContext
{
  enum TokenType
  {
    TOKEN_TYPE_SUBPOI,
    TOKEN_TYPE_COMPLEX_POI,
    TOKEN_TYPE_BUILDING,
    TOKEN_TYPE_STREET,
    TOKEN_TYPE_SUBURB,
    TOKEN_TYPE_UNCLASSIFIED,
    TOKEN_TYPE_VILLAGE,
    TOKEN_TYPE_CITY,
    TOKEN_TYPE_STATE,
    TOKEN_TYPE_COUNTRY,
    TOKEN_TYPE_POSTCODE,

    TOKEN_TYPE_COUNT
  };

  static TokenType FromModelType(Model::Type type);
  static TokenType FromRegionType(Region::Type type);

  size_t NumTokens() const;

  // Advances |curToken| to the nearest unused token, or to the end of
  // |m_usedTokens| if there are no unused tokens.
  size_t SkipUsedTokens(size_t curToken) const;

  // Returns true if |token| is marked as used.
  bool IsTokenUsed(size_t token) const;

  // Returns true iff all tokens are used.
  bool AllTokensUsed() const;

  // Returns true if there exists at least one used token in |range|.
  bool HasUsedTokensInRange(TokenRange const & range) const;

  // Counts number of groups of consecutive unused tokens.
  size_t NumUnusedTokenGroups() const;

  // List of bit-vectors of features, where i-th element of the list
  // corresponds to the i-th token in the search query.
  std::vector<Retrieval::ExtendedFeatures> m_features;
  CBV m_villages;
  CBV m_streets;
  CBV m_suburbs;

  // Stack of layers filled during geocoding.
  std::vector<FeaturesLayer> m_layers;

  // Stack of regions filled during geocoding.
  std::vector<Region const *> m_regions;

  City const * m_city = nullptr;

  // This vector is used to indicate what tokens were already matched
  // and can't be re-used during the geocoding process.
  std::vector<TokenType> m_tokens;

  // The total number of results emitted using this
  // context in all branches of the search.
  size_t m_numEmitted = 0;

  std::unique_ptr<cuisine_filter::CuisineFilter::ScopedFilter> m_cuisineFilter;
};

std::string DebugPrint(BaseContext::TokenType type);
std::string ToString(BaseContext::TokenType type);
}  // namespace search
