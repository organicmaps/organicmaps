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
double const kDistanceToPivot = -0.1940753;
double const kRank = 0.6904166;
double const kFalseCats = -0.0944214;
double const kErrorsMade = -0.0167366;
double const kAllTokensUsed = 1.0000000;
double const kNameScore[NameScore::NAME_SCORE_COUNT] = {
  -0.0704004 /* Zero */,
  0.0178957 /* Substring */,
  0.0274588 /* Prefix */,
  0.0250459 /* Full Match */
};
double const kType[Model::TYPE_COUNT] = {
  -0.0164093 /* POI */,
  -0.0164093 /* Building */,
  0.0067338 /* Street */,
  0.0388209 /* Unclassified */,
  -0.0599702 /* Village */,
  -0.0996292 /* City */,
  0.0929370 /* State */,
  0.0375170 /* Country */
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
     << ",ErrorsMade"
     << ",SearchType"
     << ",PureCats"
     << ",FalseCats"
     << ",AllTokensUsed";
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
  os << "m_falseCats:" << info.m_falseCats << ",";
  os << "m_allTokensUsed:" << boolalpha << info.m_allTokensUsed;
  os << "]";
  return os.str();
}

void RankingInfo::ToCSV(ostream & os) const
{
  os << fixed;
  os << m_distanceToPivot << ",";
  os << static_cast<int>(m_rank) << ",";
  os << DebugPrint(m_nameScore) << ",";
  os << GetErrorsMade() << ",";
  os << DebugPrint(m_type) << ",";
  os << m_pureCats << ",";
  os << m_falseCats << ",";
  os << (m_allTokensUsed ? 1 : 0);
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

  double result = 0.0;
  result += kDistanceToPivot * distanceToPivot;
  result += kRank * rank;
  result += kNameScore[nameScore];
  result += kErrorsMade * GetErrorsMade();
  result += kType[m_type];
  result += m_falseCats * kFalseCats;
  result += (m_allTokensUsed ? 1 : 0) * kAllTokensUsed;
  return result;
}

size_t RankingInfo::GetErrorsMade() const
{
  return m_errorsMade.IsValid() ? m_errorsMade.m_errorsMade : 0;
}
}  // namespace search
