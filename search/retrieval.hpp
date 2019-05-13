#pragma once

#include "search/feature_offset_match.hpp"
#include "search/query_params.hpp"

#include "platform/mwm_traits.hpp"

#include "coding/reader.hpp"

#include "geometry/rect2d.hpp"

#include "base/cancellable.hpp"
#include "base/dfa_helpers.hpp"
#include "base/levenshtein_dfa.hpp"

#include <memory>

class MwmValue;

namespace coding
{
class CompressedBitVector;
}

namespace search
{
class MwmContext;
class TokenSlice;

class Retrieval
{
public:
  template<typename Value>
  using TrieRoot = trie::Iterator<ValueList<Value>>;
  using Features = std::unique_ptr<coding::CompressedBitVector>;

  struct ExtendedFeatures
  {
    Features m_features;
    Features m_exactMatchingFeatures;
  };

  Retrieval(MwmContext const & context, base::Cancellable const & cancellable);

  // Following functions retrieve all features matching to |request| from the search index.
  ExtendedFeatures RetrieveAddressFeatures(
      SearchTrieRequest<strings::UniStringDFA> const & request) const;

  ExtendedFeatures RetrieveAddressFeatures(
      SearchTrieRequest<strings::PrefixDFAModifier<strings::UniStringDFA>> const & request) const;

  ExtendedFeatures RetrieveAddressFeatures(
      SearchTrieRequest<strings::LevenshteinDFA> const & request) const;

  ExtendedFeatures RetrieveAddressFeatures(
      SearchTrieRequest<strings::PrefixDFAModifier<strings::LevenshteinDFA>> const & request) const;

  // Retrieves all postcodes matching to |slice| from the search index.
  Features RetrievePostcodeFeatures(TokenSlice const & slice) const;

  // Retrieves all features belonging to |rect| from the geometry index.
  Features RetrieveGeometryFeatures(m2::RectD const & rect, int scale) const;

private:
  template <template <typename> class R, typename... Args>
  ExtendedFeatures Retrieve(Args &&... args) const;

  MwmContext const & m_context;
  base::Cancellable const & m_cancellable;
  ModelReaderPtr m_reader;

  version::MwmTraits::SearchIndexFormat m_format;

  std::unique_ptr<TrieRoot<FeatureWithRankAndCenter>> m_root0;
  std::unique_ptr<TrieRoot<FeatureIndexValue>> m_root1;
};
}  // namespace search
