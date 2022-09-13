#pragma once

#include "search/model.hpp"
#include "search/ranking_utils.hpp"
#include "search/token_range.hpp"

#include "indexer/ftypes_matcher.hpp"

#include <array>
#include <ostream>
#include <string>

namespace feature { class TypesHolder; }

namespace search
{
enum class PoiType : uint8_t
{
  // Railway/subway stations, airports.
  TransportMajor,
  // Bus/tram stops.
  TransportLocal,
  // Cafes, restaurants, bars.
  Eat,
  // Hotels.
  Hotel,
  // Shops.
  Shop,
  // Attractions.
  Attraction,
  // Service types: power lines and substations, barrier-fence, etc.
  Service,
  // All other POIs.
  General,
  Count
};

using StreetType = ftypes::IsWayChecker::SearchRank;

struct StoredRankingInfo
{
  // Do not change this constant, it is used to normalize distance and takes part in distance rank, accordingly.
  static double constexpr kMaxDistMeters = 2.0E6;

  // Distance from the feature to the pivot point.
  double m_distanceToPivot = kMaxDistMeters;

  // Search type for the feature.
  Model::Type m_type = Model::TYPE_COUNT;

  // Used for non-categorial requests.
  union
  {
    PoiType poi;        // type (food/transport/attraction/etc) for POI results
    StreetType street;  // type (peddestrian/residential/regular/etc) for Street results
  } m_classifType;
};

struct RankingInfo : public StoredRankingInfo
{
  RankingInfo()
    : m_isAltOrOldName(false)
    , m_allTokensUsed(true)
    , m_exactMatch(true)
    , m_exactCountryOrCapital(true)
    , m_pureCats(false)
    , m_falseCats(false)
    , m_categorialRequest(false)
    , m_hasName(false)
  {
    m_classifType.street = StreetType::Default;
  }

  static void PrintCSVHeader(std::ostream & os);

  void ToCSV(std::ostream & os) const;

  // Returns rank calculated by a linear model, bigger is better.
  double GetLinearModelRank() const;

  double GetErrorsMadePerToken() const;

  NameScore GetNameScore() const;
  PoiType GetPoiType() const;

  // Matched parts of the query.
  // todo(@m) Using TokenType instead of ModelType here would
  //          allow to distinguish postcodes too.
  std::array<TokenRange, Model::TYPE_COUNT> m_tokenRanges;

  // Fraction of characters from original query matched to feature.
  float m_matchedFraction = 0.0;

  // Query tokens number.
  uint16_t m_numTokens = 0;

  // > 0 if matched only common tokens. Bigger is worse, depending on Feature's name tokens count.
  int16_t m_commonTokensFactor = 0;

  // Number of misprints.
  ErrorsMade m_errorsMade;

  // Rank of the feature.
  uint8_t m_rank = 0;

  // Popularity rank of the feature.
  uint8_t m_popularity = 0;

  // Score for the feature's name.
  NameScore m_nameScore = NameScore::ZERO;

  // alt_name or old_name is used.
  bool m_isAltOrOldName : 1;

  // True iff all tokens that are not stop-words
  // were used when retrieving the feature.
  bool m_allTokensUsed : 1;

  // True iff all tokens retrieved from search index were matched without misprints.
  bool m_exactMatch : 1;

  // True iff feature has country or capital type and matches request: full match with all tokens
  // used and without misprints.
  bool m_exactCountryOrCapital : 1;

  // True if all of the tokens that the feature was matched by
  // correspond to this feature's categories.
  bool m_pureCats : 1;

  // True if none of the tokens that the feature was matched by
  // corresponds to this feature's categories although all of the
  // tokens are categorial ones.
  bool m_falseCats : 1;

  // True iff the request is categorial.
  bool m_categorialRequest : 1;

  // True iff the feature has a name.
  bool m_hasName : 1;
};

PoiType GetPoiType(feature::TypesHolder const & th);

std::string DebugPrint(StoredRankingInfo const & info);
std::string DebugPrint(RankingInfo const & info);

std::string DebugPrint(PoiType type);
std::string DebugPrint(StreetType type);
}  // namespace search
