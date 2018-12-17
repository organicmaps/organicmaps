#pragma once

#include "search/model.hpp"
#include "search/pre_ranking_info.hpp"
#include "search/ranking_utils.hpp"

#include <cstddef>
#include <cstdint>
#include <ostream>
#include <string>

class FeatureType;

namespace search
{
struct RankingInfo
{
  static double const kMaxDistMeters;

  // Distance from the feature to the pivot point.
  double m_distanceToPivot = kMaxDistMeters;

  // Rank of the feature.
  uint8_t m_rank = 0;

  // Popularity rank of the feature.
  uint8_t m_popularity = 0;

  // Score for the feature's name.
  NameScore m_nameScore = NAME_SCORE_ZERO;

  // Number of typos.
  ErrorsMade m_errorsMade;

  // True iff all tokens that are not stop-words
  // were used when retrieving the feature.
  bool m_allTokensUsed = true;

  // Search type for the feature.
  Model::Type m_type = Model::TYPE_COUNT;

  // True if all of the tokens that the feature was matched by
  // correspond to this feature's categories.
  bool m_pureCats = false;

  // True if none of the tokens that the feature was matched by
  // corresponds to this feature's categories although all of the
  // tokens are categorial ones.
  bool m_falseCats = false;

  // True iff the request is categorial.
  bool m_categorialRequest = false;

  // True iff the feature has a name.
  bool m_hasName = false;

  static void PrintCSVHeader(std::ostream & os);

  void ToCSV(std::ostream & os) const;

  // Returns rank calculated by a linear model. Large values
  // correspond to important features.
  double GetLinearModelRank() const;

  size_t GetErrorsMade() const;
};

std::string DebugPrint(RankingInfo const & info);
}  // namespace search
