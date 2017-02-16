#pragma once

#include "search/intersection_result.hpp"
#include "search/model.hpp"
#include "search/token_range.hpp"

#include "geometry/point2d.hpp"

#include "base/assert.hpp"

#include <cstdint>
#include <string>

namespace search
{
struct PreRankingInfo
{
  PreRankingInfo(SearchModel::SearchType type, TokenRange const & range)
  {
    ASSERT_LESS(type, SearchModel::SEARCH_TYPE_COUNT, ());
    m_searchType = type;
    m_tokenRange[m_searchType] = range;
  }

  inline TokenRange const & InnermostTokenRange() const
  {
    ASSERT_LESS(m_searchType, SearchModel::SEARCH_TYPE_COUNT, ());
    return m_tokenRange[m_searchType];
  }

  inline size_t GetNumTokens() const { return InnermostTokenRange().Size(); }

  // An abstract distance from the feature to the pivot.  Measurement
  // units do not matter here.
  double m_distanceToPivot = 0;

  m2::PointD m_center = m2::PointD::Zero();
  bool m_centerLoaded = false;

  // Tokens match to the feature name or house number.
  TokenRange m_tokenRange[SearchModel::SEARCH_TYPE_COUNT];

  // Different geo-parts extracted from query.  Currently only poi,
  // building and street ids are in |m_geoParts|.
  IntersectionResult m_geoParts;

  // Rank of the feature.
  uint8_t m_rank = 0;

  // Search type for the feature.
  SearchModel::SearchType m_searchType = SearchModel::SEARCH_TYPE_COUNT;
};

std::string DebugPrint(PreRankingInfo const & info);
}  // namespace search
