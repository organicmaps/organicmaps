#pragma once

#include "search/v2/geocoder.hpp"
#include "search/v2/pre_ranking_info.hpp"
#include "search/v2/ranking_utils.hpp"
#include "search/v2/search_model.hpp"

class FeatureType;

namespace search
{
namespace v2
{
struct RankingInfo
{
  static double const kMaxDistMeters;

  // Distance from the feature to the pivot point.
  double m_distanceToPivot = kMaxDistMeters;

  // Rank of the feature.
  uint8_t m_rank = 0;

  // Score for the feature's name.
  NameScore m_nameScore = NAME_SCORE_ZERO;

  // Fraction of tokens from the query matched to a feature name.
  double m_nameCoverage = 0;

  // Search type for the feature.
  SearchModel::SearchType m_searchType = SearchModel::SEARCH_TYPE_COUNT;

  static void PrintCSVHeader(ostream & os);

  void ToCSV(ostream & os) const;

  // Returns rank calculated by a linear model. Large values
  // correspond to important features.
  double GetLinearModelRank() const;
};

string DebugPrint(RankingInfo const & info);
}  // namespace v2
}  // namespace search
