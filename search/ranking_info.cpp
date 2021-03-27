#include "search/ranking_info.hpp"

#include "search/utils.hpp"

#include "ugc/types.hpp"

#include "indexer/classificator.hpp"
#include "indexer/search_string_utils.hpp"

#include "base/assert.hpp"
#include "base/stl_helpers.hpp"

#include <array>
#include <iomanip>
#include <limits>
#include <sstream>

using namespace std;

namespace search
{
namespace
{
// See search/search_quality/scoring_model.py for details.  In short,
// these coeffs correspond to coeffs in a linear model.
double constexpr kHasName = 0.5;
double constexpr kCategoriesPopularity = 0.05;
double constexpr kCategoriesDistanceToPivot = -0.6874177;
double constexpr kCategoriesRank = 1.0000000;
double constexpr kCategoriesRating = 0.0500000;
double constexpr kCategoriesFalseCats = -1.0000000;
double constexpr kCategoriesRefusedByFilter = -1.0000000;

double constexpr kDistanceToPivot = -0.2123693;
double constexpr kRank = 0.1065355;
double constexpr kPopularity = 1.0000000;
double constexpr kRating = 0.0716319;
double constexpr kFalseCats = -0.4172461;
double constexpr kErrorsMade = -0.0391331;
double constexpr kMatchedFraction = 0.1876736;
double constexpr kAllTokensUsed = 0.0478513;
double constexpr kExactCountryOrCapital = 0.1247733;
double constexpr kRefusedByFilter = -1.0000000;
double constexpr kNameScore[NameScore::NAME_SCORE_COUNT] = {
  0.0085962 /* Zero */,
  -0.0099698 /* Substring */,
  -0.0158311 /* Prefix */,
  0.0172047 /* Full Match */
};
double constexpr kType[Model::TYPE_COUNT] = {
  -0.0467816 /* SUBPOI */,
  -0.0467816 /* COMPLEX_POI */,
  -0.0467816 /* Building */,
  -0.0444630 /* Street */,
  -0.0348396 /* Unclassified */,
  -0.0725383 /* Village */,
  0.0073583 /* City */,
  0.0233254 /* State */,
  0.1679389 /* Country */
};
double constexpr kResultType[base::Underlying(ResultType::Count)] = {
  0.0338794 /* TransportMajor */,
  0.0216298 /* TransportLocal */,
  0.0064977 /* Eat */,
  -0.0275763 /* Hotel */,
  0.0358858 /* Attraction */,
  -0.0195234 /* Service */,
  -0.0128952 /* General */
};

// Coeffs sanity checks.
static_assert(kHasName >= 0, "");
static_assert(kCategoriesPopularity >= 0, "");
static_assert(kDistanceToPivot <= 0, "");
static_assert(kRank >= 0, "");
static_assert(kPopularity >= 0, "");
static_assert(kErrorsMade <= 0, "");
static_assert(kExactCountryOrCapital >= 0, "");
static_assert(kCategoriesRefusedByFilter < 0, "");
static_assert(kRefusedByFilter < 0, "");

double TransformDistance(double distance)
{
  return min(distance, RankingInfo::kMaxDistMeters) / RankingInfo::kMaxDistMeters;
}

double TransformRating(pair<uint8_t, float> const & rating)
{
  double r = 0.0;
  // From statistics.
  double constexpr kAverageRating = 7.6;
  if (rating.first != 0)
  {
    r = (static_cast<double>(rating.second) - kAverageRating) /
        (ugc::UGC::kMaxRating - ugc::UGC::kRatingDetalizationThreshold);
    r *= static_cast<double>(rating.first) / 3.0 /* maximal confidence */;
  }
  return r;
}

void PrintParse(ostringstream & oss, array<TokenRange, Model::TYPE_COUNT> const & ranges,
                size_t numTokens)
{
  vector<Model::Type> types(numTokens, Model::Type::TYPE_COUNT);
  for (size_t i = 0; i < ranges.size(); ++i)
  {
    for (size_t pos : ranges[i])
    {
      CHECK_LESS(pos, numTokens, ());
      CHECK_EQUAL(types[pos], Model::Type::TYPE_COUNT, ());
      types[pos] = static_cast<Model::Type>(i);
    }
  }

  oss << "Parse [";
  for (size_t i = 0; i < numTokens; ++i)
  {
    if (i > 0)
      oss << " ";
    oss << DebugPrint(types[i]);
  }
  oss << "]";
}

class IsServiceTypeChecker
{
private:
  IsServiceTypeChecker()
  {
    array<string, 3> const oneLevelTypes = {"barrier", "power", "traffic_calming"};

    for (auto const & t : oneLevelTypes)
      m_oneLevelTypes.push_back(classif().GetTypeByPath({t}));
  }

public:
  static IsServiceTypeChecker const & Instance()
  {
    static const IsServiceTypeChecker instance;
    return instance;
  }

