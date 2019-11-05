#pragma once

#include "search/cbv.hpp"
#include "search/geocoder_context.hpp"
#include "search/token_range.hpp"

#include <cstddef>
#include <cstdint>
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
    inline size_t GetNumTokens() const { return m_tokenRange.Size(); }

    CBV m_features;
    TokenRange m_tokenRange;
    bool m_withMisprints = false;
    double m_prob = 0.0;
    uint64_t m_hash = 0;
  };

  static void Go(BaseContext const & ctx, CBV const & candidates, FeaturesFilter const & filter,
                 QueryParams const & params, std::vector<Prediction> & predictions);

private:
  static void FindStreets(BaseContext const & ctx, CBV const & candidates,
                          FeaturesFilter const & filter, QueryParams const & params,
                          std::vector<Prediction> & prediction);
};
}  // namespace search
