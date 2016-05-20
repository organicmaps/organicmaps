#include "search/v2/ranking_info.hpp"

#include "std/cmath.hpp"
#include "std/limits.hpp"
#include "std/sstream.hpp"

namespace search
{
namespace v2
{
namespace
{
// See search/search_quality/scoring_model.py for details.  In short,
// these coeffs correspond to coeffs in a linear model.
double const kDistanceToPivot = -1.0000000;
double const kRank = 0.5430747;
double const kNameScore[NameScore::NAME_SCORE_COUNT] = {
  -0.3686323 /* Zero */,
  0.0977193 /* Substring Prefix */,
  0.1340500 /* Substring */,
  0.1368631 /* Full Match Prefix */,
  0.1368631 /* Full Match */
};
double const kSearchType[SearchModel::SEARCH_TYPE_COUNT] = {
  -0.9195533 /* POI */,
  -0.9195533 /* Building */,
  -0.1470504 /* Street */,
  -0.6392620 /* Unclassified */,
  -0.0900970 /* Village */,
  0.4383605 /* City */,
  0.6296097 /* State */,
  0.7279924 /* Country */
};

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
     << ",NameScore"
     << ",SearchType"
     << ",PureCats"
     << ",FalseCats";
}

string DebugPrint(RankingInfo const & info)
{
  ostringstream os;
  os << "RankingInfo [";
  os << "m_distanceToPivot:" << info.m_distanceToPivot << ",";
  os << "m_rank:" << static_cast<int>(info.m_rank) << ",";
  os << "m_nameScore:" << DebugPrint(info.m_nameScore) << ",";
  os << "m_searchType:" << DebugPrint(info.m_searchType) << ",";
  os << "m_pureCats:" << info.m_pureCats << ",";
  os << "m_falseCats:" << info.m_falseCats;
  os << "]";
  return os.str();
}

void RankingInfo::ToCSV(ostream & os) const
{
  os << fixed;
  os << m_distanceToPivot << "," << static_cast<int>(m_rank) << "," << DebugPrint(m_nameScore)
     << "," << DebugPrint(m_searchType) << "," << m_pureCats << "," << m_falseCats;
}

double RankingInfo::GetLinearModelRank() const
{
  // NOTE: this code must be consistent with scoring_model.py.  Keep
  // this in mind when you're going to change scoring_model.py or this
  // code. We're working on automatic rank calculation code generator
  // integrated in the build system.
  double const distanceToPivot = TransformDistance(m_distanceToPivot);
  double const rank = static_cast<double>(m_rank) / numeric_limits<uint8_t>::max();

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

  return kDistanceToPivot * distanceToPivot + kRank * rank + kNameScore[nameScore] +
         kSearchType[m_searchType];
}
}  // namespace v2
}  // namespace search
