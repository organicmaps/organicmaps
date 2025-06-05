#pragma once

#include "search/intersection_result.hpp"
#include "search/model.hpp"
#include "search/token_range.hpp"

#include "indexer/feature_decl.hpp"

#include "geometry/point2d.hpp"

#include "base/assert.hpp"

#include <array>
#include <string>

namespace search
{
struct PreRankingInfo
{
  PreRankingInfo(Model::Type type, TokenRange const & range)
    : m_allTokensUsed(true)
    , m_exactMatch(true)
    , m_centerLoaded(false)
    , m_isCommonMatchOnly(false)
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

  m2::PointD m_center{0, 0};

  // An abstract distance from the feature to the pivot.  Measurement
  // units do not matter here.
  double m_distanceToPivot = 0;

  // Matched parts of the query.
  std::array<TokenRange, Model::TYPE_COUNT> m_tokenRanges;

  // Different geo-parts extracted from query.  Currently only poi,
  // building and street ids are in |m_geoParts|.
  IntersectionResult m_geoParts;

  // Id of the matched city, if any.
  FeatureID m_cityId;

  // Rank of the feature.
  uint8_t m_rank = 0;

  // Popularity rank of the feature.
  uint8_t m_popularity = 0;

  // Search type for the feature.
  Model::Type m_type = Model::TYPE_COUNT;

  // True iff all tokens that are not stop-words were used when retrieving the feature.
  bool m_allTokensUsed : 1;

  // True iff all tokens retrieved from search index were matched without misprints.
  bool m_exactMatch : 1;

  // Is m_center valid.
  bool m_centerLoaded : 1;

  // Result is matched by "common" tokens only, see QuaryParams::IsCommonToken. Low-scored if true.
  bool m_isCommonMatchOnly : 1;
};

std::string DebugPrint(PreRankingInfo const & info);
}  // namespace search
