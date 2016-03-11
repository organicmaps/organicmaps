#include "search/v2/ranking_info.hpp"

#include "std/cmath.hpp"
#include "std/limits.hpp"

namespace search
{
namespace v2
{
// static
void RankingInfo::PrintCSVHeader(ostream & os)
{
  os << "DistanceToViewport"
     << ",DistanceToPosition"
     << ",Rank"
     << ",NameScore"
     << ",NameCoverage"
     << ",SearchType"
     << ",PositionInViewport";
}

string DebugPrint(RankingInfo const & info)
{
  ostringstream os;
  os << "RankingInfo [";
  os << "m_distanceToViewport:" << info.m_distanceToViewport << ",";
  os << "m_distanceToPosition:" << info.m_distanceToPosition << ",";
  os << "m_rank:" << static_cast<int>(info.m_rank) << ",";
  os << "m_nameScore:" << DebugPrint(info.m_nameScore) << ",";
  os << "m_nameCoverage:" << info.m_nameCoverage << ",";
  os << "m_searchType:" << DebugPrint(info.m_searchType) << ",";
  os << "m_positionInViewport:" << info.m_positionInViewport;
  os << "]";
  return os.str();
}

void RankingInfo::ToCSV(ostream & os) const
{
  os << fixed;
  os << m_distanceToViewport << "," << m_distanceToPosition << "," << static_cast<int>(m_rank)
     << "," << DebugPrint(m_nameScore) << "," << m_nameCoverage << "," << DebugPrint(m_searchType)
     << "," << m_positionInViewport;
}

double RankingInfo::GetLinearModelRank() const
{
  // See search/search_quality/scoring_model.py for details.  In
  // short, these coeffs correspond to coeffs in a linear model.

  // NOTE: this code must be consistent with scoring_model.py.  Keep
  // this in mind when you're going to change scoring_model.py or this
  // code. We're working on automatic rank calculation code generator
  // integrated in the build system.
  static double const kCoeffs[] = {0.98369469, 0.40219458, 0.97463078, 0.21027244, 0.07368054};

  double const minDistance =
      exp(-min(m_distanceToViewport, m_distanceToPosition) / PreRankingInfo::kMaxDistMeters);
  double const rank = static_cast<double>(m_rank) / numeric_limits<uint8_t>::max();
  double const nameScore = static_cast<double>(m_nameScore) / NAME_SCORE_COUNT;
  double const nameCoverage = m_nameCoverage;
  double const searchType = static_cast<double>(m_searchType) / SearchModel::SEARCH_TYPE_COUNT;

  return kCoeffs[0] * minDistance + kCoeffs[1] * rank + kCoeffs[2] * nameScore +
         kCoeffs[3] * nameCoverage + kCoeffs[4] * searchType;
}
}  // namespace v2
}  // namespace search
