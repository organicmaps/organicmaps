#include "search/ranking_info.hpp"

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
double constexpr kDistanceToPivot = -1.0000000;
double constexpr kRank = 1.0000000;
// todo: (@t.yan) Adjust.
double constexpr kPopularity = 0.0500000;
double constexpr kFalseCats = -0.3691859;
double constexpr kErrorsMade = -0.0579812;
double constexpr kAllTokensUsed = 0.0000000;
double constexpr kHasName = 0.5;
double constexpr kNameScore[NameScore::NAME_SCORE_COUNT] = {
  -0.7245815 /* Zero */,
  0.1853727 /* Substring */,
  0.2046046 /* Prefix */,
  0.3346041 /* Full Match */
};
double constexpr kType[Model::TYPE_COUNT] = {
  -0.4458349 /* POI */,
  -0.4458349 /* Building */,
  -0.3001181 /* Street */,
  -0.3299295 /* Unclassified */,
  -0.3530548 /* Village */,
  0.4506418 /* City */,
  0.2889073 /* State */,
  0.6893882 /* Country */
};

// Coeffs sanity checks.
static_assert(kDistanceToPivot <= 0, "");
static_assert(kRank >= 0, "");
static_assert(kPopularity >= 0, "");
static_assert(kErrorsMade <= 0, "");
static_assert(kHasName >= 0, "");

double TransformDistance(double distance)
{
  return min(distance, RankingInfo::kMaxDistMeters) / RankingInfo::kMaxDistMeters;
}
}  // namespace

// static
double const RankingInfo::kMaxDistMeters = 2e6;

// static
void RankingInfo::PrintCSVHeader(ostream & os)
{
  os << "DistanceToPivot"
     << ",Rank"
     << ",Popularity"
     << ",NameScore"
     << ",ErrorsMade"
     << ",SearchType"
     << ",PureCats"
     << ",FalseCats"
     << ",AllTokensUsed";
}

string DebugPrint(RankingInfo const & info)
{
  ostringstream os;
  os << boolalpha;
  os << "RankingInfo [";
  os << "m_distanceToPivot:" << info.m_distanceToPivot << ",";
  os << "m_rank:" << static_cast<int>(info.m_rank) << ",";
  os << "m_popularity:" << static_cast<int>(info.m_popularity) << ",";
  os << "m_nameScore:" << DebugPrint(info.m_nameScore) << ",";
  os << "m_errorsMade:" << DebugPrint(info.m_errorsMade) << ",";
  os << "m_type:" << DebugPrint(info.m_type) << ",";
  os << "m_pureCats:" << info.m_pureCats << ",";
  os << "m_falseCats:" << info.m_falseCats << ",";
  os << "m_allTokensUsed:" << info.m_allTokensUsed;
  os << "m_categorialRequest:" << info.m_categorialRequest;
  os << "m_hasName:" << info.m_hasName;
  os << "]";
  return os.str();
}

void RankingInfo::ToCSV(ostream & os) const
{
  os << fixed;
  os << m_distanceToPivot << ",";
  os << static_cast<int>(m_rank) << ",";
  os << static_cast<int>(m_popularity) << ",";
  os << DebugPrint(m_nameScore) << ",";
  os << GetErrorsMade() << ",";
  os << DebugPrint(m_type) << ",";
  os << m_pureCats << ",";
  os << m_falseCats << ",";
  os << (m_allTokensUsed ? 1 : 0) << ",";
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
  result += kDistanceToPivot * distanceToPivot;
  result += kRank * rank;
  result += kPopularity * popularity;
  result += m_falseCats * kFalseCats;
  if (!m_categorialRequest)
  {
    result += kType[m_type];
    result += kNameScore[nameScore];
    result += kErrorsMade * GetErrorsMade();
    result += (m_allTokensUsed ? 1 : 0) * kAllTokensUsed;
  }
  else
  {
    result += m_hasName * kHasName;
  }
  return result;
}

size_t RankingInfo::GetErrorsMade() const
{
  return m_errorsMade.IsValid() ? m_errorsMade.m_errorsMade : 0;
}
}  // namespace search
