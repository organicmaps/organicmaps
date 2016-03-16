#pragma once

#include "search/v2/search_model.hpp"

#include "std/cstdint.hpp"

namespace search
{
namespace v2
{
struct PreRankingInfo
{
  // An abstract distance from the feature to the viewport.
  // Measurement units do not matter here.
  double m_distanceToViewport = 0;

  // An abstract distance from the feature to the user's position.
  // Measurement units do not matter here.
  double m_distanceToPosition = 0;

  // Tokens [m_startToken, m_endToken) match to the feature name or
  // house number.
  size_t m_startToken = 0;
  size_t m_endToken = 0;

  // Rank of the feature.
  uint8_t m_rank = 0;

  // Search type for the feature.
  SearchModel::SearchType m_searchType = SearchModel::SEARCH_TYPE_COUNT;
};
}  // namespace v2
}  // namespace search
