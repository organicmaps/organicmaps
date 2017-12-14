#pragma once

#include "search/feature_offset_match.hpp"
#include "search/query_params.hpp"

#include "platform/mwm_traits.hpp"

#include "coding/reader.hpp"

#include "geometry/rect2d.hpp"

#include "base/cancellable.hpp"
#include "base/dfa_helpers.hpp"
#include "base/levenshtein_dfa.hpp"

#include "std/unique_ptr.hpp"

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

  Retrieval(MwmContext const & context, my::Cancellable const & cancellable);

  // Following functions retrieve from the search index corresponding to
  // |value| all features matching to |request|.
  unique_ptr<coding::CompressedBitVector> RetrieveAddressFeatures(
      SearchTrieRequest<strings::UniStringDFA> const & request) const;

  unique_ptr<coding::CompressedBitVector> RetrieveAddressFeatures(
      SearchTrieRequest<strings::PrefixDFAModifier<strings::UniStringDFA>> const & request) const;

  unique_ptr<coding::CompressedBitVector> RetrieveAddressFeatures(
      SearchTrieRequest<strings::LevenshteinDFA> const & request) const;

  unique_ptr<coding::CompressedBitVector> RetrieveAddressFeatures(
      SearchTrieRequest<strings::PrefixDFAModifier<strings::LevenshteinDFA>> const & request) const;

  // Retrieves from the search index corresponding to |value| all
  // postcodes matching to |slice|.
  unique_ptr<coding::CompressedBitVector> RetrievePostcodeFeatures(TokenSlice const & slice) const;

  // Retrieves from the geometry index corresponding to |value| all features belonging to |rect|.
  unique_ptr<coding::CompressedBitVector> RetrieveGeometryFeatures(m2::RectD const & rect,
                                                                   int scale) const;

private:
  template <template <typename> class R, typename... Args>
  unique_ptr<coding::CompressedBitVector> Retrieve(Args &&... args) const;

  MwmContext const & m_context;
  my::Cancellable const & m_cancellable;
  ModelReaderPtr m_reader;

  version::MwmTraits::SearchIndexFormat m_format;

  unique_ptr<TrieRoot<FeatureWithRankAndCenter>> m_root0;
  unique_ptr<TrieRoot<FeatureIndexValue>> m_root1;
};
}  // namespace search
