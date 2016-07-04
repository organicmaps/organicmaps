#pragma once

#include "search/cbv.hpp"
#include "search/geocoder_context.hpp"

#include "std/vector.hpp"

namespace search
{
class FeaturesFilter;
struct QueryParams;

class StreetsMatcher
{
public:
  struct Prediction
  {
    inline size_t GetNumTokens() const { return m_endToken - m_startToken; }

    CBV m_features;

    size_t m_startToken = 0;
    size_t m_endToken = 0;

    double m_prob = 0.0;

    uint64_t m_hash = 0;
  };

  static void Go(BaseContext const & ctx, FeaturesFilter const & filter, QueryParams const & params,
                 vector<Prediction> & predictions);

private:
  static void FindStreets(BaseContext const & ctx, FeaturesFilter const & filter,
                          QueryParams const & params, vector<Prediction> & prediction);
};
}  // namespace search
