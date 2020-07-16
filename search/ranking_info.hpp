#pragma once

#include "search/model.hpp"
#include "search/pre_ranking_info.hpp"
#include "search/ranking_utils.hpp"

#include "indexer/feature_data.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <ostream>
#include <string>
#include <utility>

class FeatureType;

namespace search
{
enum class ResultType : uint8_t
{
  // Railway/subway stations, airports.
  TransportMajor,
  // Bus/tram stops.
  TransportLocal,
  // Cafes, restaurants, bars.
  Eat,
  // Hotels.
  Hotel,
  // Attractions.
  Attraction,
  // Service types: power lines and substations, barrier-fence, etc.
  Service,
  // All other POIs.
  General,
  Count
};

struct RankingInfo
{
  static double const kMaxDistMeters;

  static void PrintCSVHeader(std::ostream & os);

  void ToCSV(std::ostream & os) const;

  // Returns rank calculated by a linear model. Large values
  // correspond to important features.
  double GetLinearModelRank() const;

  double GetErrorsMadePerToken() const;

  // Distance from the feature to the pivot point.
  double m_distanceToPivot = kMaxDistMeters;

  // Rank of the feature.
  uint8_t m_rank = 0;

  // Popularity rank of the feature.
  uint8_t m_popularity = 0;

  // Confidence and UGC rating.
  std::pair<uint8_t, float> m_rating = {0, 0.0f};

  // Score for the feature's name.
  NameScore m_nameScore = NAME_SCORE_ZERO;

  // Number of misprints.
  ErrorsMade m_errorsMade;

  // alt_name or old_name is used.
  bool m_isAltOrOldName = false;

  // Query tokens number.
  size_t m_numTokens = 0;

  // Matched parts of the query.
  // todo(@m) Using TokenType instead of ModelType here would
  //          allow to distinguish postcodes too.
  std::array<TokenRange, Model::TYPE_COUNT> m_tokenRanges;

  // Fraction of characters from original query matched to feature.
  double m_matchedFraction = 0.0;

  // True iff all tokens that are not stop-words
  // were used when retrieving the feature.
  bool m_allTokensUsed = true;

  // True iff all tokens retrieved from search index were matched without misprints.
  bool m_exactMatch = true;

  // True iff feature has country or capital type and matches request: full match with all tokens
  // used and without misprints.
  bool m_exactCountryOrCapital = true;

  // Search type for the feature.
  Model::Type m_type = Model::TYPE_COUNT;

  // Type (food/transport/attraction/etc) for POI results for non-categorial requests.
  ResultType m_resultType = ResultType::Count;

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

  // We may want to show results which did not pass filter.
  bool m_refusedByFilter = false;
};

ResultType GetResultType(feature::TypesHolder const & th);

std::string DebugPrint(RankingInfo const & info);
std::string DebugPrint(ResultType type);
}  // namespace search
