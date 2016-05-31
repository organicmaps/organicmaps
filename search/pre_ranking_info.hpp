#pragma once

#include "search/search_model.hpp"

#include "std/cstdint.hpp"

namespace search
{
struct PreRankingInfo
{
  inline size_t GetNumTokens() const { return m_endToken - m_startToken; }

  // An abstract distance from the feature to the pivot.  Measurement
  // units do not matter here.
  double m_distanceToPivot = 0;

  // Tokens [m_startToken, m_endToken) match to the feature name or
  // house number.
  size_t m_startToken = 0;
  size_t m_endToken = 0;

  // Rank of the feature.
  uint8_t m_rank = 0;

  // Search type for the feature.
  SearchModel::SearchType m_searchType = SearchModel::SEARCH_TYPE_COUNT;
};

string DebugPrint(PreRankingInfo const & info);

}  // namespace search
