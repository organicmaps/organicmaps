#pragma once

#include "search/intersection_result.hpp"
#include "search/model.hpp"
#include "search/token_range.hpp"

#include "indexer/feature_decl.hpp"

#include "geometry/point2d.hpp"

#include "base/assert.hpp"

#include <array>
#include <cstdint>
#include <string>
#include <utility>

namespace search
{
struct PreRankingInfo
{
  PreRankingInfo(Model::Type type, TokenRange const & range)
  {
    ASSERT_LESS(type, Model::TYPE_COUNT, ());
    m_type = type;
    m_tokenRanges[m_type] = range;
  }

  TokenRange const & InnermostTokenRange() const
  {
    ASSERT_LESS(m_type, Model::TYPE_COUNT, ());
    return m_tokenRanges[m_type];
  }

  // An abstract distance from the feature to the pivot.  Measurement
  // units do not matter here.
  double m_distanceToPivot = 0;

  m2::PointD m_center = m2::PointD::Zero();
  bool m_centerLoaded = false;

  // Matched parts of the query.
  std::array<TokenRange, Model::TYPE_COUNT> m_tokenRanges;

  // Different geo-parts extracted from query.  Currently only poi,
  // building and street ids are in |m_geoParts|.
  IntersectionResult m_geoParts;

  // Id of the matched city, if any.
  FeatureID m_cityId;

  // True iff all tokens that are not stop-words
  // were used when retrieving the feature.
  bool m_allTokensUsed = true;

  // True iff all tokens retrieved from search index were matched without misprints.
  bool m_exactMatch = true;

  // Rank of the feature.
  uint8_t m_rank = 0;

  // Popularity rank of the feature.
  uint8_t m_popularity = 0;

  // Search type for the feature.
  Model::Type m_type = Model::TYPE_COUNT;
};

std::string DebugPrint(PreRankingInfo const & info);
}  // namespace search
