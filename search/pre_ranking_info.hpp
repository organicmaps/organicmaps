#pragma once

#include "search/model.hpp"
#include "search/token_range.hpp"

#include "geometry/point2d.hpp"

#include "std/cstdint.hpp"

namespace search
{
struct PreRankingInfo
{
  inline size_t GetNumTokens() const { return m_tokenRange.Size(); }

  // An abstract distance from the feature to the pivot.  Measurement
  // units do not matter here.
  double m_distanceToPivot = 0;

  m2::PointD m_center = m2::PointD::Zero();
  bool m_centerLoaded = false;

  // Tokens match to the feature name or house number.
  TokenRange m_tokenRange;

  // Rank of the feature.
  uint8_t m_rank = 0;

  // Search type for the feature.
  SearchModel::SearchType m_searchType = SearchModel::SEARCH_TYPE_COUNT;
};

string DebugPrint(PreRankingInfo const & info);

}  // namespace search
