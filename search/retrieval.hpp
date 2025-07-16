#pragma once

#include "search/cbv.hpp"
#include "search/feature_offset_match.hpp"
#include "search/query_params.hpp"

#include "platform/mwm_traits.hpp"

#include "coding/reader.hpp"

#include "geometry/rect2d.hpp"

#include "base/cancellable.hpp"
#include "base/checked_cast.hpp"
#include "base/dfa_helpers.hpp"
#include "base/levenshtein_dfa.hpp"

#include <cstdint>
#include <functional>
#include <memory>
#include <utility>

class MwmValue;

namespace search
{
class MwmContext;
class TokenSlice;

class Retrieval
{
public:
  template <typename Value>
  using TrieRoot = trie::Iterator<ValueList<Value>>;
  using Features = search::CBV;

  struct ExtendedFeatures
  {
    ExtendedFeatures() = default;
    ExtendedFeatures(ExtendedFeatures const &) = default;
    ExtendedFeatures(ExtendedFeatures &&) = default;

    explicit ExtendedFeatures(Features const & cbv) : m_features(cbv), m_exactMatchingFeatures(cbv) {}

    ExtendedFeatures(Features && features, Features && exactMatchingFeatures)
      : m_features(std::move(features))
      , m_exactMatchingFeatures(std::move(exactMatchingFeatures))
    {}

    ExtendedFeatures & operator=(ExtendedFeatures const &) = default;
    ExtendedFeatures & operator=(ExtendedFeatures &&) = default;

    ExtendedFeatures Intersect(ExtendedFeatures const & rhs) const
    {
      ExtendedFeatures result;
      result.m_features = m_features.Intersect(rhs.m_features);
      result.m_exactMatchingFeatures = m_exactMatchingFeatures.Intersect(rhs.m_exactMatchingFeatures);
      return result;
    }

    ExtendedFeatures Intersect(Features const & cbv) const
    {
      ExtendedFeatures result;
      result.m_features = m_features.Intersect(cbv);
      result.m_exactMatchingFeatures = m_exactMatchingFeatures.Intersect(cbv);
      return result;
    }

    void SetFull()
    {
      m_features.SetFull();
      m_exactMatchingFeatures.SetFull();
    }

    void ForEach(std::function<void(uint32_t, bool)> const & f) const
    {
      m_features.ForEach([&](uint64_t id)
      { f(base::asserted_cast<uint32_t>(id), m_exactMatchingFeatures.HasBit(id)); });
    }

    Features m_features;
    Features m_exactMatchingFeatures;
  };

  Retrieval(MwmContext const & context, base::Cancellable const & cancellable);

  // Following functions retrieve all features matching to |request| from the search index.
  ExtendedFeatures RetrieveAddressFeatures(SearchTrieRequest<strings::UniStringDFA> const & request) const;

  ExtendedFeatures RetrieveAddressFeatures(
      SearchTrieRequest<strings::PrefixDFAModifier<strings::UniStringDFA>> const & request) const;

  ExtendedFeatures RetrieveAddressFeatures(SearchTrieRequest<strings::LevenshteinDFA> const & request) const;

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

  std::unique_ptr<TrieRoot<Uint64IndexValue>> m_root;
};
}  // namespace search
