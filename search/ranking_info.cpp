#include "search/ranking_info.hpp"

#include "std/cmath.hpp"
#include "std/iomanip.hpp"
#include "std/limits.hpp"
#include "std/sstream.hpp"

namespace search
{
namespace
{
// See search/search_quality/scoring_model.py for details.  In short,
// these coeffs correspond to coeffs in a linear model.
double const kDistanceToPivot = -0.37897824370302247;
double const kRank = 1.0;
double const kFalseCats = -0.05775625793967508;

double const kNameScore[NameScore::NAME_SCORE_COUNT] = {
     -0.11436302557264734 /* Zero */
    , 0.014295634567960331 /* Substring */
    , 0.046219090910780115 /* Prefix */
    , 0.05384830009390816 /* Full Match */
};

double const kType[Model::TYPE_COUNT] = {
      -0.09164609318265761 /* POI */
    , -0.09164609318265761 /* Building */
    , -0.0805969548653964 /* Street */
    , -0.030493728520630793 /* Unclassified */
    , -0.19242203325862917 /* Village */
    , -0.10945592241057521 /* City */
    , 0.19250143015921584 /* State */
    , 0.31211330207867427 /* Country */
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
  os << "m_errorsMade:" << DebugPrint(info.m_errorsMade) << ",";
  os << "m_type:" << DebugPrint(info.m_type) << ",";
  os << "m_pureCats:" << info.m_pureCats << ",";
  os << "m_falseCats:" << info.m_falseCats;
  os << "]";
  return os.str();
}

void RankingInfo::ToCSV(ostream & os) const
{
  os << fixed;
  os << m_distanceToPivot << "," << static_cast<int>(m_rank) << "," << DebugPrint(m_nameScore)
     << "," << DebugPrint(m_type) << "," << m_pureCats << "," << m_falseCats;
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

  return kDistanceToPivot * distanceToPivot + kRank * rank + kNameScore[nameScore] + kType[m_type] +
         m_falseCats * kFalseCats;
}
}  // namespace search
