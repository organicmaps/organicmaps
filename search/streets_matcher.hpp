#pragma once

#include "search/cbv.hpp"
#include "search/geocoder_context.hpp"
#include "search/token_range.hpp"

#include <vector>

namespace search
{
class FeaturesFilter;
class QueryParams;

class StreetsMatcher
{
public:
  struct Prediction
  {
    size_t GetNumTokens() const { return m_tokenRange.Size(); }

    // Tokens and misprints are better signals than probability here. For example in BA
    // searching for "French 1700", gives 100% probabilty 'street' "Peatonal Juramento 1700".
    bool SameForRelaxedMatch(Prediction const & rhs) const
    {
      return (m_withMisprints == rhs.m_withMisprints && GetNumTokens() == rhs.GetNumTokens());
    }
    bool IsBetter(Prediction const & rhs) const
    {
      // Tokens number on first place as the most valuable signal (even with misprints), see BarcelonaStreet.
      if (GetNumTokens() != rhs.GetNumTokens())
        return GetNumTokens() > rhs.GetNumTokens();

      if (m_withMisprints != rhs.m_withMisprints)
        return !m_withMisprints;

      return m_prob > rhs.m_prob;
    }

    CBV m_features;
    TokenRange m_tokenRange;
    bool m_withMisprints = false;
    double m_prob = 0.0;
    uint64_t m_hash = 0;
  };

  static void Go(BaseContext const & ctx, CBV const & candidates, FeaturesFilter const & filter,
                 QueryParams const & params, std::vector<Prediction> & predictions);

private:
  static void FindStreets(BaseContext const & ctx, CBV const & candidates, FeaturesFilter const & filter,
                          QueryParams const & params, std::vector<Prediction> & prediction);
};
}  // namespace search
