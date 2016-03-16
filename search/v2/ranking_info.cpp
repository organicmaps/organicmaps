#include "search/v2/ranking_info.hpp"

#include "std/cmath.hpp"
#include "std/limits.hpp"

namespace search
{
namespace v2
{
namespace
{
double const kDistanceToViewport = 1.850;
double const kDistanceToPosition = 85.898;
double const kMinDistance = 6.908;
double const kRank = 78.441;
double const kNameScore = 1.0;
double const kNameCoverage = 0.0;
double const kPositionInViewport = 0.0;

double TransformDistance(double distance)
{
  return exp(-distance * 1000 / RankingInfo::kMaxDistMeters);
}
}  // namespace

// static
double const RankingInfo::kMaxDistMeters = 1e9;

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
  double const distanceToViewport = TransformDistance(m_distanceToViewport);
  double const distanceToPosition = TransformDistance(m_distanceToPosition);
  double const minDistance = min(distanceToViewport, distanceToPosition);
  double const rank = static_cast<double>(m_rank) / numeric_limits<uint8_t>::max();
  double const nameScore = static_cast<double>(m_nameScore) / NAME_SCORE_FULL_MATCH;
  double const nameCoverage = m_nameCoverage;
  double const positionInViewport = static_cast<double>(m_positionInViewport);

  double searchType;
  switch (m_searchType)
  {
    case SearchModel::SEARCH_TYPE_POI:
    case SearchModel::SEARCH_TYPE_BUILDING:
      searchType = 0;
      break;
    default:
      searchType = m_searchType - 1;
      break;
  }
  searchType = searchType / (SearchModel::SEARCH_TYPE_COUNTRY - 1);

  return kDistanceToViewport * distanceToViewport + kDistanceToPosition * distanceToPosition +
         kMinDistance * minDistance + kRank * rank + kNameScore * nameScore +
         kNameCoverage * nameCoverage + kPositionInViewport * positionInViewport;
}
}  // namespace v2
}  // namespace search