  bool operator()(feature::TypesHolder const & th) const
  {
    return base::AnyOf(th, [&](auto t) {
      ftype::TruncValue(t, 1);
      return find(m_oneLevelTypes.begin(), m_oneLevelTypes.end(), t) != m_oneLevelTypes.end();
    });
  }

private:
  vector<uint32_t> m_oneLevelTypes;
};
}  // namespace

// static
double const RankingInfo::kMaxDistMeters = 2e6;

// static
void RankingInfo::PrintCSVHeader(ostream & os)
{
  os << "DistanceToPivot"
     << ",Rank"
     << ",Popularity"
     << ",Rating"
     << ",NameScore"
     << ",ErrorsMade"
     << ",MatchedFraction"
     << ",SearchType"
     << ",ResultType"
     << ",PureCats"
     << ",FalseCats"
     << ",AllTokensUsed"
     << ",ExactCountryOrCapital"
     << ",IsCategorialRequest"
     << ",HasName";
}

string DebugPrint(RankingInfo const & info)
{
  ostringstream os;
  os << boolalpha;
  PrintParse(os, info.m_tokenRanges, info.m_numTokens);
  os << ", RankingInfo [";
  os << "m_distanceToPivot:" << info.m_distanceToPivot;
  os << ", m_rank:" << static_cast<int>(info.m_rank);
  os << ", m_popularity:" << static_cast<int>(info.m_popularity);
  os << ", m_rating:[" << static_cast<int>(info.m_rating.first) << ", " << info.m_rating.second
     << "]";
  os << ", m_nameScore:" << DebugPrint(info.m_nameScore);
  os << ", m_errorsMade:" << DebugPrint(info.m_errorsMade);
  os << ", m_isAltOrOldName: " << info.m_isAltOrOldName;
  os << ", m_numTokens:" << info.m_numTokens;
  os << ", m_matchedFraction:" << info.m_matchedFraction;
  os << ", m_type:" << DebugPrint(info.m_type);
  os << ", m_resultType:" << DebugPrint(info.m_resultType);
  os << ", m_pureCats:" << info.m_pureCats;
  os << ", m_falseCats:" << info.m_falseCats;
  os << ", m_allTokensUsed:" << info.m_allTokensUsed;
  os << ", m_exactCountryOrCapital:" << info.m_exactCountryOrCapital;
  os << ", m_categorialRequest:" << info.m_categorialRequest;
  os << ", m_hasName:" << info.m_hasName;
  os << ", m_refusedByFilter:" << info.m_refusedByFilter;
  os << "]";
  return os.str();
}

void RankingInfo::ToCSV(ostream & os) const
{
  os << fixed;
  os << m_distanceToPivot << ",";
  os << static_cast<int>(m_rank) << ",";
  os << static_cast<int>(m_popularity) << ",";
  os << TransformRating(m_rating) << ",";
  os << DebugPrint(m_nameScore) << ",";
  os << GetErrorsMadePerToken() << ",";
  os << m_matchedFraction << ",";
  os << DebugPrint(m_type) << ",";
  os << DebugPrint(m_resultType) << ",";
  os << m_pureCats << ",";
  os << m_falseCats << ",";
  os << (m_allTokensUsed ? 1 : 0) << ",";
  os << (m_exactCountryOrCapital ? 1 : 0) << ",";
  os << (m_categorialRequest ? 1 : 0) << ",";
  os << (m_hasName ? 1 : 0);
}

double RankingInfo::GetLinearModelRank() const
{
  // NOTE: this code must be consistent with scoring_model.py.  Keep
  // this in mind when you're going to change scoring_model.py or this
  // code. We're working on automatic rank calculation code generator
  // integrated in the build system.
  double const distanceToPivot = TransformDistance(m_distanceToPivot);
  double const rank = static_cast<double>(m_rank) / numeric_limits<uint8_t>::max();
  double const popularity = static_cast<double>(m_popularity) / numeric_limits<uint8_t>::max();
  double const rating = TransformRating(m_rating);

  auto nameScore = m_nameScore;
  if (m_pureCats || m_falseCats)
  {
    // If the feature was matched only by categorial tokens, it's
    // better for ranking to set name score to zero.  For example,
    // when we're looking for a "cafe", cafes "Cafe Pushkin" and
    // "Lermontov" both match to the request, but must be ranked in
    // accordance to their distances to the user position or viewport,
    // in spite of "Cafe Pushkin" has a non-zero name rank.
    nameScore = NAME_SCORE_ZERO;
  }

  double result = 0.0;
  if (!m_categorialRequest)
  {
    result += kDistanceToPivot * distanceToPivot;
    result += kRank * rank;
    result += kPopularity * popularity;
    result += kRating * rating;
    result += m_falseCats * kFalseCats;
    result += kType[m_type];
    if (Model::IsPoi(m_type))
    {
      CHECK_NOT_EQUAL(m_resultType, ResultType::Count, ());
      result += kResultType[base::Underlying(m_resultType)];
    }
    result += (m_allTokensUsed ? 1 : 0) * kAllTokensUsed;
    result += (m_exactCountryOrCapital ? 1 : 0) * kExactCountryOrCapital;
    auto const nameRank = kNameScore[nameScore] + kErrorsMade * GetErrorsMadePerToken() +
                          kMatchedFraction * m_matchedFraction;
    result += (m_isAltOrOldName ? 0.7 : 1.0) * nameRank;
    result += (m_refusedByFilter ? 1 : 0) * kRefusedByFilter;
  }
  else
  {
    result += kCategoriesDistanceToPivot * distanceToPivot;
    result += kCategoriesRank * rank;
    result += kCategoriesPopularity * popularity;
    result += kCategoriesRating * rating;
    result += kCategoriesFalseCats * kFalseCats;
    result += m_hasName * kHasName;
    result += (m_refusedByFilter ? 1 : 0) * kCategoriesRefusedByFilter;
  }
  return result;
}

// We build LevensteinDFA based on feature tokens to match query.
// Feature tokens can be longer than query tokens that's why every query token can be
// matched to feature token with maximal supported errors number.
// As maximal errors number depends only on tokens number (not tokens length),
// errorsMade per token is supposed to be a good metric.
double RankingInfo::GetErrorsMadePerToken() const
{
  size_t static const kMaxErrorsPerToken =
      GetMaxErrorsForTokenLength(numeric_limits<size_t>::max());
  if (!m_errorsMade.IsValid())
    return static_cast<double>(kMaxErrorsPerToken);

  CHECK_GREATER(m_numTokens, 0, ());
  return static_cast<double>(m_errorsMade.m_errorsMade) / static_cast<double>(m_numTokens);
}

ResultType GetResultType(feature::TypesHolder const & th)
{
  if (ftypes::IsEatChecker::Instance()(th))
    return ResultType::Eat;
  if (ftypes::IsHotelChecker::Instance()(th))
    return ResultType::Hotel;
  if (ftypes::IsRailwayStationChecker::Instance()(th) ||
      ftypes::IsSubwayStationChecker::Instance()(th) || ftypes::IsAirportChecker::Instance()(th))
  {
    return ResultType::TransportMajor;
  }
  if (ftypes::IsPublicTransportStopChecker::Instance()(th))
    return ResultType::TransportLocal;

  // We have several lists for attractions: short list in search categories for @tourism and long
  // list in ftypes::AttractionsChecker. We have highway-pedestrian, place-square, historic-tomb,
  // landuse-cemetery, amenity-townhall etc in long list and logic of long list is "if this object
  // has high popularity and/or wiki description probably it is attraction". It's better to use
  // short list here.
  auto static const attractionTypes =
      search::GetCategoryTypes("sights", "en", GetDefaultCategories());
  if (base::AnyOf(attractionTypes, [&th](auto t) { return th.Has(t); }))
    return ResultType::Attraction;

  if (IsServiceTypeChecker::Instance()(th))
    return ResultType::Service;

  return ResultType::General;
}

string DebugPrint(ResultType type)
{
  switch (type)
  {
  case ResultType::TransportMajor: return "TransportMajor";
  case ResultType::TransportLocal: return "TransportLocal";
  case ResultType::Eat: return "Eat";
  case ResultType::Hotel: return "Hotel";
  case ResultType::Attraction: return "Attraction";
  case ResultType::Service: return "Service";
  case ResultType::General: return "General";
  case ResultType::Count: return "Count";
  }
  UNREACHABLE();
}
}  // namespace search
